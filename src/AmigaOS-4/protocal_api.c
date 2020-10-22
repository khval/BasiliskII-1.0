
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "/infinity/includes/protocol_api.h"

struct protocol_api *get_protocol(char *name, int sig)
{
	struct MsgPort		*port;
	struct protocol_msg	*msg;
	void				*ptr;


	ptr = NULL;

	if (port = FindPort(name))
	{
		msg = (struct protocol_msg *) IExec->AllocSysObjectTags(ASOT_MESSAGE
				,ASOMSG_Size, sizeof(struct protocol_msg)
				,TAG_DONE);

		msg -> get_api = &ptr;
		msg -> sig = sig;
		msg -> task = FindTask(NULL);

		PutMsg(  port,  (struct Message *) msg );
	}

	printf("return hex: %x\n",ptr);

	Delay(2);	// just wait, and hope ptr is set.

	return (struct protocol_api *) ptr;
}


