#include "vtcp_engine.h"

SOCKET vtcp_socket_create(HANDLE hcompletion)
{
	unsigned char inbuffer[4];
	DWORD numberofbytes = 0;
	BOOL flag = FALSE;
	int optval;
	SOCKET fd;

	fd = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (fd != INVALID_SOCKET)
	{
		//下面的函数用于解决远端突然关闭会导致WSARecvFrom返回10054错误导致服务器完成队列中没有reeceive操作而设置
		// bNewBehavior
		memset(inbuffer, 0, 4);

		if (WSAIoctl(fd, SIO_UDP_CONNRESET, inbuffer, 4, NULL, 0, &numberofbytes, NULL, NULL) == 0)
		{
			optval = 0;
			if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, sizeof(optval)) != SOCKET_ERROR)
			{
				optval = 0;;
				if (setsockopt(fd, IPPROTO_IP, IP_DONTFRAGMENT, (const char *)&optval, sizeof(optval)) != SOCKET_ERROR)
				{
					flag = CreateIoCompletionPort((HANDLE)fd, hcompletion, NULL, 0) == hcompletion;
				}
			}
		}

		if (!flag)
		{
			closesocket(fd);
			fd = INVALID_SOCKET;
		}
	}

	return(fd);
}
int vtcp_socket_send(unsigned int fd, const sockaddr *psai, unsigned int saisize, const unsigned char *buffer, unsigned int length)
{
	WSABUF wb;
	DWORD numberofbytes;
	int result = 0;
	int errorcode;

	wb.buf = (char *)buffer;
	wb.len = length;

	if (WSASendTo((SOCKET)fd, &wb, 1, &numberofbytes, 0, psai, saisize, NULL, NULL) == 0)
	{
		result = numberofbytes == length;
	}
	else
	{
		errorcode = WSAGetLastError();
		_tprintf(_T("WSASendTo WSAGetLastError %d\r\n"), errorcode);
	}
	return(result);
}

DWORD WINAPI VtcpTimerProc(LPVOID parameter)
{
	struct vtcp_engine *pengine = (struct vtcp_engine *)parameter;
	struct vtcp *pvtcp = pengine->pvtcp;
	HANDLE hevents[2];
	LARGE_INTEGER li;
	unsigned int count = 0;
	DWORD tickcount0, tickcount1;

	li.QuadPart = -1;

	hevents[0] = pengine->hquit;
	hevents[1] = CreateWaitableTimer(NULL, FALSE, NULL);
	if (hevents[1])
	{
		if (SetWaitableTimer(hevents[1], &li, 15, NULL, NULL, FALSE))
		{
			tickcount0 = GetTickCount();

			while ((WAIT_OBJECT_0 + 1) == WaitForMultipleObjects(2, hevents, FALSE, INFINITE))
			{
				tickcount1 = GetTickCount();

				vtcp_session_timer(pvtcp, tickcount1, tickcount1 - tickcount0, count++);

				tickcount0 = tickcount1;
			}
		}

		CloseHandle(hevents[1]);
	}

	return(0);
}

unsigned char *vtcp_engine_receive(struct vtcp *pvtcp, PSOCKET_OVERLAPPED pso, unsigned char *buffer, unsigned int *bufferlength, unsigned int buffersize, SOCKET s)
{
	DWORD flags;
	DWORD numberofbytes;
	int errorcode;
	int flag;

	*bufferlength = 0;

	do
	{
		flag = 0;

		ZeroMemory(&pso->o, sizeof(OVERLAPPED));

		pso->wb.buf = (char *)buffer;
		pso->wb.len = buffersize;

		pso->saisize = sizeof(pso->sai);

		flags = 0;

		numberofbytes = 0;
		if (WSARecvFrom(s, &pso->wb, 1, &numberofbytes, &flags, (PSOCKADDR)&pso->sai, &pso->saisize, &pso->o, NULL) != SOCKET_ERROR)
		{
			//*bufferlength = numberofbytes;
		}
		else
		{
			errorcode = WSAGetLastError();
			switch (errorcode)
			{
			case WSA_IO_PENDING:
				buffer = NULL;
				break;
			case WSAENOBUFS:
				pso->wb.buf = NULL;
				pso->wb.len = 0;
				if (WSARecvFrom(s, &pso->wb, 1, &numberofbytes, &flags, (PSOCKADDR)&pso->sai, &pso->saisize, NULL, NULL) != SOCKET_ERROR)
				{
					flag = 1;

					_tprintf(_T("RecvFrom %d\r\n"), numberofbytes);
				}
				else
				{
					errorcode = WSAGetLastError();
					switch (errorcode)
					{
					case WSAEWOULDBLOCK:
						// 再去请求投递
						flag = 1;

						_tprintf(_T("Pending again\r\n"));
						break;
					default:
						_tprintf(_T("RecvFrom WSAGetLastError %d\r\n"), errorcode);
						break;
					}
				}
				break;
			default:
				_tprintf(_T("WSAGetLastError %d\r\n"), errorcode);
				MessageBox(NULL, L"", L"", MB_OK);
				break;
			}
		}
	} while (flag);

	return(buffer);
}

DWORD CALLBACK VtcpWorkProc(LPVOID parameter)
{
	struct vtcp_engine *pengine = (struct vtcp_engine *)parameter;
	struct vtcp *pvtcp = pengine->pvtcp;
	HANDLE hcompletion = pengine->hcompletion;
	OVERLAPPED *po;
	PSOCKET_OVERLAPPED pso;
	ULONG_PTR completionkey;
	DWORD numberofbytes;
	int errorcode;
	BOOL flag;
	unsigned int fd = pengine->fd;
	unsigned char *p;
	unsigned int bufferlength;
	unsigned int tickcount;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

	while (pengine->working)
	{
		errorcode = 0;

		flag = GetQueuedCompletionStatus(hcompletion, &numberofbytes, &completionkey, &po, INFINITE);
		if (po != NULL)
		{
			pso = (PSOCKET_OVERLAPPED)CONTAINING_RECORD(po, SOCKET_OVERLAPPED, o);

			if (numberofbytes)
			{
				p = (unsigned char *)pso->wb.buf;
				bufferlength = numberofbytes;

				tickcount = GetTickCount();

				int temp = 0;

				while (pengine->working && p && bufferlength)
				{
					struct vtcp_door door;
					struct vtcp_pkt *pp;

					tickcount = GetTickCount();

					memset(door.address, 0, sizeof(door.address));
					pvtcp->p_procedure(pvtcp->parameter, NULL, pvtcp->fd, VTCP_ADDRESS_READ, (const unsigned char *)&pso->sai, pso->saisize, NULL, door.address, sizeof(door.address));

					pp = (struct vtcp_pkt *)p;

					pp->hdr.cmd = vtcp_read2bytes((const unsigned char *)&pp->hdr.cmd);
					pp->hdr.index = vtcp_read2bytes((const unsigned char *)&pp->hdr.index);

					switch (LOBYTE(pp->hdr.cmd))
					{
					case VTCP_PKTCMD_CONNECT:
						vtcp_door_onrecv(pvtcp, pp, bufferlength, door.address, sizeof(door.address), 0, tickcount);
						break;
					default:
						vtcp_onrecv(pvtcp, pp, bufferlength, door.address, sizeof(door.address), 0, tickcount);
						break;
					}

					temp++;

					p = vtcp_engine_receive(pvtcp, pso, p, &bufferlength, 4096, (SOCKET)fd);
				}

				if (temp > 1)
				{
					_tprintf(_T("temp %d\r\n"), temp);
				}
			}
		}
	}

	return(0);
}

int vtcp_engine_startup(struct vtcp_engine *pengine, struct vtcp *pvtcp, unsigned int port)
{
	WSADATA wsadata;

	WSAStartup(MAKEWORD(1, 1), &wsadata);

	unsigned int i;

	for (i = 0; i < 2; i++)
	{
		InitializeCriticalSection(&pengine->critical_sections[i]);
	}

	pengine->pvtcp = pvtcp;

	pengine->working = TRUE;

	pengine->hquit = CreateEvent(NULL, TRUE, FALSE, NULL);

	pengine->hcompletion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	pengine->fd = vtcp_socket_create(pengine->hcompletion);

	SOCKADDR_IN sai;
	memset(&sai, 0, sizeof(sai));
	sai.sin_family = AF_INET;
	sai.sin_addr.S_un.S_addr = INADDR_ANY;
	sai.sin_port = htons(port);
	if (bind(pengine->fd, (const sockaddr *)&sai, sizeof(sai)) != SOCKET_ERROR)
	{
	}

	unsigned char *buffer;
	unsigned int bufferlength;
	unsigned int j = 0;

	pengine->hthread = CreateThread(NULL, 0, VtcpTimerProc, (LPVOID)pengine, 0, NULL);
	for (i = 0; i < VTCP_THREAD_COUNT; i++)
	{
		pengine->hthreads[i] = NULL;

		pengine->buffers[i] = (unsigned char *)VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
		if (pengine->buffers[i])
		{
			j++;

			buffer = pengine->buffers[i];

			do
			{
				buffer = vtcp_engine_receive(pengine->pvtcp, &pengine->so[i], buffer, &bufferlength, 4096, (SOCKET)pengine->fd);
			} while (buffer && bufferlength);

			if (buffer == NULL)
			{
				pengine->hthreads[i] = CreateThread(NULL, 0, VtcpWorkProc, (LPVOID)pengine, 0, NULL);
			}
		}
	}

	return(0);
}

int vtcp_engine_cleanup(struct vtcp_engine *pengine)
{
	pengine->working = FALSE;

	if (pengine->hquit)
	{
		SetEvent(pengine->hquit);
	}

	unsigned int i;

	for (i = 0; i < 2; i++)
	{
		DeleteCriticalSection(&pengine->critical_sections[i]);
	}

	WSACleanup();

	if (pengine->hthread)
	{
		WaitForSingleObject(pengine->hthread, INFINITE);
		CloseHandle(pengine->hthread);
		pengine->hthread = NULL;
	}

	if (pengine->fd != INVALID_SOCKET)
	{
		closesocket(pengine->fd);
	}

	for (i = 0; i < VTCP_THREAD_COUNT; i++)
	{
		if (pengine->hthreads[i])
		{
			PostQueuedCompletionStatus(pengine->hcompletion, 0, (DWORD)NULL, NULL);
		}
	}

	for (i = 0; i < VTCP_THREAD_COUNT; i++)
	{
		if (pengine->hthreads[i])
		{
			WaitForSingleObject(pengine->hthreads[i], INFINITE);
			CloseHandle(pengine->hthreads[i]);
			pengine->hthreads[i] = NULL;
		}
	}

	CloseHandle(pengine->hcompletion);

	for (i = 0; i < VTCP_THREAD_COUNT; i++)
	{
		if (pengine->buffers[i])
		{
			VirtualFree(pengine->buffers[i], 0, MEM_RELEASE);
			pengine->buffers[i] = NULL;
		}
	}

	if (pengine->hquit)
	{
		CloseHandle(pengine->hquit);
		pengine->hquit = NULL;
	}

	return(0);
}

