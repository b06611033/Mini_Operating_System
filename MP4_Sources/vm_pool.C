/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    region_array = (Region*) _base_address;
    page_table->register_pool(this);
    num_of_region = 0;   // no region assigned yet
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned long frames = (_size/Machine::PAGE_SIZE); // number of frames to be allocated
    if(_size%Machine::PAGE_SIZE != 0) {
        frames += 1;
    }
    int curr = num_of_region;
    if(num_of_region == 0) {
        // check if the region allocated will exceed pool range
        if((base_address + Machine::PAGE_SIZE + frames*Machine::PAGE_SIZE) > (base_address + size)) {
            return 0;
        }
        region_array[0].start_address = base_address + Machine::PAGE_SIZE; // first page is used to store region array, so can't be used
        region_array[0].size = frames*Machine::PAGE_SIZE;
    }
    else {
        for(int i = 1; i < num_of_region; i++) {
            if((region_array[i].start_address - (region_array[i-1].start_address + region_array[i-1].size)) > (frames*Machine::PAGE_SIZE)){
                curr = i;
                break;
            }
        }
        if(curr == num_of_region) {
            // check if the region allocated will exceed pool range
            if((region_array[num_of_region-1].start_address + region_array[num_of_region-1].size + frames*Machine::PAGE_SIZE ) > (base_address + size)) {
              return 0;
            }
        }
        // reorganize region array, so it is sorted according to start_address
        for(int i = curr; i < num_of_region; i++) {
            region_array[i+1] = region_array[i];
        }
        region_array[curr].start_address = region_array[curr-1].start_address + region_array[curr-1].size;
        region_array[curr].size = frames*Machine::PAGE_SIZE;
    }
    num_of_region++;
    return region_array[curr].start_address;
}

void VMPool::release(unsigned long _start_address) {
    int region_number = num_of_region;
    for(int i = 0; i < num_of_region; i++) {
        if(region_array[i].start_address == _start_address) {
            region_number = i;
            break;
        }
    }
    if(region_number == num_of_region) {
        return;
    }
    // free all pages in the region
    for(int i = 0; i < region_array[region_number].size/Machine::PAGE_SIZE; i++) {
        page_table->free_page(region_array[region_number].start_address + (i*Machine::PAGE_SIZE));
    }
    // reorganize region array, so it is sorted according to start_address; invalidate the last region by turning it to 0
    for(int i = region_number; i < num_of_region; i++) {
        region_array[i] = region_array[i+1];
    }
    num_of_region--;
}

bool VMPool::is_legitimate(unsigned long _address) {
    if(_address >= base_address && _address < base_address + size) {
        return true;
    }
    return false;
}

