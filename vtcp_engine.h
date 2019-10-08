#ifndef VTCP_ENGINE_H
#define VTCP_ENGINE_H

#include "vtcp.h"

#include "vtcp_session.h"

#define VTCP_THREAD_COUNT							3

typedef struct _SOCKET_OVERLAPPED
{
	WSAOVERLAPPED o;

	LPVOID pointer;

	WSABUF wb;

	struct sockaddr_in sai;
	int saisize;

	DWORD flags;
	DWORD numberofbytes;

}SOCKET_OVERLAPPED, *PSOCKET_OVERLAPPED;

struct vtcp_engine
{
	HANDLE hcompletion;

	HANDLE hquit;

	struct vtcp *pvtcp;

	HANDLE hthread;
	HANDLE hthreads[VTCP_THREAD_COUNT];

	unsigned char *buffers[VTCP_THREAD_COUNT];
	SOCKET_OVERLAPPED so[VTCP_THREAD_COUNT];

	CRITICAL_SECTION critical_sections[2];

	unsigned int fd;
	int working;
};

SOCKET vtcp_socket_create(HANDLE hcompletion);
int vtcp_socket_send(unsigned int fd, const sockaddr *psai, unsigned int saisize, const unsigned char *buffer, unsigned int length);

int vtcp_engine_startup(struct vtcp_engine *pengine, struct vtcp *pvtcp, unsigned int port);
int vtcp_engine_cleanup(struct vtcp_engine *pengine);

#endif