//THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING CODE WRITTEN BY OTHER STUDENTS-OWEN HANDEL 
#include "../../h/const.h"
#include "../../h/vpop.h"
#include "../../h/util.h"
#include "../../h/support.h"
#include "../../h/procq.e"
#include "../../h/asl.e"
#include "../../h/main.e"
#include "../../h/int.e"
#include "../../h/trap.e"
#include "../../h/page.e"
#include "../../h/support.e"
#include "../../h/slsyscall2.e"

register int r2 asm("%d2");
register int r3 asm("%d3");
register int r4 asm("%d4");

int tod_count = 0;

void Read_from_Terminal(state_t *t_old, int dnum){
    	char *buf_add;
    	char *virtual_add;
    	buf_add = tmm[dnum].buf;
    	devReg[dnum]->d_badd = buf_add;
    	virtual_add = (char *)t_old->s_r[3];
	devReg[dnum]->d_op = IOREAD;
    	r4 = dnum;
    	SYS8();
	int i;
    	if(devReg[dnum]->d_stat == NORMAL || devReg[dnum]->d_stat == ENDOFINPUT){
        	int amnt = devReg[dnum]->d_amnt;
        	if(amnt == 0){
       			t_old->s_r[2] = -ENDOFINPUT;
        	} else if(amnt != 0){
            		i = 0;
            		while(i < amnt){
                		*(virtual_add+i) = tmm[dnum].buf[i];
                		i++;
            		}
            		t_old->s_r[2] = amnt;
        	}
    	} else if(devReg[dnum]->d_stat != NORMAL || devReg[dnum]->d_stat != ENDOFINPUT){
        	t_old->s_r[2] = -devReg[dnum]->d_stat;
    	}
}

void Write_to_Terminal(state_t *t_old, int dnum){
    	int i;
    	int len;
    	char *buf_add;
    	char *virtual_add;
    	len = (int)t_old->s_r[4];
    	buf_add = tmm[dnum].buf;
    	virtual_add = (char *)t_old->s_r[3];
    	i = 0;
	while(i < len){
		tmm[dnum].buf[i] = *(virtual_add+i);
		i++;
	}
	devReg[dnum]->d_amnt = len;
	devReg[dnum]->d_badd = buf_add;
	devReg[dnum]->d_op = IOWRITE;
	r4 = dnum;
    	SYS8();
    	if(devReg[dnum]->d_stat == NORMAL){
        	int amnt = devReg[dnum]->d_amnt;
        	if(amnt <= 0){
            		t_old->s_r[2] = -ENDOFINPUT;
        	} else if(amnt > 0){
            		t_old->s_r[2] = amnt;
        	}
    	} else{
		t_old->s_r[2] = -devReg[dnum]->d_stat;
	}
}

void Delay(state_t *t_old, int dnum){
    	int delay_ug;
	int per;
	long check;
    	vpop semop_c[2];
    	vpop semop_d;
    	delay_ug = t_old->s_r[4];
	t_dly[dnum].delay = delay_ug;
    	STCK(&(t_dly[dnum].start));
        r3 = 2;
        r4 = (int)&semop_c;
	switch( dnum ){
		case 0:
			semop_c[0].op = LOCK;
        		semop_c[0].sem = &(t_dly[dnum].sem);
			semop_c[1].op = UNLOCK;
    			semop_c[1].sem = &csem;
			break;
		case 1:
			semop_c[1].op = LOCK;
                        semop_c[1].sem = &(t_dly[dnum].sem);
                        semop_c[0].op = UNLOCK;
                        semop_c[0].sem = &csem; 
                        break;
		default:
			break;
	}
    	SYS3();
}

void Get_Time_of_Day(state_t *t_old){
    	long time;
    	STCK(&time);
    	t_old->s_r[2] = time;
	tod_count++;
}

void Terminate(state_t *t_old, int dnum){
    	vpop semop;
    	p_alive--;
    	putframe(dnum);
    	if(p_alive == 0){
        	vpop semop;
        	semop.op = UNLOCK;
        	semop.sem = &csem;
        	r3 = 1;
        	r4 = (int)&semop;
        	SYS3();
    	}
	SYS2();
}


