//This code is my own work, it was written without consulting code written by other students (Owen Handel)$
#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/procq.e"
#include "../../h/vpop.h"
#include "../../h/asl.e"
#include "../../h/util.h"
#include "../../h/syscall.e"
#include "../../h/main.e"
#include "../../h/int.e"

state_t *old_sys;
state_t *old_mm;
state_t *old_prog;

void update_time(proc_t *_proc){//update proc time
	long time;
	STCK(&time);
	_proc->time_start = time;
}

void calctime(proc_t *_proc){//calc time
	long time_current;
	STCK(&time_current);
	int diff = (int)time_current - (_proc->time_start);//difference in proc start time and current time
	_proc->time_CPU += diff; //add to cpu time 
}

void pass_up(state_t *oldstate, int trap_type){//trap pass up function for differing types
	proc_t *head;
	head = headQueue(runQueue);
	if (head == (proc_t *) ENULL){

	}
	switch ( trap_type ){
		case 1:
                	if(head->mm_new != (state_t *) ENULL){//mem management 
                        	*head->mm_old = *oldstate;
                        	update_time(head);
                        	LDST(head->mm_new);
                	} else if(head->mm_new == (state_t *) ENULL) {
                        	Terminate_Process(oldstate);
                	}
			break;
		case 2:
			if (head->sys_new != (state_t *) ENULL){//system
				*head->sys_old = *oldstate;
				update_time(head);
				LDST(head->sys_new);
			} else if(head->sys_new == (state_t *) ENULL){
				Terminate_Process(oldstate);
			}
			break;
		default:
			if (head->prog_new != (state_t *) ENULL){//prog
				*head->prog_old = *oldstate;
				update_time(head);
				LDST(head->prog_new);
			} else if(head->prog_new == (state_t *) ENULL){
				Terminate_Process(oldstate);
			}
		}
	}

int isPriv(state_t* oldstate){//determine the status priv/or not priv
	if(oldstate->s_sr.ps_s != 1 && oldstate->s_tmp.tmp_sys.sys_no < 9){
		return 1;
	}
	return 0;
}

void static trapsyshandler(){
	proc_t *head;
	int sys_type;
	head = headQueue(runQueue);
	calctime(head);
	sys_type = (int) old_sys->s_tmp.tmp_sys.sys_no;
	if (isPriv(old_sys) == 1){
		old_sys->s_tmp.tmp_pr.pr_typ = PRIVILEGE;
		pass_up(old_sys,0);
	}
	switch ( sys_type ) {//system calls that apply for system trap
		case 1:
			Create_Process(old_sys);//sys1
			break;
		case 2:
			Terminate_Process(old_sys);//sys2
			break;
		case 3:
			Sem_OP(old_sys);//sys3
			break;
		case 4:
			notused(old_sys);//sys4
			break;
		case 5:
			Specify_Trap_State_Vector(old_sys);//sys5
			break;
		case 6:
			Get_CPU_Time(old_sys);//sys6
			break;
		case 7:
			waitforpclock(old_sys);//sys7 not implemented yet int.c
			break;
		case 8:
			waitforio(old_sys);//sys 8 not implemented yet int.c
			break;
		default:
			pass_up(old_sys,2);//default case pass up trap
			break;

	}
	update_time(head);
	LDST(old_sys);

}

void static trapmmhandler(){//mem management
	proc_t *head;
	head = headQueue(runQueue);
	calctime(head);
	pass_up(old_mm, 1);

}

void static trapproghandler(){//prog
	proc_t *head;
	head = headQueue(runQueue);
	calctime(head);
	pass_up(old_prog, 0);
}

void trapinit(){
	*(int *)0x008 = (int)STLDMM;//EVT
	*(int *)0x00c = (int)STLDADDRESS;
	*(int *)0x010 = (int)STLDILLEGAL;
	*(int *)0x014 = (int)STLDZERO;
	*(int *)0x020 = (int)STLDPRIVILEGE;
	*(int *)0x08c = (int)STLDSYS;
	*(int *)0x94 = (int)STLDSYS9;
	*(int *)0x98 = (int)STLDSYS10;
	*(int *)0x9c = (int)STLDSYS11;
	*(int *)0xa0 = (int)STLDSYS12;
	*(int *)0xa4 = (int)STLDSYS13;
	*(int *)0xa8 = (int)STLDSYS14;
	*(int *)0xac = (int)STLDSYS15;
	*(int *)0xb0 = (int)STLDSYS16;
	*(int *)0xb4 = (int)STLDSYS17;
	*(int *)0x100 = (int)STLDTERM0;
	*(int *)0x104 = (int)STLDTERM1;
	*(int *)0x108 = (int)STLDTERM2;
	*(int *)0x10c = (int)STLDTERM3;
	*(int *)0x110 = (int)STLDTERM4;
	*(int *)0x114 = (int)STLDPRINT0;
	*(int *)0x11c = (int)STLDDISK0;
	*(int *)0x12c = (int)STLDFLOPPY0;
	*(int *)0x140 = (int)STLDCLOCK;
	state_t *new_prog_area;//new areas
        state_t *new_mm_area;
        state_t *new_sys_area;
        old_prog = (state_t *)0x800;//loading old and new
        new_prog_area = (state_t *)0x800+1;
        STST(new_prog_area);//save new
        new_prog_area->s_pc = (int)trapproghandler;
        old_mm = (state_t *)0x898;//same
        new_mm_area = (state_t *)0x898+1;
        STST(new_mm_area);
        new_mm_area->s_pc = (int)trapmmhandler;
        old_sys = (state_t *)0x930;//same
        new_sys_area = (state_t *)0x930+1;
        STST(new_sys_area);
        new_sys_area->s_pc = (int)trapsyshandler;
}
