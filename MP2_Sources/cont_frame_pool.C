/*
 File: ContFramePool.C
 
 Author: Po Han Hou
 Date  : 2023/9/15
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/


ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no) {
    // 1 byte has 8 bit, a single index in bitmap is 1 byte (a single index represents info of 4 frames, since 2 bits represents 1 frame)
    unsigned int bitmap_index = _frame_no / 4;  // locate the byte
    unsigned char mask = 0x3;
    unsigned char frameBits =  bitmap[bitmap_index] >> ((_frame_no % 4)*2); // shift the 2 bits representing the frame to rightmost
    // 00 means free, 01 means used, 11 means HoS
    switch(frameBits & mask) {
    case 0x0: // free
        return FrameState::Free;
    case 0x1: // used
        return FrameState::Used;
    case 0x3: // HoS
        return FrameState::HoS;
    default:
        return FrameState::Used;
    }  
}


void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) {
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char mask = 0x3 << ((_frame_no % 4)*2);
    bitmap[bitmap_index] |= mask; // first convert the 2 bits for the frame to 11
    unsigned char mask2;
    switch(_state) {
    case FrameState::Free:  //00
      bitmap[bitmap_index] ^= mask; 
      break;
    case FrameState::Used: // 01
      mask2 = 0x2 << ((_frame_no % 4)*2);
      bitmap[bitmap_index] ^= mask2; 
      break;
    case FrameState::HoS:  // 11 
      break;
    }
}



//Constructor
ContFramePool * ContFramePool::head = NULL;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    // Everything ok. Proceed to mark all frame as free.
    for(int fno = 0; fno < _n_frames; fno++) {
        set_state(fno, FrameState::Free);
    }
    
    unsigned long info_frames_needed = needed_info_frames(_n_frames);
    // Mark the first few info frames as being used if it is being used
    if(_info_frame_no == 0) {
        set_state(0, FrameState::HoS);
        nFreeFrames--;
        for(int fno = 1; fno < info_frames_needed; fno++) {
            set_state(fno, FrameState::Used);
            nFreeFrames--;
        }
    }
    
    if(head == NULL) {
        head = this;
    }
    else {
        ContFramePool * curr = head;
        while(curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = this;
    }
    next = NULL;
    Console::puts("Frame Pool initialized\n");
}



unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
     // Check if there are enough frames to allocate
    if(nFreeFrames < _n_frames) {
        return 0;
    }
    
    // Find a sequence of frame that is not being used and return the first frame index.
    unsigned int frame_no = 0;
    unsigned int count = 0;
    unsigned int found = 0;
    for(int i = 0; i < nframes; i++) {
        if(get_state(i) == FrameState::Free) {
            count++;
            if(count == _n_frames) {
                found = 1;
                break;
            }
        }
        else {
            frame_no = i+1;
            count = 0;
        }
    }
    
    if(found == 1) {
        set_state(frame_no, FrameState::HoS);
        for(int i = frame_no + 1; i < frame_no + _n_frames; i++) {
            set_state(i, FrameState::Used);
        }
        nFreeFrames = nFreeFrames - _n_frames;
        return (frame_no + base_frame_no);
    }
    else {
        return 0;
    }
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    set_state(_base_frame_no - base_frame_no, FrameState::HoS);
    nFreeFrames--;
    for(int i = 1; i < _n_frames; i++) {
        set_state(_base_frame_no - base_frame_no + i, FrameState::Used);
        nFreeFrames--;
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // find which pool the sequence belongs to
    ContFramePool *curr_pool = head;
    while(curr_pool != NULL) {
        if(_first_frame_no >= curr_pool->base_frame_no && _first_frame_no < curr_pool->base_frame_no + curr_pool->nframes) {
            break;
        }
        curr_pool = curr_pool->next;
    }
    // release sequence from that pool
   if(curr_pool->get_state(_first_frame_no - curr_pool->base_frame_no) != FrameState::HoS) {
        return;
   } 
   else {
        curr_pool->set_state(_first_frame_no - curr_pool->base_frame_no, FrameState::Free);
        curr_pool->nFreeFrames++;
        unsigned int index = _first_frame_no - curr_pool->base_frame_no + 1;
        while(index < curr_pool->base_frame_no + curr_pool->nframes && curr_pool->get_state(index) == FrameState::Used) {
            curr_pool->set_state(index, FrameState::Free);
            curr_pool->nFreeFrames++;
            index++;
        }
   }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    return _n_frames/16384 + (_n_frames % 16384 > 0 ? 1 : 0);
}
