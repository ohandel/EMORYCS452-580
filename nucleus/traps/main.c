//This code is my own work, it was written without consulting code written by other students (Owen Handel)$
#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/procq.e"
#include "../../h/vpop.h"
#include "../../h/int.e"
#include "../../h/util.h"
#include "../../h/trap.e"
#include "../../h/asl.e"

extern int p1();
state_t initial;
proc_link runQueue;

void static init(){
    	STST(&initial);//save initial state
    	initProc();//call init stuff
    	initSemd();
    	trapinit();
    	intinit();
}

void schedule(){
    	proc_t *head;
    	head = headQueue(runQueue);
    	if (head == (proc_t *)ENULL){
        	intdeadlock();
    	} else if(head != (proc_t *) ENULL){
        	intschedule();
        	head = headQueue(runQueue);
        	update_time(head);
        	LDST(&(head->p_s));
    	}
}

void main(){
    	init();//call init
    	int stack;
	proc_t *_proc = allocProc();
    	STST(&_proc->p_s);
    	stack = initial.s_sp;//setting up proc state
   	if (stack % 2 == 1) {
        	stack = stack - 1;
    	}
    	stack = stack - 512;
    	_proc->p_s.s_sp = stack;
	_proc->p_s.s_pc = (int)p1;
    	runQueue.index = ENULL;
    	runQueue.next = (proc_t *)ENULL;
    	insertProc(&runQueue, _proc);//placing p1 on runQueue
    	schedule();
}

