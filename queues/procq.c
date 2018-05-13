//This code is my own work, it was written without consulting code written by other students (Owen Handel)$
#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.h"
#include "../h/procq.e"

proc_t *placeFree;
proc_t *procFree_h;
proc_t procTable[20];
char pncbuf[512];
int count;

panic(sp)
register char *sp;
{
	register char *ep = pncbuf;

	while ((*ep++ = *sp++) != '\0')
			;
		asm("	trap	#0");
}

int insertProc(proc_link *tp, proc_t *p){//insert proc p at tp
	int indexFree = 0;
	proc_t *prev;
	int previndex;
	previndex = tp->index;
	prev = tp->next;
	while(p->p_link[indexFree].index != ENULL){//find open
		if(indexFree == SEMMAX - 1){
			panic("process is in too many queues <SEMMAX");
		}
		indexFree++;
	}
	if(p->p_link[indexFree].index == ENULL && prev != (proc_t *) ENULL){
		p->p_link[indexFree].next =  prev->p_link[previndex].next;
		p->p_link[indexFree].index =  prev->p_link[previndex].index;
		prev->p_link[previndex].next = p;
		prev->p_link[previndex].index = indexFree;
		p->qcount++;
		tp->index = indexFree;
		tp->next = p;//<1 in qeueu
	} else if(p->p_link[indexFree].index == ENULL  && prev == (proc_t *) ENULL) {
		p->p_link[indexFree].next = p;
		p->p_link[indexFree].index = indexFree;
		p->qcount++;
                tp->index = indexFree;
                tp->next = p;//1 in queue
	}
	return 0;
}

proc_t* removeProc(proc_link *tp){//remove head
	proc_link queue = *(tp);
	proc_t *temp = tp->next;
	int tempIndex = tp->index;
	int remIndex;
	proc_t *remTemp;
	if(tp->next == (proc_t *) ENULL){
		return (proc_t *) ENULL;
	}
	if(headQueue(queue) == tp->next){//if queue head is pointed to by tail
                remTemp = tp->next;
                temp->p_link[tempIndex].next = (proc_t *) ENULL;
                temp->p_link[tempIndex].index = ENULL;
                tp->index = ENULL;
                tp->next = (proc_t *) ENULL;
		temp->qcount--;
                return remTemp;
        } else if(headQueue(queue) != tp->next) {//if tail !-> head
		remIndex = temp->p_link[tempIndex].index;
		remTemp = temp->p_link[tempIndex].next;
		temp->p_link[tempIndex].next = remTemp->p_link[remIndex].next;
		temp->p_link[tempIndex].index = remTemp->p_link[remIndex].index;
		remTemp->p_link[remIndex].next = (proc_t *) ENULL;
        	remTemp->p_link[remIndex].index = ENULL;
		remTemp->qcount--;
		return remTemp;
	}
}

proc_t* outProc(proc_link *tp, proc_t *p){//remove p from queue
	int tempIndex = tp->index;
	proc_t *temp = tp->next;
	proc_t *orginal = tp->next;
	int prevIndex;
	int nextIndex;
	proc_t *prevTemp;
	if (tp->next == (proc_t *)ENULL){
		return (proc_t *)ENULL;
	} else if(temp->p_link[tempIndex].next == (proc_t *) ENULL){
                        return (proc_t *) ENULL;
        }
	while(temp->p_link[tempIndex].next != p){//find p in queue
		nextIndex = temp->p_link[tempIndex].index;
		if(temp->p_link[tempIndex].next == orginal && p != tp->next){
			return (proc_t *) ENULL;
		}
                temp = temp->p_link[tempIndex].next;
                tempIndex = nextIndex;
	}
	if(temp->p_link[tempIndex].next == p && tp->next->p_link[tp->index].next != tp->next){//if tail !-> head
		prevIndex = tempIndex;
		tempIndex = temp->p_link[prevIndex].index;
		prevTemp = temp;
		temp = temp->p_link[prevIndex].next;
		prevTemp->p_link[prevIndex].next = temp->p_link[tempIndex].next;
		prevTemp->p_link[prevIndex].index = temp->p_link[tempIndex].index;
		temp->p_link[tempIndex].next = (proc_t *) ENULL;
                temp->p_link[tempIndex].index = ENULL;
		temp->qcount--;
		if(tp->next == temp){
			tp->next = prevTemp->p_link[prevIndex].next;
			tp->index = prevTemp->p_link[prevIndex].index;
		}
		return p;
	} else if(temp->p_link[tempIndex].next == p && tp->next->p_link[tp->index].next == tp->next){//if tail->head
               	prevIndex = tempIndex;
                tempIndex = temp->p_link[prevIndex].index;
               	prevTemp = temp;
                temp = temp->p_link[prevIndex].next;
                prevTemp->p_link[prevIndex].next = temp->p_link[tempIndex].next;
                prevTemp->p_link[prevIndex].index = temp->p_link[tempIndex].index;
                temp->p_link[tempIndex].next = (proc_t *) ENULL;
		temp->p_link[tempIndex].index = ENULL;
		temp->qcount--;
		tp->next = (proc_t *) ENULL;
                tp->index = ENULL;
                return p;
        }

}

proc_t* allocProc(){//return pointer from freelist
	proc_t *temp;
	temp = procFree_h;
	if(temp != (proc_t *) ENULL){//if freelist isnt empty
		procFree_h = procFree_h->p_link[0].next;
		temp->p_link[0].next = (proc_t *) ENULL;
		temp->p_link[0].index = ENULL;
		return temp;
	} else if(temp == (proc_t *) ENULL) {//freelist empty
		return (proc_t *) ENULL;
	}

		return (proc_t *) ENULL;
}

int freeProc(proc_t *p){//return element to freelist
	int i;
	proc_t *temp;
	proc_t *prev;
	proc_t *bad;
	temp = procFree_h;
	if(procFree_h == (proc_t *) ENULL){//if freelist empty
		procFree_h = p;
	} else {
		while(temp->p_link[0].next != (proc_t *) ENULL){//find free spot
			prev = temp;
			temp = temp->p_link[0].next;
		}
		temp->p_link[0].next = p;
	}
	i = 0;
	while(i < SEMMAX){//remove queues
		p->p_link[i].next = (proc_t *) ENULL;
		p->p_link[i].index = ENULL;
		p->semvec[i] = (int *) ENULL;
		i++;
	}
	p->qcount = 0;
	p->time_CPU = 0;
	p->time_start = 0;
	return 0;
}

proc_t* headQueue(proc_link tp){//return head of queue
	proc_t *tail;
	tail = tp.next;
	if(tail != (proc_t *) ENULL){//while head exists
		return tail->p_link[tp.index].next;//tail points to head 
	}
	return (proc_t *) ENULL;
}

int initProc(){//init the values for procTable
	int i;
	int j;
      	for (i=0;i<MAXPROC;i++){
		procTable[i].qcount = 0;
		procTable[i].parent = (proc_t *)ENULL;
		procTable[i].child = (proc_t *)ENULL;
		procTable[i].sib = (proc_t *)ENULL;
		procTable[i].first_child = (proc_t *)ENULL;
		procTable[i].time_CPU = 0;
		procTable[i].time_start = 0;
		procTable[i].mm_old = (state_t *)ENULL;
		procTable[i].mm_new = (state_t *)ENULL;
		procTable[i].prog_old = (state_t *)ENULL;
		procTable[i].prog_new = (state_t *)ENULL;
		procTable[i].sys_old = (state_t *)ENULL;
		procTable[i].sys_new = (state_t *)ENULL;
		for (j=0;j<SEMMAX;j++){
			procTable[i].p_link[j].next = (proc_t *) ENULL;
			procTable[i].p_link[j].index = ENULL;
			procTable[i].semvec[j] = (int *)ENULL;
		}
	}
	for(i = 0; i < MAXPROC - 1; i++){
        	procTable[i].p_link[0].next = &procTable[i+1];
		procTable[i].p_link[0].index = 0;
	}
	procTable[19].p_link[0].next = (proc_t *) ENULL;
	procTable[19].p_link[0].index = 0;
	procFree_h = &(procTable[0]);
	return 0;
}
