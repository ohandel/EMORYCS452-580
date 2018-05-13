#include "types.h"

typedef struct t_mmtable{
	char buf[512];
	sd_t segs[32];
	sd_t segs_priv[32];
	pd_t page_tbl[32];
	pd_t page_priv[256];
}t_mmtable;

typedef struct t_delay{
	int sem;
	int delay;
	long start;
}t_delay;

typedef struct t_statearea{
	state_t t_sys_new;
        state_t t_mm_new;
        state_t t_prog_new;
	state_t t_sys_old;
	state_t t_mm_old;
	state_t t_prog_old;
}t_statearea;
