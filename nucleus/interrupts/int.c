//This code is my own work, it was written without consulting code written by other students (Owen Handel)$
#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/vpop.h"
#include "../../h/util.h"
#include "../../h/procq.e"
#include "../../h/asl.e"
#include "../../h/int.e"
#include "../../h/main.e"
#include "../../h/trap.e"
#define PRINT0ADDR      (devreg_t *)0x1450
devreg_t *p_dr = PRINT0ADDR;
state_t *oldterminal;
state_t *oldprinter;
state_t *olddisk;
state_t *oldfloppy;
state_t *oldclock;
int pseudo_clock;
int amnt[15];
int stat[15];
devreg_t *devReg[15];
int devSem[15];
long timeSlice = 500;
int t_count;
int test = 0;
int maybe = 0;
char normal[] = "Last process terminated->shutdown ";
char deadlock[] = "Deadlock detected->shutdown ";
void static unlockOP();
void static lockOP();
void static handleInt();
void static devBlocked();
void static intterminalhandler();
void static intprinterhandler();
void static intdiskhandler();
void static intfloppyhandler();
void static intclockhandler();
void static sleepy();

void static sleepy(){
    asm("       stop    #0x2000");
}

void intinit(){
	state_t *new_terminal_area;//new areas
    	state_t *new_printer_area;
    	state_t *new_disk_area;
    	state_t *new_floppy_area;
    	state_t *new_clock_area;
    	oldterminal = (state_t *) 0x9c8; //address of old
    	new_terminal_area = (state_t *) 0x9c8+1; //address for new area
    	STST(new_terminal_area);
    	new_terminal_area->s_pc = (int)intterminalhandler;//pass to handler
    	new_terminal_area->s_sr.ps_int = 7; //set mask
    	oldprinter = (state_t *) 0xa60; //addr
    	new_printer_area = (state_t *) 0xa60+1; //addrnew
    	STST(new_printer_area);
    	new_printer_area->s_pc = (int)intprinterhandler;//pass
    	new_printer_area->s_sr.ps_int = 7; //mask
    	olddisk = (state_t *) 0xaf8; //addr
    	new_disk_area = (state_t *) 0xaf8+1; //addrnew
    	STST(new_disk_area);
    	new_disk_area->s_pc = (int)intdiskhandler; //pass
    	new_disk_area->s_sr.ps_int = 7; //mask
    	oldfloppy = (state_t *) 0xb90; //addr
    	new_floppy_area = (state_t *) 0xb90+1; //addrnew
    	STST(new_floppy_area);
    	new_floppy_area->s_pc = (int)intfloppyhandler; //pass
    	new_floppy_area->s_sr.ps_int = 7; //mask
    	oldclock = (state_t *) 0xc28; //addr
    	new_clock_area = (state_t *) 0xc28+1; //addrnew
    	STST(new_clock_area);
    	new_clock_area->s_pc = (int)intclockhandler;
    	new_clock_area->s_sr.ps_int = 7; //mask
    	pseudo_clock = 0;
    	t_count = 100000 / timeSlice;
	int i = 0;
    	while( i < 15 ){//loop to set addr
        	devSem[i] = 0;
		devReg[i] = (devreg_t *)(0x1400+0x10*i);
    		i++;//start at terminal(1400),printer(1450),etc
	}
}

void waitforpclock(state_t *oldstate){
	proc_t *head;
    	head = headQueue(runQueue);
    	head->p_s = *oldstate;
    	lockOP(&pseudo_clock);//p operation on pseudoclock
}

void waitforio(state_t *oldstate){
    	proc_t *head;
    	int devNumber;
    	devNumber = oldstate->s_r[4]; //IO device number located in D4
    	if(devSem[devNumber] == UNLOCK){
        	devSem[devNumber] += LOCK;
        	oldstate->s_r[3] = stat[devNumber];//status
        	oldstate->s_r[2] = amnt[devNumber];//length
    	} else if(devSem[devNumber] != UNLOCK){
        	head = headQueue(runQueue);
        	head->p_s = *oldstate;
        	lockOP(&(devSem[devNumber]));//p operation
    	}
}

void print_0(char *text, int length){
    	p_dr->d_badd = text; //device registers
    	p_dr->d_amnt = length;
    	p_dr->d_stat = ENULL;
    	p_dr->d_op = IOWRITE;
    	while (devReg[5]->d_stat != 0){
	}//wait for IO
}

void intdeadlock(){//deadlock stats/normal termination
    	semd_t *check;
	semd_t *next;
	int i = 0;
    	int isempty = TRUE;
    	while(i < 15){
        	if(headBlocked(&devSem[i]) != (proc_t *) ENULL){
            		isempty = FALSE;//not empty if entry exists associated with the semadd
		}
		i++;
	}
	if(headBlocked(&pseudo_clock) != (proc_t *) ENULL){
        	intschedule();
        	sleepy();
    	} else if(isempty == FALSE){ //still not empty
        	sleepy();
    	} else if(headASL() == FALSE){//none left on ASL
        	print_0(normal, 34);
        	HALT();
    	} else if(headASL() == TRUE){
		check = findASL();
		if(check->s_next == (semd_t *) ENULL){
			cleanASL(check);
		} else{
			while(next->s_next != (semd_t *) ENULL){
				next = check->s_next;
				cleanASL(check);
				check = next;
			}
		}
		if(headASL() == FALSE){
			print_0(normal, 34);
                	HALT();
		}
	} else{
		print_0(deadlock,27);//deadlocked state
        	HALT();
    	}
}

void intschedule(){
    	LDIT(&timeSlice);
}

void static lockOP(int *sem){//p operation similar to Sem_OP in trap
    	proc_t *rem;
    	int prev;
    	prev = *(sem);
    	*(sem) += LOCK;//-1
    	if(prev <= 0){
        	rem = removeProc(&runQueue);
        	if(rem != (proc_t *) ENULL){
            		insertBlocked(sem,rem);
        	}
    		schedule();
    	}
}

void static unlockOP(int *sem){//v operation similar to Sem_OP in trap
    	proc_t *rem;
    	int prev;
    	prev = *(sem);
    	*(sem) += UNLOCK;//1
    	if(prev < 0){
        	rem = removeBlocked(sem);
        	if(rem != (proc_t *) ENULL && rem->qcount == 0){
            		insertProc(&runQueue, rem);
        	}
    	}
}

void static handleInt(int devType, int devNumber){
    switch( devType ){//determine what to do based on what type of int
        case 0:
            if(devSem[devNumber] == -1){ //offset 0 term starts at 0
                devBlocked(devNumber);
            } else if(devSem[devNumber] != -1){
                devSem[devNumber] += 1;
                stat[devNumber] = devReg[devNumber]->d_stat;
                amnt[devNumber] = devReg[devNumber]->d_amnt;
            }
            break;
	case 1:
            if(devSem[devNumber+5] == -1){ //offset 5 print starts at 5
                devBlocked(devNumber+5);
            } else if(devSem[devNumber+5] != -1){
                devSem[devNumber+5] += 1;
                stat[devNumber+5] = devReg[devNumber+5]->d_stat;
                amnt[devNumber+5] = devReg[devNumber+5]->d_amnt;
            }
            break;
        case 2:
            devNumber;
            if(devSem[devNumber+7] == -1){ //offset 7 disk starts at 7
                devBlocked(devNumber+7);
            } else if(devSem[devNumber+7] != -1){
                devSem[devNumber+7] += 1;
                stat[devNumber+7] = devReg[devNumber+7]->d_stat;
                amnt[devNumber+7] = devReg[devNumber+7]->d_amnt;
            }
            break;
        case 3:
            devNumber;
            if(devSem[devNumber+11] == -1){//offset 11 floppy starts at 11
               devBlocked(devNumber+11);
            } else if(devSem[devNumber+11] != -1){
                devSem[devNumber+11] += 1;
                stat[devNumber+11] = devReg[devNumber+11]->d_stat;
                amnt[devNumber+11] = devReg[devNumber+11]->d_amnt;
            }
            break;
        default:
            break;
    }
}

void static devBlocked(int devNumber){//saving lines
    	proc_t *tail;
    	proc_t *head;
	proc_t *rem;
    	int prev;
	head = headQueue(runQueue);
	tail = headBlocked(&(devSem[devNumber]));
	tail->p_s.s_r[3] = devReg[devNumber]->d_stat;
    	tail->p_s.s_r[2] = devReg[devNumber]->d_amnt;
    	unlockOP(&devSem[devNumber]);
}

void static intterminalhandler(){//handler for terminal
	proc_t *head;
	head = headQueue(runQueue);
	handleInt(0,oldterminal->s_tmp.tmp_int.in_dno);
	if(head == (proc_t *) ENULL){
		schedule();
	} else if(head != (proc_t *) ENULL){
		LDST(oldterminal);
	}
}

void static intprinterhandler(){//handler for printer
	proc_t *head;
	head = headQueue(runQueue);
	handleInt(1,oldprinter->s_tmp.tmp_int.in_dno);
	if (head == (proc_t *) ENULL){
		schedule();
	} else if(head != (proc_t *) ENULL){ 
		LDST(oldprinter);
	}
}

void static intdiskhandler(){//handler for disk
	proc_t *head;
	head = headQueue(runQueue);
	handleInt(2,olddisk->s_tmp.tmp_int.in_dno);
	if(head == (proc_t *) ENULL){
		schedule();
	} else if(head != (proc_t *) ENULL){ 
		LDST(olddisk);
	}
}

void static intfloppyhandler(){//handler for floppy
	proc_t *head;
	head = headQueue(runQueue);
	handleInt(3,oldfloppy->s_tmp.tmp_int.in_dno);
	if(head == (proc_t *) ENULL){
		schedule();
	} else{
		LDST(oldfloppy);
	}
}

void static intclockhandler(){//handler for clock
	proc_t *head;
	head = headQueue(runQueue);
	t_count--;
	if(head != (proc_t *) ENULL){
                head = removeProc(&runQueue);
                calctime(head);
                head->p_s = *oldclock;
                insertProc(&runQueue, head);
        }
	if(t_count == 0){
                t_count = 100000 / timeSlice;
		if(headBlocked(&pseudo_clock) != (proc_t *) ENULL){
                        unlockOP(&pseudo_clock);
                }
        }

	schedule();
}


