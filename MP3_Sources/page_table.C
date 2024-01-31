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
   page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE); // allocate a frame and assign frame address to pointer
   unsigned long *page_table = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long address = 0;
   for(int i = 0; i < 1024; i++) {
      // first 12 bits can be used without effecting the address, since frame size is 4096 = 2^12
      page_table[i] = address | 3; // convert first 3 bits to 011 (supervisor, read/write, present)
      address += PAGE_SIZE;
   }
   page_directory[0] = (unsigned long)page_table;
   page_directory[0] = page_directory[0] | 3;
   for(int i = 1; i < 1024; i++) {
      // address doesn't matter, because not present
      page_directory[i] = 0 | 2; // convert first 3 bits to 010 (supervisor, read/write, not present)
   }
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
   unsigned long address = read_cr2(); // read the fault address (virtual address)
   unsigned long pde_offset = address >> 22;  // get the offset for page directory entry
   unsigned long pte_offset = (address >> 12) & 1023; // get the offset for page table entry
   unsigned int present;
   present = current_page_table->page_directory[pde_offset] & 1;
   // page fault because pde not present or pte not present
   if(present == 0) {
      unsigned long *new_table = (unsigned long *) (kernel_mem_pool->get_frames(1)*PAGE_SIZE); // allocate  memory for new table and assign address to pointer
      current_page_table->page_directory[pde_offset] = (unsigned long) new_table; // store the address in pde
      current_page_table->page_directory[pde_offset] |= 3;
      for(int i = 0; i < 1024; i++) {
         new_table[i] = 0|2; // entry in new table not present yet
      }
   }
   else {
      unsigned long *new_page = (unsigned long *) (process_mem_pool->get_frames(1)*PAGE_SIZE); // allocate  memory for new page and assign address to pointer
      unsigned long *table = (unsigned long *) (current_page_table->page_directory[pde_offset] >> 12 << 12); // get the table address from pde by turning first 12 bits to 0
      table[pte_offset] = (unsigned long) new_page;
      table[pte_offset] |= 3;
   }

}

