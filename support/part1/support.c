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
#include "../../h/slsyscall1.e"
#include "../../h/slsyscall2.e"

#define ENDDEVREG       0x1600
#define BOOTPC		0x80000+31*PAGESIZE;
#define BOOTSP		0x80000+32*PAGESIZE-4;

register int r2 asm("%d2");
register int r3 asm("%d3");
register int r4 asm("%d4");

int bootcode[] = {
0x41f90008,
0x00002608,
0x4e454a82,
0x6c000008,
0x4ef90008,
0x0000d1c2,
0x787fb882,
0x6d000008,
0x10bc000a,
0x52486000,
0xffde4e71
};

int boot_len = 11;
int csem;
t_mmtable tmm[2];
t_delay t_dly[2];
t_statearea t_area[2];
sd_t p1aseg_tble[32];
pd_t p1aseg_pgtble[256];
pd_t share_tble[32];

extern int start();
extern int endt0();
extern int startt1();
extern int etext();
extern int startd0();
extern int endd0();
extern int startd1();
extern int edata();
extern int startb0();
extern int endb0();
extern int startb1();
extern int end();

void static p1a();
void static tprocess();
void static slmmhandler();
void static slsyshandler();
void static slproghandler();
void static cron();


p1(){
	pageinit();
	int boot;
	int i = 1;
	int j;
	p1aseg_tble[0].sd_p = 1;
	p1aseg_tble[0].sd_len = 255; //one less than 256(max valid) //presence bit set at 1
	p1aseg_tble[0].sd_prot = 7; //rwe
	p1aseg_tble[0].sd_pta = &p1aseg_pgtble[0]; //addr of page table
	while(i < 32){
		p1aseg_tble[i].sd_p = 0; //set presence bits to 0
		i++;
	}
	int t1 = (int)startt1/512;
	int etxt = (int)etext/512;
	int d1 = (int)startd1/512;
	int edta = (int)edata/512;
	int b1 = (int)startb1/512;
	int ends = (int)end/512;
	int dev_end = ENDDEVREG/512;
	int dev_begin = BEGINDEVREG/512;
	i = 0;
	while(i < 256){
		if(i == 2){
			p1aseg_pgtble[i].pd_r = 1;
			p1aseg_pgtble[i].pd_p = 1;
			p1aseg_pgtble[i].pd_frame = i;
		} else if(i <= etxt && i >= t1){
			p1aseg_pgtble[i].pd_r = 1;
                        p1aseg_pgtble[i].pd_p = 1;
                        p1aseg_pgtble[i].pd_frame = i;
		} else if(i <= edta && i >= d1){
			p1aseg_pgtble[i].pd_r = 1;
                        p1aseg_pgtble[i].pd_p = 1;
                        p1aseg_pgtble[i].pd_frame = i;
		} else if(i <= ends && i >= b1){
			p1aseg_pgtble[i].pd_r = 1;
                        p1aseg_pgtble[i].pd_p = 1;
                        p1aseg_pgtble[i].pd_frame = i;
		} else if(i == Scronframe){
			p1aseg_pgtble[i].pd_r = 1;
                        p1aseg_pgtble[i].pd_p = 1;
                        p1aseg_pgtble[i].pd_frame = i;
		} else{
			p1aseg_pgtble[i].pd_p = 0; //presence bit 0 will invoke mmtrap
                        p1aseg_pgtble[i].pd_r = 1;
                        p1aseg_pgtble[i].pd_frame = i;
		}
		i++;
	}
	i = 0;
	while(i < 2){ //for two terminals
		t_dly[i].sem = 0;
		t_dly[i].delay = 0;
		t_dly[i].start = 0;
		tmm[i].segs[0].sd_p = 0; //safety first
                tmm[i].segs[1].sd_p = 1; //presence bit for unique seg
                tmm[i].segs[2].sd_p = 1; //presence bit for shared seg
		tmm[i].segs[1].sd_pta = &(tmm[i].page_tbl[0]);
                tmm[i].segs[2].sd_pta = &(share_tble[0]);
		j = 1;
		while(j < 3){
                        tmm[i].segs[j].sd_len = 31;
                        tmm[i].segs[j].sd_prot = 7;
                        j++;
                }
		j = 0;
		while(j < 3){
			tmm[i].segs_priv[j].sd_p = 1;
			tmm[i].segs_priv[j].sd_prot = 7;
			j++;
		}
		tmm[i].segs_priv[0].sd_len = 255;
		tmm[i].segs_priv[1].sd_len = 31;
		tmm[i].segs_priv[2].sd_len = 31;
		tmm[i].segs_priv[0].sd_pta = &(tmm[i].page_priv[0]);
		tmm[i].segs_priv[1].sd_pta = &(tmm[i].page_tbl[0]);
		tmm[i].segs_priv[2].sd_pta = &(share_tble[0]);
		j = 3;
		while(j < 32){
			tmm[i].segs[j].sd_p = 0;
			tmm[i].segs_priv[j].sd_p = 0;
			j++;
		}
		j = 0;
		while(j < 32){
			tmm[i].page_tbl[j].pd_r = 1;
			share_tble[j].pd_r = 1;
			tmm[i].page_tbl[j].pd_p = 0;
			share_tble[j].pd_p = 0;
			j++;
		}
		j = 0;
		while(j < 256){
			if (j == 2){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else if (j <= dev_end && j >= dev_begin){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else if (j <= etxt && j >= t1){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else if (j <= edta && j >= d1){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else if (j <= ends && j >= b1){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else if (j == Tsysframe[i]){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else if (j == Tmmframe[i]){
    				tmm[i].page_priv[j].pd_p = 1;
    				tmm[i].page_priv[j].pd_r = 1;
    				tmm[i].page_priv[j].pd_frame = j;
			}else{
    				tmm[i].page_priv[j].pd_p = 0;
    				tmm[i].page_priv[j].pd_r = 1;
  	  			tmm[i].page_priv[j].pd_frame = j;
			}
			j++;
		}
		i++;
	}
	csem = 0;
	sem_mm = 1;
	i = 0;
	j = 0;
	while(i < 2){
		boot = getfreeframe(i, 31, 1);
		for(j = 0; j < boot_len; j++){
			*(((int *)(boot*512))+j) = bootcode[j];
		}
		tmm[i].page_tbl[31].pd_frame = boot;
		tmm[i].page_tbl[31].pd_p = 1;
		i++;
	}
	p_alive = 2;
	state_t p1_a;
 	STST(&p1_a);
	p1_a.s_pc = (int)p1a;
	p1_a.s_sr.ps_m = 1;
	p1_a.s_sr.ps_int = 0;
	p1_a.s_sp = Scronstack;
	p1_a.s_crp = &p1aseg_tble[0];
	LDST(&p1_a);
}

void static p1a(){
	int i = 0;
	state_t t_proc;
	STST(&t_proc);
	while(i < 2){
		t_proc.s_pc = (int)tprocess;
		t_proc.s_sp = Tsysstack[i];
		t_proc.s_crp = &tmm[i].segs_priv[0];
		t_proc.s_r[12] = i;
		r4 = (int)&t_proc;
		SYS1();
		i++;
	}
	cron();
}

void static tprocess(){
	state_t boot_state;
	int index;
	STST(&boot_state);
	index = boot_state.s_r[12];
	STST(&t_area[index].t_sys_new);
	t_area[index].t_sys_new.s_pc = (int)slsyshandler;
	t_area[index].t_sys_new.s_sp = Tsysstack[index];
	t_area[index].t_sys_new.s_crp = &tmm[index].segs_priv[0];
	t_area[index].t_sys_new.s_r[12] = index;
	STST(&t_area[index].t_mm_new);
	t_area[index].t_mm_new.s_pc = (int)slmmhandler;
	t_area[index].t_mm_new.s_sp = Tmmstack[index];
	t_area[index].t_mm_new.s_crp = &tmm[index].segs_priv[0];
	t_area[index].t_mm_new.s_r[12] = index;
	STST(&t_area[index].t_prog_new);
        t_area[index].t_prog_new.s_pc = (int)slproghandler;
        t_area[index].t_prog_new.s_sp = Tsysstack[index];
        t_area[index].t_prog_new.s_crp = &tmm[index].segs_priv[0];
        t_area[index].t_prog_new.s_r[12] = index;
	r2 = PROGTRAP;
        r3 = (int)&t_area[index].t_prog_old;
        r4 = (int)&t_area[index].t_prog_new;
	SYS5();
        r2 = MMTRAP;
        r3 = (int)&t_area[index].t_mm_old;
        r4 = (int)&t_area[index].t_mm_new;
	SYS5();
	r2 = SYSTRAP;
        r3 = (int)&t_area[index].t_sys_old;
        r4 = (int)&t_area[index].t_sys_new;
        SYS5();
	boot_state.s_sr.ps_s = 0;
	boot_state.s_pc = BOOTPC;
	boot_state.s_crp = &tmm[index].segs[0];
	boot_state.s_sp = BOOTSP;
	LDST(&boot_state);
}

void static slmmhandler(){
    	int index;
    	int newpg;
    	int pgnum;
    	int segnum;
    	int presence;
    	int reference_t;
    	int reference_s;
    	state_t temp;
    	STST(&temp);
    	index = temp.s_r[12];
    	pgnum = t_area[index].t_mm_old.s_tmp.tmp_mm.mm_pg;
    	segnum = t_area[index].t_mm_old.s_tmp.tmp_mm.mm_seg;
    	presence = tmm[index].segs_priv[segnum].sd_p;
    	reference_t = tmm[index].page_tbl[pgnum].pd_r;
    	reference_s = share_tble[pgnum].pd_r;
    	if(presence < 1){
        	Terminate(&t_area[index].t_mm_old, index);
    	} else{
		switch(segnum){
            		case 0:
                		Terminate(&t_area[index].t_mm_old, index);
               	 		break;
            		case 1:
                		newpg = getfreeframe(index, pgnum, 1);
                		if(reference_t == 0){
                    			pagein(index, pgnum, 1, newpg);
                		}
                		vpop semop;
				semop.op = LOCK;
                		semop.sem = &sem_mm;
                		r3 = 1;
                		r4 = (int)&semop;
                		SYS3();
				tmm[index].page_tbl[pgnum].pd_frame = newpg;
				tmm[index].page_tbl[pgnum].pd_p = 1;
				semop.op = UNLOCK;
    				semop.sem = &sem_mm;
    				r3 = 1;
    				r4 = (int)&semop;
    				SYS3();	
                		break;
            		case 2:
                		newpg = getfreeframe(index, pgnum, 2);
                		if(reference_s == 0){
                    			pagein(index, pgnum, 2, newpg);
                		}
                		//vpop semop;
				semop.op = LOCK;
                		semop.sem = &sem_mm;
                		r3 = 1;
                		r4 = (int)&semop;
                		SYS3();
                		share_tble[pgnum].pd_p = 1;
                		share_tble[pgnum].pd_frame = newpg;
                		semop.op = UNLOCK;
                		semop.sem = &sem_mm;
                		r3 = 1;
                		r4 = (int)&semop;
                		SYS3();
            		default:
                		break;
        	}
        LDST(&t_area[index].t_mm_old);
    }
}

void static slsyshandler(){
    	int index;
    	int sysnum;
    	state_t temp;
    	STST(&temp);
    	index = temp.s_r[12];
    	sysnum = t_area[index].t_sys_old.s_tmp.tmp_sys.sys_no;
    	switch (sysnum) {
        	case 9:
            		Read_from_Terminal(&t_area[index].t_sys_old, index);
            		break;
        	case 10:
            		Write_to_Terminal(&t_area[index].t_sys_old, index);
            		break;
        	case 11:
			virtualv();
			break;
		case 12:
			virtualp();
			break;
		case 13:
            		Delay(&t_area[index].t_sys_old, index);
            		break;
		case 14:
			diskput();
			break;
		case 15:
			diskget();
			break;
        	case 16:
			Get_Time_of_Day(&t_area[index].t_sys_old);
			break;
		case 17:
            		Terminate(&t_area[index].t_sys_old, index);
            		break;
        	default:
            		break;
    	}
    	LDST(&t_area[index].t_sys_old);
}

void static slproghandler(){
    	int t_index;
    	state_t t_temp;
    	STST(&t_temp);
    	t_index = t_temp.s_r[12];
    	Terminate(&t_area[t_index].t_prog_old,t_index);
    	LDST(&t_area[t_index].t_prog_old);
}

void static cron(){
    	int i = 0;
    	int waitp;
    	long end;
    	waitp = FALSE;
    	while(p_alive > 0){
        	for (i = 0; i < 2; i++){
            		if(t_dly[i].sem < 0){
                		STCK(&end);
                		if(end - t_dly[i].start > t_dly[i].delay){
                    			vpop semop;
					semop.sem = &t_dly[i].sem;
					semop.op = UNLOCK;
                    			r3 = 1;
                    			r4 = (int)&semop;
                    			SYS3();
                		}
            		}
        		
		}
        	i = 0;
		while(i < 2){
            		if(t_dly[i].sem < 0){
                		waitp = TRUE;
            		} else{
				waitp = FALSE;
			}
        		i++;
		}
        	if(waitp == TRUE){
            		SYS7();
        	} else if(waitp == FALSE){
            		vpop semop;
			semop.sem = &csem;
			semop.op = LOCK;
            		r3 = 1;
            		r4 = (int)&semop;
          		SYS3();
        	}
    	}
    	SYS2();
}
