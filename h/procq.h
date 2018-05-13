#ifndef PROCQ
#define PROCQ

typedef struct proc_link {
    	int index;
    	struct proc_t *next;
} proc_link;

typedef struct proc_t proc_t;

struct proc_t {
    	proc_link p_link[SEMMAX];
    	state_t p_s;
    	int qcount;
    	int *semvec[SEMMAX];
	proc_t *parent;
	proc_t *child;
	proc_t *sib;
	proc_t *first_child;
	int time_CPU;
	long time_start;
	state_t *sys_old;
	state_t *sys_new;
	state_t *mm_old;
	state_t *mm_new;
	state_t *prog_old;
	state_t *prog_new;
};

#endif
