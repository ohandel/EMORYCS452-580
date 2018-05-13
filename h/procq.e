#include "procq.h"

extern panic();

extern int insertProc(proc_link *tp, proc_t *p);

extern proc_t* removeProc(proc_link *tp);

extern proc_t* outProc(proc_link *tp, proc_t *p);

extern proc_t* allocProc();

extern int freeProc(proc_t *p);

extern proc_t* headQueue(proc_link tp);

extern int initProc();

