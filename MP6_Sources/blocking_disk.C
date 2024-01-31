/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "thread.H"
extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
  disk_id   = _disk_id;
  disk_size = _size;
}

void BlockingDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {

  Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
  Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
  Machine::outportb(0x1F3, (unsigned char)_block_no);
                         /* send low 8 bits of block number */
  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
                         /* send next 8 bits of block number */
  unsigned int disk_no = disk_id == DISK_ID::MASTER ? 0 : 1;
  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (disk_no << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

  Machine::outportb(0x1F7, (_op == DISK_OPERATION::READ) ? 0x20 : 0x30);

}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  issue_operation(DISK_OPERATION::READ, _block_no);

  while(!is_ready()) {
    Thread::preempt();  // if not ready to transfer data, we preempt the current thread
  }
  /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = Machine::inportw(0x1F0);
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  } 

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  issue_operation(DISK_OPERATION::WRITE, _block_no);

  while(!is_ready()) {
    Thread::preempt(); // if not ready to transfer data, we preempt the current thread
  }
  /* write data to port */
  int i; 
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }
}
