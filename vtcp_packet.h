#ifndef VTCP_PACKET_H
#define VTCP_PACKET_H

#include "vtcp.h"

#include <tchar.h>
#include <stdio.h>

struct vtcp_packet
{
	struct vtcp_pkt_ext packets[VTCP_PACKET_CACHE_COUNT];

	unsigned int count;
};

unsigned int vtcp_read2bytes(const unsigned char *buffer);
void vtcp_write2bytes(unsigned char *buffer, unsigned int value);
unsigned int vtcp_read4bytes(const unsigned char *buffer);
void vtcp_write4bytes(unsigned char *buffer, unsigned int value);

void vtcp_packet_initialize(struct vtcp_packet *pp);
void vtcp_packet_uninitialize(struct vtcp_packet *pp);

struct vtcp_pkt_ext *vtcp_packet_alloc(struct vtcp_packet *pp, unsigned int sn);
struct vtcp_pkt_ext *vtcp_packet_get(struct vtcp_packet *pp, unsigned int sn);
unsigned int vtcp_packet_set_index(struct vtcp_packet *pp, unsigned int index, unsigned int sn, unsigned int count);
unsigned int vtcp_packet_free(struct vtcp_packet *pp, unsigned int sn);
unsigned int vtcp_packet_free(struct vtcp_packet *pp, unsigned int sn, unsigned int count);
unsigned int vtcp_packet_free(struct vtcp_packet *pp, unsigned int sn, uint8_t *bits, unsigned char bitssize);

unsigned int vtcp_packet_makebits(struct vtcp_packet *pp, unsigned int sn, unsigned int minimum, uint8_t *bits);

#endif