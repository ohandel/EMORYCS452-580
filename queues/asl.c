//This code is my own work, it was written without consulting code written by other students (Owen Handel)
#include "../h/const.h"
#include "../h/types.h"
#include "../h/procq.e"
#include "../h/asl.h"
#include "../h/asl.e"

semd_t *semd_h;
semd_t *semdFree_h;
semd_t semdTable[MAXPROC];

int insertBlocked(int *semAdd, proc_t *p){//insert p into queue at semAdd
	semd_t *temp;
	int i;
	if(semd_h == (semd_t *) ENULL){//if ASL has no active
		temp = semdFree_h;
		semdFree_h = semdFree_h->s_next;
		temp->s_next = (semd_t *) ENULL;
		temp->s_prev = (semd_t *) ENULL;
		temp->s_semAdd = semAdd;
		insertProc(&temp->s_link, p);
		i = 0;
		while(p->semvec[i] != (int *) ENULL){//find open
			i++;
		}
		p->semvec[i] = semAdd;
		semdFree_h->s_prev = (semd_t *) ENULL;
        	semd_h = temp;
        	return FALSE;
	} else if(semd_h != (semd_t *) ENULL){//ASL has active
		semd_t *orginal;
		temp = semd_h;
		while(temp->s_semAdd != semAdd){//search ASL
			orginal = temp;
			temp = temp->s_next;
			if(temp == (semd_t *) ENULL && semdFree_h != (semd_t *) ENULL){
				temp = semdFree_h;//when not found in ASL
				semdFree_h = semdFree_h->s_next;
                		temp->s_next = (semd_t *) ENULL;
				temp->s_semAdd = semAdd;
				insertProc(&temp->s_link, p);
				i = 0;
				while(p->semvec[i] != (int *) ENULL){
                        		i++;//find open
                		}
                		p->semvec[i] = semAdd;
				if(semdFree_h != (semd_t *) ENULL){//if free not empty
					semdFree_h->s_prev = (semd_t *) ENULL;
				}
				orginal->s_next = temp;
				temp->s_prev = orginal;
				return FALSE;
			} else if(temp == (semd_t *) ENULL && semdFree_h == (semd_t *) ENULL){
				return TRUE;//free list empty && need allocation
			}
		}
	   	insertProc(&temp->s_link, p);//insert p
           	i = 0;
           	while(p->semvec[i] != (int *) ENULL){
           		i++;//find open in sem vector
           	}
		p->semvec[i] = semAdd;
		return FALSE;
	}
}

proc_t *removeBlocked(int *semAdd){//remove first entry from queue at semAdd
	semd_t *temp;
	semd_t *prev;
	semd_t *next;
	proc_t *rem;
	int i;
	proc_t *head;
	if(semd_h == (semd_t *) ENULL){//if ASL empty
		return (proc_t *) ENULL;
	} else if(semd_h != (semd_t *) ENULL){
		semd_t *orginal;
		temp = semd_h;
		if(temp->s_semAdd == semAdd){
			next = temp->s_next;
			prev = (semd_t *) ENULL;
		}
		while(temp->s_semAdd != semAdd){//find in ASL
			prev = temp;
			temp = temp->s_next;
			if(temp == (semd_t *) ENULL){
				return (proc_t *) ENULL;
			}
			next = temp->s_next;
		}
		head = headQueue(temp->s_link);//get head of queue
		rem = removeProc(&temp->s_link);//remove from queue
                while(rem == head && temp->s_link.next == (proc_t *) ENULL) { 
			if(next != (semd_t *) ENULL){//if remove makes queue empty^
				if(prev != (semd_t *) ENULL){//evalute position in ASL
					prev->s_next = next;
					next->s_prev = prev;
				} else if(prev == (semd_t *) ENULL){
					next->s_prev = prev;
					semd_h = next;
				}
			} else if(next == (semd_t *) ENULL){
				if(prev != (semd_t *) ENULL){
					prev->s_next = next;
				} else {
					semd_h = (semd_t *) ENULL;
				}
			}
			if(semdFree_h != (semd_t *) ENULL){//free list not empty
				semd_t *isFree;
				isFree = semdFree_h;
				while(isFree->s_next != (semd_t *) ENULL){
					isFree = isFree->s_next;
				}
				isFree->s_next = temp;
				temp->s_prev = isFree;
			} else if (semdFree_h == (semd_t *) ENULL){//free list empty
                                temp->s_prev = (semd_t *) ENULL;
				semdFree_h = temp;
			}
			temp->s_next = (semd_t *) ENULL;
			temp->s_semAdd = (int *) ENULL;
			break;
		}
	}
	i = 0;
        while(rem->semvec[i] != semAdd){
        	i++;
        }
       	rem->semvec[i] = (int *) ENULL;
	return rem;
}

proc_t *outBlocked(proc_t *p){//similar to removeBlocked
	int i;
	semd_t *temp;
	semd_t *next;
	semd_t *prev;
	semd_t *saved;
	proc_t *rem;
	proc_t *head;
	proc_t *found = (proc_t *) ENULL;
	if(semd_h == (semd_t *) ENULL){
		return (proc_t *) ENULL;
	} else if(semd_h != (semd_t *) ENULL){
		temp = semd_h;
		while(temp != (semd_t *) ENULL){
			head = headQueue(temp->s_link);
			rem = outProc(&temp->s_link, p);
			prev = temp->s_prev;
			next = temp->s_next;
			if(rem != (proc_t *) ENULL){
                        	i = 0;
                               	while(rem->semvec[i] != temp->s_semAdd){
                                        i++;
                                }
                                rem->semvec[i] = (int *) ENULL;
                                found = rem;
                        }
			saved = temp->s_next;
			while(rem == head && temp->s_link.next == head) {
				if(next != (semd_t *) ENULL){
                                	if(prev != (semd_t *) ENULL){
                                        	prev->s_next = next;
                                        	next->s_prev = prev;
                                	} else if(prev == (semd_t *) ENULL){
                                        	next->s_prev = prev;
                                        	semd_h = next;
                                	}
                        	} else if(next == (semd_t *) ENULL){
                                	if(prev != (semd_t *) ENULL){
                                        	prev->s_next = next;
                                	} else {
                                        	semd_h = (semd_t *) ENULL;
                                	}
                        	}
				if(semdFree_h != (semd_t *) ENULL){
    					semd_t *isFree;
    					isFree = semdFree_h;
    					while(isFree->s_next != (semd_t *) ENULL){
        					isFree = isFree->s_next;
    					}
   					isFree->s_next = temp;
    					temp->s_prev = isFree;
    					temp->s_next = (semd_t *) ENULL;
    					temp->s_semAdd = (int *) ENULL;
				} else if (semdFree_h == (semd_t *) ENULL){
    					temp->s_prev = (semd_t *) ENULL;
    					semdFree_h = temp;
				}
				temp->s_next = (semd_t *) ENULL;
                        	temp->s_semAdd = (int *) ENULL;
				break;
			}
			//temp = temp->s_next;
			if(temp->s_next == (semd_t *) ENULL){//very sloppy but im over it 
				temp = saved;
				saved = (semd_t *) ENULL;
			} else{
				temp = temp->s_next;
			}
		}
		if(found != (proc_t *) ENULL){
			return found;
		} else{
			return (proc_t *) ENULL;
		}
	}
}

proc_t *headBlocked(int *semAdd){//find head of queue at semAdd
	semd_t *temp;
	proc_t *head;
	if(semd_h == (semd_t *) ENULL){//if ASL empty
		return (proc_t *) ENULL;
	} else if(semd_h != (semd_t *) ENULL){
		temp = semd_h;
		while(temp != (semd_t *) ENULL && temp->s_semAdd != semAdd){//find in ASL
			temp = temp->s_next;
		}
		if(temp == (semd_t *) ENULL){//not found 
			return (proc_t *) ENULL;
		} else if(headQueue(temp->s_link) != (proc_t *) ENULL){
			head = headQueue(temp->s_link);
			return head;
		}
	}
}

proc_t *headBlocked_special(int *semAdd){//find head of queue at semAdd
        semd_t *temp;
        proc_t *head;
        if(semd_h == (semd_t *) ENULL){//if ASL empty
                return (proc_t *) ENULL;
        } else if(semd_h != (semd_t *) ENULL){
                temp = semd_h;
                while(temp != (semd_t *) ENULL && temp->s_semAdd != semAdd){//find in A$
                        temp = temp->s_next;
                }
                if(temp == (semd_t *) ENULL){//not found 
                        return (proc_t *) ENULL;
                } else{
			return temp->s_link.next;
		}
	}
}

int initSemd(){//initialize semdTable semdFree_h semd_h same as initProc more or less
	int i;
	for(i = 0; i < MAXPROC; i++){
		semdTable[i].s_next = (semd_t *) ENULL;
		semdTable[i].s_prev = (semd_t *) ENULL;
		semdTable[i].s_semAdd = (int *) ENULL;
		semdTable[i].s_link.next = (proc_t *) ENULL;
		semdTable[i].s_link.index = ENULL;

	}
	for(i = 1; i < MAXPROC - 1; i++){
		semdTable[i].s_next = &semdTable[i+1];
		semdTable[i].s_prev = &semdTable[i-1];
	}
	semdTable[0].s_next = &semdTable[0+1];
	semdTable[19].s_prev = &semdTable[19-1];
	semd_h = (semd_t *) ENULL;
	semdFree_h = &semdTable[0];
	return 0;
}

int headASL(){//determine if ASL empty
	if(semd_h == (semd_t *) ENULL){//empty
		return FALSE;
	}else {//not empty
		return TRUE;
	}
}

semd_t *findASL(){
	if(semd_h == (semd_t *) ENULL){
		return (semd_t *) ENULL;
	} else if(semd_h != (semd_t *) ENULL){
		return semd_h;
	}
}

void cleanASL(semd_t *dead){
	semd_t *temp;
	temp = dead;
	if(temp->s_link.index != ENULL && temp->s_link.next != (proc_t*) ENULL){
		return;
	}
	if(semdFree_h != (semd_t *) ENULL){//free list not empty
        	semd_t *isFree;
                isFree = semdFree_h;
                while(isFree->s_next != (semd_t *) ENULL){
                	isFree = isFree->s_next;
                }
                isFree->s_next = temp;
                temp->s_prev = isFree;
                } else if (semdFree_h == (semd_t *) ENULL){//free list empty
                	temp->s_prev = (semd_t *) ENULL;
                        semdFree_h = temp;
                }
                temp->s_next = (semd_t *) ENULL;
                temp->s_semAdd = (int *) ENULL;
		if(temp == semd_h){
			semd_h = (semd_t *) ENULL;
		}
}
