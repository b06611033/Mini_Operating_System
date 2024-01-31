#include "simple_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

SimpleFramePool::FrameState SimpleFramePool::get_state(unsigned long _frame_no) {
    // 1 byte has 8 bit, a single index in bitmap is 1 byte (a single index represents info 8 frames, since 1 bit represents 1 frame)
    unsigned int bitmap_index = _frame_no / 8;  // locate the byte
    unsigned char mask = 0x1 << (_frame_no % 8); // create a mask that has value 1 at the same location of the bit for the frame (other bits in mask are 0)
    // 0 means used, 1 means free
    return ((bitmap[bitmap_index] & mask) == 0) ? FrameState::Used : FrameState::Free; // AND the mask with the byte to determine the bit value for the frame
    
}

void SimpleFramePool::set_state(unsigned long _frame_no, FrameState _state) {
    unsigned int bitmap_index = _frame_no / 8;
    unsigned char mask = 0x1 << (_frame_no % 8);

    switch(_state) {
      case FrameState::Used:  // need to change to used, which is bit 0
      bitmap[bitmap_index] ^= mask; // current value for the bit is 1, the masked bit is 1, 1 XOR 1 = 0
      break;
    case FrameState::Free: // need to change to free, which is bit 1
      bitmap[bitmap_index] |= mask; // current value for the bit is 0, the masked bit is 1, 0 OR 1 = 1
      break;
    }
    // Other bit stay the same, since the mask of all other bits is also 0, no matter XOR or OR with 0 doesn't change the original bit value
}

SimpleFramePool::SimpleFramePool(unsigned long _base_frame_no,
                                 unsigned long _nframes,
                                 unsigned long _info_frame_no)
{
    // Bitmap must fit in a single frame!
    assert(_nframes <= FRAME_SIZE * 8); // 1 byte has 8 bits
    
    base_frame_no = _base_frame_no;
    nframes = _nframes;
    nFreeFrames = _nframes;
    info_frame_no = _info_frame_no;
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    // Everything ok. Proceed to mark all frame as free.
    for(int fno = 0; fno < _nframes; fno++) {
        set_state(fno, FrameState::Free);
    }
    
    // Mark the first frame as being used if it is being used
    if(_info_frame_no == 0) {
        set_state(0, FrameState::Used);
        nFreeFrames--;
    }
    
    Console::puts("Frame Pool initialized\n");
}

unsigned long SimpleFramePool::get_frame()
{
    
    // Any frames left to allocate?
    assert(nFreeFrames > 0);
    
    // Find a frame that is not being used and return its frame index.
    // Mark that frame as being used in the bitmap.
    unsigned int frame_no = 0;
    
    while(get_state(frame_no) == FrameState::Used) {
        // This of course can be optimized!
    	frame_no++;
    }
    
    // We don't need to check whether we overrun. This is handled by assert(nFreeFrame>0) above.
    set_state(frame_no, FrameState::Used);
    nFreeFrames--;
    
    return (frame_no + base_frame_no);
}

void SimpleFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                        unsigned long _nframes)
{
    // Mark all frames in the range as being used.
    for(int fno = _base_frame_no; fno < _base_frame_no + _nframes; fno++){
        set_state(fno - this->base_frame_no, FrameState::Used);
    }
}



void SimpleFramePool::release_frame(unsigned long _frame_no)
{
    // -- WE LEAVE THE IMPLEMENTATION OF THIS FUNCTION TO YOU.
    //    NOTE: Keep in mind that you first need to identify the correct frame pool.
    //    The function is static, and you are only given a frame number. You do have
    //    to figure out which frame pool this frame belongs to before you can call the
    //    frame-pool-specific release_frame function.
    
    
#ifdef JUST_AS_EXAMPLE
    // Inside the frame-pool specific release_frame function we mark the frame
    // as released as follows:
    
    // The frame better be used before we release it.
    assert(get_state(_frame_no - base_frame_no) == FrameState::Used);
    
    set_state(_frame_no - base_frame_no, FrameState::Free);
    
    nFreeFrames++;
#endif
}

