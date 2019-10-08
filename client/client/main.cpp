#include "..\..\vtcp_engine.h"
#include <Windows.h>
#include <tchar.h>

#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

unsigned int ipv4_2_buffer(const SOCKADDR_IN *psai, unsigned char *buffer)
{
	unsigned int result = 1;

	buffer[result++] = (unsigned char)psai->sin_family;
	buffer[result++] = (unsigned char)psai->sin_port;
	buffer[result++] = psai->sin_port >> 8;
	buffer[result++] = psai->sin_addr.S_un.S_un_b.s_b1;
	buffer[result++] = psai->sin_addr.S_un.S_un_b.s_b2;
	buffer[result++] = psai->sin_addr.S_un.S_un_b.s_b3;
	buffer[result++] = psai->sin_addr.S_un.S_un_b.s_b4;
	buffer[0] = result;

	return(result);
}
unsigned int buffer_2_ipv4(const unsigned char *buffer, SOCKADDR_IN *psai)
{
	unsigned int result = 1;

	memset(psai, 0, sizeof(SOCKADDR_IN));

	psai->sin_family = buffer[result++];
	psai->sin_port = buffer[result++];
	psai->sin_port |= buffer[result++] << 8;
	psai->sin_addr.S_un.S_un_b.s_b1 = buffer[result++];
	psai->sin_addr.S_un.S_un_b.s_b2 = buffer[result++];
	psai->sin_addr.S_un.S_un_b.s_b3 = buffer[result++];
	psai->sin_addr.S_un.S_un_b.s_b4 = buffer[result++];
	return(result);
}
unsigned int ipv6_2_buffer(const SOCKADDR_IN6 *psai, unsigned char *buffer)
{
	unsigned int i;
	unsigned int result = 1;

	// 可能不对, 还有别的没有写

	buffer[result++] = (unsigned char)psai->sin6_family;
	buffer[result++] = (unsigned char)psai->sin6_port;
	buffer[result++] = psai->sin6_port >> 8;
	for (i = 0; i < sizeof(psai->sin6_addr.u.Byte); i++)
	{
		buffer[result++] = psai->sin6_addr.u.Byte[i];
	}
	buffer[0] = result;
	return(result);
}
unsigned int buffer_2_ipv6(const unsigned char *buffer, SOCKADDR_IN6 *psai)
{
	unsigned int i;
	unsigned int result = 1;

	memset(psai, 0, sizeof(SOCKADDR_IN6));

	psai->sin6_family = buffer[result++];
	psai->sin6_port = buffer[result++];
	psai->sin6_port |= buffer[result++] << 8;
	for (i = 0; i < sizeof(psai->sin6_addr.u.Byte); i++)
	{
		psai->sin6_addr.u.Byte[i] = buffer[result++];
	}
	return(result);
}

int addresses_compare(const unsigned char *address0, const unsigned char *address1)
{
	unsigned int i;
	int compareresult;

	compareresult = address0[0] - address1[0];
	if (compareresult == 0)
	{
		compareresult = address0[1] - address1[1];
		if (compareresult == 0)
		{
			if (address0[2] != 0 || address0[3] != 0)
			{
				compareresult = address0[3] - address1[3];
				if (compareresult == 0)
				{
					compareresult = address0[2] - address1[2];
				}
			}

			if (compareresult == 0)
			{
				switch (address0[1])
				{
				case AF_INET:
					for (i = 0; i < 4; i++)
					{
						if (address0[4 + i])
						{
							break;
						}
					}
					if (i < 4)
					{
						for (i = 0; compareresult == 0 && i < 4; i++)
						{
							compareresult = address0[4 + i] - address1[4 + i];
						}
					}
					break;
				case AF_INET6:
					for (i = 0; i < 16; i++)
					{
						if (address0[4 + i])
						{
							break;
						}
					}
					if (i < 4)
					{
						for (i = 0; compareresult == 0 && i < 16; i++)
						{
							compareresult = address0[4 + i] - address1[4 + i];
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
	return(compareresult);
}

unsigned char *temp;

int WINAPI vtcp_procedure(void *parameter, const void *psession, unsigned int fd, unsigned char number, const unsigned char *address, unsigned int addresssize, void **packet, unsigned char *buffer, unsigned int bufferlength)
{
	struct vtcp_engine *pengine = (struct vtcp_engine *)parameter;
	int result = 0;
	SOCKADDR_IN sai;

	switch (number)
	{
	case VTCP_SEND:
		buffer_2_ipv4(address, &sai);
		if (!vtcp_socket_send(fd, (const sockaddr *)&sai, sizeof(sai), buffer, bufferlength))
		{
			result = 1;
		}
		break;
	case VTCP_LOAD_SEND:
		//_tprintf(_T("VTCP_LOAD_SEND %d\r\n"), bufferlength);
		break;
	case VTCP_SENT:
		*packet = (void *)"1234567890";
		result = 10;

		*packet = (void *)temp;
		result = 1024 * 16;
		result = bufferlength;
		break;
	case VTCP_RECV:
		_tprintf(_T("Receive %d\r\n"), bufferlength);
		break;
	case VTCP_CONNECT:
		_tprintf(_T("Connect %d\r\n"), bufferlength);
		break;
	case VTCP_ACCEPT:
		break;
	case VTCP_LISTEN:
		break;
	case VTCP_ADDRESSES_COMPARE:
		result = addresses_compare(address, buffer);
		break;
	case VTCP_ADDRESS_READ:
		ipv4_2_buffer((const SOCKADDR_IN *)address, buffer);
		break;
	case VTCP_REQUEST:
		*packet = MALLOC(bufferlength);
		break;
	case VTCP_RECYCLE:
		FREE(*packet);
		*packet = NULL;
		break;
	case VTCP_LOCK:
		if (bufferlength)
		{
			if (packet)
			{
				*packet = MALLOC(sizeof(CRITICAL_SECTION));
				if (*packet)
				{
					InitializeCriticalSection((LPCRITICAL_SECTION)*packet);
				}
			}
			else
			{
				if (buffer)
				{
					EnterCriticalSection((LPCRITICAL_SECTION)buffer);
				}
				else
				{
					EnterCriticalSection(&pengine->critical_sections[(unsigned int)address]);
				}
			}
		}
		else
		{
			if (packet)
			{
				if (*packet)
				{
					DeleteCriticalSection((LPCRITICAL_SECTION)*packet);

					FREE(*packet);
					*packet = NULL;
				}
			}
			else
			{
				if (buffer)
				{
					LeaveCriticalSection((LPCRITICAL_SECTION)buffer);
				}
				else
				{
					LeaveCriticalSection(&pengine->critical_sections[(unsigned int)address]);
				}
			}
		}
		break;
	default:
		break;
	}

	return(result);
}

int _tmain(int argc, TCHAR *argv[])
{
	struct vtcp_engine pengine[1];
	struct vtcp pvtcp[1];
	struct vtcp_session *psession;
	unsigned char address[20];
	unsigned int addresssize;
	SOCKADDR_IN sai;
	unsigned int tickcount;

	temp = (unsigned char *)MALLOC(1024 * 16);

	vtcp_initialize(pvtcp, pengine, vtcp_procedure);

	vtcp_engine_startup(pengine, pvtcp, 0);

	pvtcp->fd = pengine->fd;

	memset(&sai, 0, sizeof(sai));
	sai.sin_family = AF_INET;
	sai.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	sai.sin_port = htons(1980);
	//if (bind(pvtcp->fd, (const sockaddr *)&sai, sizeof(sai)) != SOCKET_ERROR)
	{
		memset(address, 0, sizeof(address));
		addresssize = ipv4_2_buffer((const SOCKADDR_IN *)&sai, address);

		tickcount = GetTickCount();
		psession = vtcp_connect(pvtcp, address, addresssize, tickcount);
		if (psession)
		{
			//vtcp_send(pvtcp, psession, (const unsigned char *)"1234567890", 10, tickcount);

			vtcp_send(pvtcp, psession, (const unsigned char *)temp, 1024 * 16, tickcount);
		}

		getchar();
	}

	vtcp_engine_cleanup(pengine);

	vtcp_uninitialize(pvtcp);

	FREE(temp);

	return(0);
}