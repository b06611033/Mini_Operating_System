#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
VMPool * PageTable::vm_pool_head = NULL;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
}

PageTable::PageTable()
{
   // PAGING NOT TURNED ON YET, SO ANY REFERENCE IS BY PHYSICAL ADDRESS!
   // allocate physical memory frame to store page_directory
   page_directory = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE); // allocate a frame and assign frame address to pointer
   unsigned long *page_table = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long address = 0;
   for(int i = 0; i < 1024; i++) {
      // first 12 bits can be used without effecting the address, since frame size is 4096 = 2^12
      page_table[i] = address | 3; // convert first 3 bits to 011 (supervisor, read/write, present)
      address += PAGE_SIZE;
   }
   page_directory[0] = (unsigned long)page_table;
   page_directory[0] = page_directory[0] | 3;
   for(int i = 1; i < 1023; i++) {
      // address doesn't matter, because not present
      page_directory[i] = 0 | 2; // convert first 3 bits to 010 (supervisor, read/write, not present)
   }
   page_directory[1023] = (unsigned long)page_directory | 3;
   paging_enabled = 0; // paging disabled initially
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long) current_page_table->page_directory); // storing address of page directory to cr3 register, so system knows where to start translating address
}

void PageTable::enable_paging()
{
   paging_enabled = 1;
   write_cr0(read_cr0() | 0x80000000);
}

void PageTable::handle_fault(REGS * _r)
{
   unsigned long address = read_cr2(); // read the fault address (logical address)
   unsigned long pde_offset = address >> 22;  // get the offset for page directory entry
   unsigned long pte_offset = (address >> 12) & 1023; // get the offset for page table entry
   unsigned long *logical_page_directory = (unsigned long *) 0xFFFFF000; // logical address of page directory 1023 | 1023 | 0
   unsigned long *logical_page_table = (unsigned long *) (0xFFC00000 | pde_offset << 12); // logical address of page table 1023 | pde offset | 0
   // check if address is legit
   unsigned int legit = 0;
   VMPool * curr = vm_pool_head;
   while(curr != NULL) {
      if(curr->is_legitimate(address) == true) {
         legit = 1;
         break;
      }
      curr = curr->next;
   }
   if(legit == 0 && curr != NULL) {
      assert(false);
   }
   // address is legit, so it is either PDE or PTE not present
   unsigned int present;
   present = logical_page_directory[pde_offset] & 1;
   // page fault because pde not present or pte not present
   if(present == 0) {
      unsigned long *new_table = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE); // allocate physical memory frame for new table
      logical_page_directory[pde_offset] = (unsigned long) new_table; // store the physical address in pde
      logical_page_directory[pde_offset] |= 3;
      for(int i = 0; i < 1024; i++) {
         logical_page_table[i] = 0|2; // entry in new table not present yet
      }
   }
   else {
      unsigned long *new_page = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE); // allocate physical memory frame for new page
      logical_page_table[pte_offset] = (unsigned long) new_page; // store the physical address in pte
      logical_page_table[pte_offset] |= 3;
   }

}

void PageTable::register_pool(VMPool * _vm_pool){
   if(vm_pool_head == NULL) {
      vm_pool_head = _vm_pool;
      return;
   }
   VMPool * curr = vm_pool_head;
   while(curr->next != NULL) {
      curr = curr->next;
   }
   curr->next = _vm_pool;
}

void PageTable::free_page(unsigned long _page_no) {
   unsigned long pde_offset = _page_no >> 22;  // get the offset for page directory entry
   unsigned long pte_offset = (_page_no >> 12) & 1023; // get the offset for page table entry
   unsigned long *page_table = (unsigned long *) (0xFFC00000 | (pde_offset << 12)); // logical address of page_table
   if((page_table[pte_offset] & 1) == 1) {
      process_mem_pool->release_frames(page_table[pte_offset]/PAGE_SIZE);  // release the frame
      // don't need to worry about the first 12 bits, because division of long will return the floor value
      page_table[pte_offset] = 2;  // set PTE to invalid
      load(); // flush TLB
   }
}

