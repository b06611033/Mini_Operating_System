/*
 File: scheduler.C
 
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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "simple_timer.H"

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
/* METHODS FOR CLASS   Q u e u e */
/*--------------------------------------------------------------------------*/
Queue * Queue::head = NULL;

Queue::Queue(Thread * _thread) {
  curr_thread = _thread;
  next  = NULL;
}

void Queue::enqueue(Thread * _thread) {
  Queue * newQ = new Queue(_thread);
  if(head == NULL) {
    head = newQ;
  }
  else {
    Queue * curr = head;
    while(curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = newQ;
  }
}

Thread * Queue::dequeue() {
  if(head == NULL) {
    assert(false);
  }
  Thread * removed = head->curr_thread;
  Queue * old_head = head;
  head = head->next;
  delete old_head;
  return removed;
}


/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  
}

void Scheduler::yield() {
  if(q->head != NULL) {
    if(Machine::interrupts_enabled()) {
      Machine::disable_interrupts();
    }
    Thread * new_thread = Queue::dequeue(); 
    Thread::dispatch_to(new_thread);
    Machine::enable_interrupts();
  } 
}

void Scheduler::resume(Thread * _thread) {
  if(Machine::interrupts_enabled()) {
      Machine::disable_interrupts();
  }
  Queue::enqueue(_thread);
  Machine::enable_interrupts();
}

void Scheduler::add(Thread * _thread) {
  if(Machine::interrupts_enabled()) {
      Machine::disable_interrupts();
  }
  Queue::enqueue(_thread); 
  Machine::enable_interrupts();
}

void Scheduler::terminate(Thread * _thread) {
  if(Machine::interrupts_enabled()) {
      Machine::disable_interrupts();
  }
  _thread->delete_thread();
  Machine::enable_interrupts();
  yield();
}

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   R R S c h e d u l e r  */
/*--------------------------------------------------------------------------*/
RRScheduler::RRScheduler(EOQTimer * _eoqt) {
  eoqt = _eoqt;
}

void RRScheduler::yield() {
  if(q->head != NULL) {
    if(Machine::interrupts_enabled()) {
      Machine::disable_interrupts();
    }
    eoqt->ticks = 0;  // sets tick to 0, so the next thread will start from tick 0
    Thread * new_thread = Queue::dequeue(); 
    Thread::dispatch_to(new_thread);
    Machine::enable_interrupts();
  } 
}