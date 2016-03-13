#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_
#include "linux/spinlock_types.h"
#include "linux/wait.h"

typedef void *(mempool_alloc_t)(gfp_t gfp_mask,void *pool_data);
typedef void  (mempool_free_t)(void *element,void *pool_data);

typedef struct mempool_s {
	spinlock_t lock;
	int min_nr;           /* nr of elements at *elements */
	int curr_nr;          /* Current nr of elements at *elements */
	void **elements;

	void *pool_data;
	mempool_alloc_t *alloc;
	mempool_free_t *free;
	wait_queue_head_t wait;
} mempool_t;

#endif
