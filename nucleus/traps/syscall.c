//This code is my own work, it was written without consulting code written by other students (Owen Handel)$
#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/procq.e"
#include "../../h/vpop.h"
#include "../../h/asl.e"
#include "../../h/util.h"
#include "../../h/main.e"
#include "../../h/int.e"

proc_t *final;
state_t *temp;
int count = 0;

static void grow_Child(proc_t* child,proc_t* p){//next gen of children
	if(child->sib == (proc_t*) ENULL){//no sibs
        	child->sib = p;
        	p->sib = (proc_t*) ENULL;
	} else if(child->sib != (proc_t*) ENULL){//previous sibs
        	grow_Child(child->sib,p);
	}
}

void growChild(proc_t* parent,proc_t* p){//child proc
	if(parent->first_child == (proc_t*) ENULL){//no child
        	parent->first_child = p;
        	p->sib = (proc_t*) ENULL;
	} else if(parent->first_child != (proc_t*) ENULL){//previous children
        	grow_Child(parent->first_child,p);
	}
    	p->parent = parent;
}

void Create_Process(state_t *oldstate){//system call 1 create proc
            proc_t *new_proc;
            new_proc = allocProc();//allocate new proc
            proc_t *head;
            head = headQueue(runQueue);//pointer to head of runQueue
            if(new_proc == (proc_t *) ENULL){//lack of resources
                oldstate->s_r[2] = ENULL;//return -1 in s_r[2]
            } else if(new_proc != (proc_t *) ENULL){//enough resources
                new_proc->p_s = *(state_t *) oldstate->s_r[4];
                temp = &new_proc->p_s;
                growChild(head, new_proc);//make child
                insertProc(&runQueue, new_proc);//insert proc on runQueue
                oldstate->s_r[2] = 0;//return 0 
            }
        }

proc_t* remChild(proc_t* p){
	if(p == (proc_t*) ENULL || p->first_child == (proc_t*) ENULL){//error
        	return (proc_t*) ENULL;
	} else if(p->first_child != (proc_t*) ENULL){//has at least one child
        	proc_t* rem = p->first_child;
        	p->first_child = p->first_child->sib;//change first child
        	rem->parent = (proc_t*) ENULL;//clean
        	rem->sib =(proc_t*) ENULL;
        	return rem;
    }
}

static void kill_Child(proc_t* child,proc_t* p){
	if(child->sib != (proc_t*) ENULL){//more sib
		if(child->sib == p){//found
			child->sib = child->sib->sib;
        	} else if(child->sib != p){
			kill_Child(child->sib,p);//look harder
        	}
    	}
}

proc_t* killChild(proc_t* p){
	if(p->parent == (proc_t*) ENULL){//is parent
        	return (proc_t*) ENULL;
	}
	if(p->parent->first_child == p){//found link
        	p->parent->first_child = p->parent->first_child->sib;
	} else if(p->parent->first_child != p){//not found link
		kill_Child(p->parent->first_child,p);
    	}
    	p->parent = (proc_t*) ENULL;//clean
    	p->sib = (proc_t*) ENULL;
    	return p;
}

void clearSem(proc_t *first){
	int i;
	for(i=0; i<SEMMAX; i++){
		if(first->semvec[i] != (int *) ENULL){
                	*(first->semvec[i])+=1;//clear
                }
        }
}

void kill(proc_t* killProc){
	proc_t *p;
	while((p = remChild(killProc)) != (proc_t *) ENULL){//
		kill(p);
	}
	int i;
	for(i=0; i<SEMMAX; i++){//is on semaphore queue?
		if(killProc->semvec[i] != (int *) ENULL){
        		clearSem(killProc);;
			outBlocked(killProc);
		}
	}
	killChild(killProc);
	outProc(&runQueue,killProc);//not sem on runqueue
	freeProc(killProc);//free
}

void Terminate_Process(state_t* oldstate){//system call 2
	proc_t *head;
    	head = headQueue(runQueue);
    	final = head;//added final
	kill(head);
    	schedule();
}

void Sem_OP(state_t *oldstate){//set of v and p system call 3
        proc_t *head;//variables
        proc_t *remove;
        vpop *vec;
        int vpop_count;
        int i;
	int prevSem;
	int toSchedule;
        head = headQueue(runQueue);
        vpop_count = oldstate->s_r[3];
        vec = (vpop *)oldstate->s_r[4];//address of vector
	for (i = 0; i < vpop_count; i++){
        	prevSem = *(vec->sem);
        	*(vec->sem) += vec->op;//change op
        	if (vec->op == LOCK){
                	if (prevSem <= 0){
                   		// *(vec->sem) += vec->op;
                    		head->p_s = *oldstate;
                    		insertBlocked(vec->sem,head);
				toSchedule = 1;// need to schedule
			}
            	} else{
			if(prevSem < 0){
                   		// *(vec->sem) += vec->op;
                		proc_t *removed;
                    		removed = removeBlocked(vec->sem);
                    		if (removed != (proc_t *) ENULL && removed->qcount == 0){
                        		insertProc(&runQueue, removed);
                    		}
                                toSchedule = 0;//no schedule cuz no need
			}
            	}
            	vec++;
        }
	if(toSchedule == 1){
		head = removeProc(&runQueue);
		schedule();
	}
}

void notused(state_t *oldstate){//system call 4
	HALT();
}

void Specify_Trap_State_Vector(state_t *oldstate){//system call 5
	int trap_type;
    	proc_t *head;
    	head = headQueue(runQueue);
   	trap_type = oldstate->s_r[2];
    	switch( trap_type ){//determine what to do based on what type of trap
           	case 1:
            		if(head->mm_old == (state_t *) ENULL){//mem management trap
                		head->mm_new = (state_t *) oldstate->s_r[4];
				head->mm_old = (state_t *) oldstate->s_r[3];
            		} else if(head->mm_old != (state_t *) ENULL){
                		Terminate_Process(oldstate);
            		}
            		break;
           	case 2:
            		if(head->sys_old == (state_t *) ENULL){//system trap
                		head->sys_new = (state_t *) oldstate->s_r[4];
				head->sys_old = (state_t *) oldstate->s_r[3];
            		} else if(head->sys_old != (state_t *) ENULL){
                		Terminate_Process(oldstate);
            		}
            		break;
           	default:
            		if(head->prog_old == (state_t *) ENULL){//prog trap
                		head->prog_new = (state_t *) oldstate->s_r[4];
				head->prog_old = (state_t *) oldstate->s_r[3];
            		} else if(head->prog_old != (state_t *) ENULL){
                		Terminate_Process(oldstate);
            		}
            		break;
    	}
}

void Get_CPU_Time(state_t *oldstate){//system call 6
	proc_t *head;
	head = headQueue(runQueue);
	oldstate->s_r[2] = head->time_CPU;
}

