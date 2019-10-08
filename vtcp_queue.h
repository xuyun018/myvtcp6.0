#ifndef VTCP_QUEUE_H
#define VTCP_QUEUE_H

#include "vtcp.h"

void vtcp_queue_initialize(struct vtcp_queue *pq);
void vtcp_queue_uninitialize(struct vtcp_queue *pq);

struct vtcp_buffer *vtcp_queue_alloc(struct vtcp_queue *pq);
struct vtcp_buffer *vtcp_queue_getfirst(struct vtcp_queue *pq);
struct vtcp_buffer *vtcp_queue_getat(struct vtcp_queue *pq, unsigned int index);
void vtcp_queue_skip(struct vtcp_queue *pq);

#endif