#include "vtcp.h"

void vtcp_initialize(struct vtcp *pvtcp, void *parameter, t_vtcp_procedure p_procedure)
{
	unsigned int i;

	pvtcp->door_count = 0;

	pvtcp->count = 0;
	pvtcp->maximum = 0;

	pvtcp->parameter = parameter;

	pvtcp->p_procedure = p_procedure;

	i = 1024;
	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_REQUEST, NULL, 0, (void **)&pvtcp->sessions, NULL, sizeof(void *) * i);
	if (pvtcp->sessions)
	{
		pvtcp->maximum = i;
		for (i = 0; i < pvtcp->maximum; i++)
		{
			pvtcp->sessions[i] = NULL;
		}
	}

	pvtcp->fd = (unsigned int)-1;
}
void vtcp_uninitialize(struct vtcp *pvtcp)
{
	if (pvtcp->sessions)
	{
		pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_RECYCLE, NULL, 0, (void **)&pvtcp->sessions, NULL, 0);
	}
	pvtcp->count = 0;
	pvtcp->maximum = 0;
}

struct vtcp_door *vtcp_search_door(struct vtcp *pvtcp, const unsigned char *address, unsigned int addresssize)
{
	struct vtcp_door *pdoor = NULL;
	unsigned int i;

	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_LOCK, (const unsigned char *)VTCP_LOCK_DOOR, 0, NULL, NULL, 1);

	for (i = 0; i < pvtcp->door_count; i++)
	{
		pdoor = &pvtcp->doors[i];
		if (pvtcp->p_procedure(pvtcp->parameter, NULL, pvtcp->fd, VTCP_ADDRESSES_COMPARE, pdoor->address, sizeof(pdoor->address), NULL, (unsigned char *)address, addresssize) == 0)
		{
			// ÕÒµ½
			break;
		}
		pdoor = NULL;
	}

	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_LOCK, (const unsigned char *)VTCP_LOCK_DOOR, 0, NULL, NULL, 0);

	return(pdoor);
}

struct vtcp_door *vtcp_door_open(struct vtcp *pvtcp, void *pointer, const unsigned char *address, unsigned int addresssize)
{
	struct vtcp_door *pdoor = NULL;
	unsigned int i;

	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_LOCK, (const unsigned char *)VTCP_LOCK_DOOR, 0, NULL, NULL, 1);

	for (i = 0; i < pvtcp->door_count; i++)
	{
		pdoor = &pvtcp->doors[i];
		if (pvtcp->p_procedure(pvtcp->parameter, NULL, pvtcp->fd, VTCP_ADDRESSES_COMPARE, pdoor->address, sizeof(pdoor->address), &pointer, (unsigned char *)address, addresssize) == 0)
		{
			// ÕÒµ½
			break;
		}
		pdoor = NULL;
	}

	if (pdoor == NULL)
	{
		if (pvtcp->door_count < sizeof(pvtcp->doors) / sizeof(pvtcp->doors[0]))
		{
			pdoor = &pvtcp->doors[pvtcp->door_count++];

			memset(pdoor->address, 0, sizeof(pdoor->address));
			memcpy(pdoor->address, address, addresssize);
		}
	}

	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_LOCK, (const unsigned char *)VTCP_LOCK_DOOR, 0, NULL, NULL, 0);

	return(pdoor);
}
int vtcp_door_close(struct vtcp *pvtcp, struct vtcp_door *pdoor)
{
	struct vtcp_door *pdoor0;
	struct vtcp_door *pdoor1;
	unsigned int i;
	int result = 0;

	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_LOCK, (const unsigned char *)VTCP_LOCK_DOOR, 0, NULL, NULL, 1);

	i = pdoor - &pvtcp->doors[0];
	if (i < pvtcp->door_count)
	{
		pvtcp->door_count--;

		if (i < pvtcp->door_count)
		{
			pdoor0 = &pvtcp->doors[i];
			pdoor1 = &pvtcp->doors[pvtcp->door_count];

			memcpy(pdoor0->address, pdoor1->address, sizeof(pdoor0->address));
		}

		result = 1;
	}

	pvtcp->p_procedure(pvtcp->parameter, NULL, 0, VTCP_LOCK, (const unsigned char *)VTCP_LOCK_DOOR, 0, NULL, NULL, 0);

	return(result);
}