#include "vtcp_queue.h"

void vtcp_queue_initialize(struct vtcp_queue *pq)
{
	pq->index = 0;
	pq->count = 0;
}
void vtcp_queue_uninitialize(struct vtcp_queue *pq)
{
	//
}

struct vtcp_buffer *vtcp_queue_alloc(struct vtcp_queue *pq)
{
	struct vtcp_buffer *result = NULL;
	unsigned int index;

	if (pq->count < VTCP_ASYNC_CACHE_COUNT)
	{
		index = (pq->index + pq->count++) & VTCP_ASYNC_CACHE_COUNT_MASK;

		result = (struct vtcp_buffer *)&pq->queue[sizeof(struct vtcp_buffer) * index];
	}

	return(result);
}
struct vtcp_buffer *vtcp_queue_getfirst(struct vtcp_queue *pq)
{
	struct vtcp_buffer *result = NULL;

	if (pq->count)
	{
		result = (struct vtcp_buffer *)&pq->queue[sizeof(struct vtcp_buffer) * pq->index];
	}

	return(result);
}
struct vtcp_buffer *vtcp_queue_getat(struct vtcp_queue *pq, unsigned int index)
{
	struct vtcp_buffer *result = NULL;

	if (index < pq->count)
	{
		result = (struct vtcp_buffer *)&pq->queue[sizeof(struct vtcp_buffer) * ((pq->index + index) & VTCP_ASYNC_CACHE_COUNT_MASK)];
	}

	return(result);
}
void vtcp_queue_skip(struct vtcp_queue *pq)
{
	if (pq->count)
	{
		pq->index++;
		pq->index &= VTCP_ASYNC_CACHE_COUNT_MASK;
		pq->count--;
	}
}