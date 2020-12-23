/*
 *  ether_amiga.cpp - Ethernet device driver, AmigaOS specific stuff
 *
 *  Basilisk II (C) 1997-2001 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <exec/errors.h>
#include <dos/dostags.h>
#include <devices/sana2.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "prefs.h"
#include "user_strings.h"
#include "macos_util.h"
#include "ether.h"
#include "ether_defs.h"

#define DEBUG 1
// #include "debug.h"

#define MONITOR 0

#define D(n) n
#define bug IExec->DebugPrintF

int net_proc_quit = 0;

// These messages are sent to the network process
static const uint32 MSG_CLEANUP = 'clea';			// Remove all protocols
static const uint32 MSG_ADD_MULTI = 'addm';		// Add multicast address
static const uint32 MSG_DEL_MULTI = 'delm';		// Add multicast address
static const uint32 MSG_ATTACH_PH = 'atph';		// Attach protocol handler
static const uint32 MSG_DETACH_PH = 'deph';		// Attach protocol handler
static const uint32 MSG_WRITE = 'writ';			// Write packet

struct Sana2Hook *sana2_hook;

void Sana2ErrorMsg(struct IOSana2Req *req);

struct NetMessage : public Message {
	NetMessage(uint32 what_, const struct MsgPort *reply_port)
	{
		what = what_;
		mn_ReplyPort = (struct MsgPort *)reply_port;
		mn_Length = sizeof(*this);
	}
	uint32 what;
	uint32 pointer;
	uint16 type;
	int16 result;
};


// List of attached protocols
static const int NUM_READ_REQUESTS = 32;	// Number of read requests that are sent to device in advance

struct NetProtocol : public Node {
	struct IOSana2Req read_io[NUM_READ_REQUESTS];
	uint8 read_buf[NUM_READ_REQUESTS][1518];	// 14 bytes header, 1500 bytes data, 4 bytes CRC
	uint16 type;
	uint32 handler;
};

static struct List prot_list;


// Global variables
static struct Process *net_proc = NULL;		// Network device handler process
static bool proc_error;						// Flag: process didn't initialize
static struct MsgPort *proc_port = NULL;	// Message port of process, for communication with main task
static struct MsgPort *read_port = NULL;	// Reply port for read IORequests (set up and owned by network process)

static bool write_done = false;				// Flag: write request done

extern struct Task *MainTask;				// Pointer to main task (from main_morphos.cpp)
extern struct MsgPort *StartupMsgPort;


// Prototypes
static void net_func(void);


/*
 *  Send message to network process
 */

static int16 send_to_proc(uint32 what, uint32 pointer = 0, uint16 type = 0)
{
	struct MsgPort *reply_port = &((struct Process *)FindTask(NULL))->pr_MsgPort;
	D(bug("sending %08lx to net_proc\n", what));
	NetMessage msg(what, reply_port);
	msg.pointer = pointer;
	msg.type = type;
	PutMsg(proc_port, &msg);
	WaitPort(reply_port);
	GetMsg(reply_port);
	D(bug(" sent\n"));
	return msg.result;
}


/*
 *  Initialization
 */

void EtherInit(void)
{
	static struct Message msg;

	// Do nothing if no Ethernet device specified
	if (PrefsFindString("ether") == NULL)
		return;

	// Initialize protocol list
	NewList(&prot_list);

	msg.mn_Node.ln_Type	= NT_MESSAGE;
	msg.mn_ReplyPort		= StartupMsgPort;
	msg.mn_Length			= sizeof(struct Message);

	// Start process
	proc_error = false;
	SetSignal(0, SIGF_SINGLE);

// MorphOS version

// Classic AmigaOS3 / AmigaOS4

	net_proc = CreateNewProcTags(
		NP_Entry, (ULONG)net_func,
		NP_Name, (ULONG)"Basilisk II Ethernet Task",
		NP_Priority, 1,
		TAG_END	
	);	

	if (net_proc == NULL)
		return;

	// Wait for signal from process
	Wait(SIGF_SINGLE);

	// Initialization error? Then bail out
	if (proc_error)
	{
		WaitPort(StartupMsgPort);
		GetMsg(StartupMsgPort);	// Remove startup message
		net_proc = NULL;
		return;
	}

	// Everything OK
	net_open = true;
}


/*
 *  Deinitialization
 */

void EtherExit(void)
{
	// Stop process
	if (net_proc)
	{
		Signal(&net_proc->pr_Task, SIGBREAKF_CTRL_C);

		for (;!net_proc_quit;)
		{
			Delay(1);
		}

/* DOES NOT WORK!!!!! */
//		printf("%x\n",FindTask(0));
//		Wait(SIGF_SINGLE);
//		WaitPort(StartupMsgPort);
//		GetMsg(StartupMsgPort);	// Remove startup message
	}
}


/*
 *  Reset
 */

void EtherReset(void)
{
	// Remove all protocols
	if (net_open)
		send_to_proc(MSG_CLEANUP);
}


/*
 *  Add multicast address
 */

int16 ether_add_multicast(uint32 pb)
{
	return send_to_proc(MSG_ADD_MULTI, pb);
}


/*
 *  Delete multicast address
 */

int16 ether_del_multicast(uint32 pb)
{
	return send_to_proc(MSG_DEL_MULTI, pb);
}


/*
 *  Attach protocol handler
 */

int16 ether_attach_ph(uint16 type, uint32 handler)
{
	return send_to_proc(MSG_ATTACH_PH, handler, type);
}


/*
 *  Detach protocol handler
 */

int16 ether_detach_ph(uint16 type)
{
	return send_to_proc(MSG_DETACH_PH, type);
}


/*
 *  Transmit raw ethernet packet
 */

int16 ether_write(uint32 wds)
{
	send_to_proc(MSG_WRITE, wds);
	return 1;	// Command in progress
}


/*
 *  Remove protocol from protocol list
 */

static void remove_protocol(NetProtocol *p)
{
	// Remove from list
	Forbid();
	Remove(p);
	Permit();

	// Cancel read requests
	for (int i=0; i<NUM_READ_REQUESTS; i++)
	{
		AbortIO((struct IORequest *)(p->read_io + i));
		WaitIO((struct IORequest *)(p->read_io + i));
	}

	// Free protocol struct
	FreeVec(p);
}


/*
 *  Remove all protocols
 */

static void remove_all_protocols(void)
{
	NetProtocol *n = (NetProtocol *)prot_list.lh_Head, *next;
	while ((next = (NetProtocol *)n->ln_Succ) != NULL)
	{
		remove_protocol(n);
		n = next;
	}
}


/*
 *  Copy received network packet to Mac side
 */

LONG s2_copy_to_buff(UBYTE * to_buffer,  UBYTE * from, ULONG len)
{
	D(bug("Receiving Ethernet packet:\n"));
	D(bug("CopyToBuff to %08lx, from %08lx, size %08lx\n", to_buffer, from, len));

	if ((to_buffer)&&(from)&&(len))
	{
		CopyMem(from, to_buffer, len);

#if MONITOR
		for (int i=0; i<len; i++) {
			bug("%02lx ", from[i]);
		}
		bug("\n");
#endif
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
 *  Copy data from Mac WDS to outgoing network packet
 */

LONG s2_copy_from_buff(UBYTE * to,uint8	*wds,ULONG len)
{
	if ((to)&&(wds)&&(len))
	{
		D(bug("Sending Ethernet packet:\n"));
		D(bug("CopyFromBuff to %08lx, wds %08lx, size %08lx\n", to, wds, len));

		for (;;) {
			int len = ReadMacInt16((uint32)wds);
			if (len == 0)
				break;
#if MONITOR
			uint8 *adr = Mac2HostAddr(ReadMacInt32((uint32)wds + 2));
			for (int i=0; i<len; i++) {
				bug("%02lx ", adr[i]);
			}
#endif
			CopyMem(Mac2HostAddr(ReadMacInt32((uint32)wds + 2)), to, len);
			to += len;
			wds += 6;
		}

		return 1;
	}
	else
	{
		return 0;
	}
}

ULONG hook_function(struct Hook *hook,
	struct IOSana2Req *sana2req,
	struct SANA2HookMsg *sana2hookmsg)
{
	struct SANA2CopyHookMsg *cpy = (struct SANA2CopyHookMsg *) sana2hookmsg;

	switch ( sana2hookmsg->shm_Method )
	{
		case S2_CopyToBuff :
			return s2_copy_to_buff( (UBYTE *) cpy -> schm_To, (UBYTE *) cpy -> schm_From, cpy -> schm_Size);
			break;

		case S2_CopyFromBuff:
			return s2_copy_from_buff( (UBYTE *) cpy -> schm_To, (uint8 *) cpy -> schm_From, cpy -> schm_Size);
			break;
	}

	return 0;
}


/*
 *  Process for communication with the Ethernet device
 */

	static CONST ULONG hook_tags[] =
	{
		S2_CopyToBuff, (uint32) S2_CopyToBuff,
		S2_CopyFromBuff, (uint32) S2_CopyFromBuff,
		TAG_END
	};


void display_mac(char *mac)
{
	int n;
	char buffer[(6*2)+1];

	buffer[0]=0;

	for (n=0;n<6;n++)
	{
		sprintf(buffer,"%s%02X",buffer,mac[n]);		
	}

	printf("MAC: %s\n",buffer);

}

void setup_hooks(struct IOSana2Req * s2req)
{
	if (sana2_hook = (struct Sana2Hook *) AllocVec(sizeof(struct Sana2Hook),MEMF_SHARED | MEMF_CLEAR))
	{
		sana2_hook -> s2h_Hook.h_Entry = (ULONG (*)()) hook_function;
		sana2_hook -> s2h_Hook.h_SubEntry = NULL;
		sana2_hook -> s2h_Methods = (Tag *) hook_tags;

		s2req->ios2_Req.io_Command = S2_SANA2HOOK;
		s2req->ios2_DataLength = sizeof(struct Sana2Hook);
		s2req->ios2_Data = sana2_hook;

		printf("hook errror? %d\n",DoIO((struct IORequest *)s2req));
	}
}

static void net_func(void)
{
	const char *str;
	BYTE od_error;
	struct MsgPort *write_port = NULL, *control_port = NULL;
	struct IOSana2Req *write_io = NULL, *control_io = NULL;
	bool opened = false;
	ULONG read_mask = 0, write_mask = 0, proc_port_mask;
	struct Sana2DeviceQuery query_data = {sizeof(Sana2DeviceQuery)};

	// Default: error occured
	proc_error = true;

//	NewGetTaskAttrs(NULL, &proc_port, sizeof(struct MsgPort *), TASKINFOTYPE_TASKMSGPORT, TAG_DONE);
	proc_port = CreateMsgPort();
		if (!proc_port) goto quit;

	proc_port_mask = 1 << proc_port->mp_SigBit;

	// Create message ports for device I/O
	read_port = CreateMsgPort();
	write_port = CreateMsgPort();
	control_port = CreateMsgPort();

	if (!read_port || !write_port || !control_port)
		goto quit;

	read_mask = 1 << read_port->mp_SigBit;
	write_mask = 1 << write_port->mp_SigBit;

	// Create control IORequest
	control_io = (struct IOSana2Req *)CreateIORequest(control_port, sizeof(struct IOSana2Req));
	if (control_io == NULL)
		goto quit;

	#if 1
	// this workaround is not needed for MorphOS
	control_io->ios2_Req.io_Message.mn_Node.ln_Type = 0;	// Avoid CheckIO() bug
	#endif

	// Parse device name
	char dev_name[256];
	ULONG dev_unit;

	str = PrefsFindString("ether");
	if (str)
	{
		const char *FirstSlash = strchr(str, '/');
		const char *LastSlash = strrchr(str, '/');

		if (FirstSlash && FirstSlash && FirstSlash != LastSlash)
			{
			// Device name contains path, i.e. "Networks/xyzzy.device"
			const char *lp = str;
			char *dp = dev_name;

			while (lp != LastSlash)
				*dp++ = *lp++;
			*dp = '\0';

			if (strlen(dev_name) < 1)
				goto quit;

			if (1 != sscanf(LastSlash, "/%ld", &dev_unit))
				goto quit;

//			printf("dev=<%s> unit=%d\n", dev_name, dev_unit);
			}
		else
			{
			if (2 != sscanf(str, "%[^/]/%ld", dev_name, &dev_unit))
				goto quit;
			}
		}
	else
		goto quit;

	// Open device
	control_io->ios2_BufferManagement = (void *)&hook_tags;
	od_error = OpenDevice(dev_name, dev_unit, (struct IORequest *)control_io, 0);
	if (0 != od_error || control_io->ios2_Req.io_Device == 0)
		{
		printf("WARNING: OpenDevice(<%s>, unit=%ld) returned error %d)\n", (UBYTE *)dev_name, dev_unit, od_error);
		goto quit;
		}
	opened = true;

	// Is it Ethernet?
	control_io->ios2_Req.io_Command = S2_DEVICEQUERY;
	control_io->ios2_StatData = (void *)&query_data;
	DoIO((struct IORequest *)control_io);
	if (control_io->ios2_Req.io_Error)
		goto quit;
	if (query_data.HardwareType != S2WireType_Ethernet) {
		WarningAlert(GetString(STR_NOT_ETHERNET_WARN));
		goto quit;
	}

	// Yes, create IORequest for writing
	write_io = (struct IOSana2Req *)CreateIORequest(write_port, sizeof(struct IOSana2Req));
	if (write_io == NULL)
		goto quit;
	memcpy(write_io, control_io, sizeof(struct IOSana2Req));
	write_io->ios2_Req.io_Message.mn_Node.ln_Type = 0;	// Avoid CheckIO() bug
	write_io->ios2_Req.io_Message.mn_ReplyPort = write_port;

	// Configure Ethernet
	control_io->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
	DoIO((struct IORequest *)control_io);
	memcpy(ether_addr, control_io->ios2_DstAddr, 6);
	memcpy(control_io->ios2_SrcAddr, control_io->ios2_DstAddr, 6);
	control_io->ios2_Req.io_Command = S2_CONFIGINTERFACE;
	DoIO((struct IORequest *)control_io);
	D(bug("Ethernet address %08lx %08lx\n", *(uint32 *)ether_addr, *(uint16 *)(ether_addr + 4)));

	display_mac((char *) ether_addr);

	setup_hooks(control_io);

	// Initialization went well, inform main task
	proc_error = false;
	Signal(MainTask, SIGF_SINGLE);

	// Main loop
	for (;;) {

		// Wait for I/O and messages (CTRL_C is used for quitting the task)
		ULONG sig = Wait(proc_port_mask | read_mask | write_mask | SIGBREAKF_CTRL_C);

		// Main task wants to quit us
		if (sig & SIGBREAKF_CTRL_C)
			break;

		// Main task sent a command to us
		if (sig & proc_port_mask) {
			struct NetMessage *msg;
			while ((msg = (NetMessage *)GetMsg(proc_port)))
			{
				D(bug("net_proc received %08lx\n", msg->what));
				switch (msg->what) {
					case MSG_CLEANUP:
						remove_all_protocols();
						break;

					case MSG_ADD_MULTI:
						control_io->ios2_Req.io_Command = S2_ADDMULTICASTADDRESS;
						Mac2Host_memcpy(control_io->ios2_SrcAddr, msg->pointer + eMultiAddr, 6);
						DoIO((struct IORequest *)control_io);
						if (control_io->ios2_Req.io_Error == S2ERR_NOT_SUPPORTED) {
							WarningAlert(GetString(STR_NO_MULTICAST_WARN));
							msg->result = noErr;
						} else if (control_io->ios2_Req.io_Error)
							msg->result = eMultiErr;
						else
							msg->result = noErr;
						break;

					case MSG_DEL_MULTI:
						control_io->ios2_Req.io_Command = S2_DELMULTICASTADDRESS;
						Mac2Host_memcpy(control_io->ios2_SrcAddr, msg->pointer + eMultiAddr, 6);
						DoIO((struct IORequest *)control_io);
						if (control_io->ios2_Req.io_Error)
							msg->result = eMultiErr;
						else
							msg->result = noErr;
						break;

					case MSG_ATTACH_PH: {
						uint16 type = msg->type;
						uint32 handler = msg->pointer;

						// Protocol of that type already installed?
						NetProtocol *p = (NetProtocol *)prot_list.lh_Head, *next;
						while ((next = (NetProtocol *)p->ln_Succ) != NULL) {
							if (p->type == type) {
								msg->result = lapProtErr;
								goto reply;
							}
							p = next;
						}

						// Allocate NetProtocol, set type and handler
						p = (NetProtocol *)AllocVec(sizeof(NetProtocol), MEMF_SHARED | MEMF_CLEAR);
						if (p == NULL)
						{
							msg->result = lapProtErr;
							goto reply;
						}

						p->type = type;
						p->handler = handler;
#ifdef stupid
						if ((type>0)&&(handler!=0))
						{
#endif
							printf("[KEY] Read request for network protocol 0x%x (%d) num req %d\n", type, type, NUM_READ_REQUESTS);

							// Set up and submit read requests
							for (int i=0; i<NUM_READ_REQUESTS; i++) {
								memcpy(p->read_io + i, control_io, sizeof(struct IOSana2Req));
								p->read_io[i].ios2_Req.io_Message.mn_Node.ln_Name = (char *)p;	// Hide pointer to NetProtocol in node name
								p->read_io[i].ios2_Req.io_Message.mn_Node.ln_Type = NT_DEVICE;   // reset ln_type (just incase no reply)
								p->read_io[i].ios2_Req.io_Message.mn_ReplyPort = read_port;
								p->read_io[i].ios2_Req.io_Command = CMD_READ;
								p->read_io[i].ios2_PacketType = type;
								p->read_io[i].ios2_Data = p->read_buf[i];
								p->read_io[i].ios2_Req.io_Flags = SANA2IOF_RAW;
								BeginIO((struct IORequest *)(p->read_io + i));
							}

							// Add protocol to list
							AddTail(&prot_list, p);

							// Everything OK
							msg->result = noErr;
#ifdef stupid
						}
						else
						{
							printf("NET ERROR: No handler or not type defined, req ignored\n");
						}
#endif

						break;
					}

					case MSG_DETACH_PH:
					{
						uint16 type = msg->type;
						msg->result = lapProtErr;
						NetProtocol *p = (NetProtocol *)prot_list.lh_Head, *next;
						while ((next = (NetProtocol *)p->ln_Succ) != NULL) {
							if (p->type == type) {
								remove_protocol(p);
								msg->result = noErr;
								break;
							}
							p = next;
						}
						break;
					}

					case MSG_WRITE: {
						// Get pointer to Write Data Structure
						uint32 wds = msg->pointer;
						write_io->ios2_Data = (void *)wds;

						// Calculate total packet length
						long len = 0;
						uint32 tmp = wds;
						for (;;) {
							int16 w = ReadMacInt16(tmp);
							if (w == 0)
								break;
							len += w;
							tmp += 6;
						}
						write_io->ios2_DataLength = len;

						// Get destination address, set source address
						uint32 hdr = ReadMacInt32(wds + 2);
						Mac2Host_memcpy(write_io->ios2_DstAddr, hdr, 6);
						Host2Mac_memcpy(hdr + 6, ether_addr, 6);

						// Get packet type
						uint32 type = ReadMacInt16(hdr + 12);
						if (type <= 1500)
							type = 0;		// 802.3 packet
						write_io->ios2_PacketType = type;

						// Multicast/broadcard packet?
						if (write_io->ios2_DstAddr[0] & 1) {
							if (*(uint32 *)(write_io->ios2_DstAddr) == 0xffffffff && *(uint16 *)(write_io->ios2_DstAddr + 4) == 0xffff)
								write_io->ios2_Req.io_Command = S2_BROADCAST;
							else
								write_io->ios2_Req.io_Command = S2_MULTICAST;
						} else
							write_io->ios2_Req.io_Command = CMD_WRITE;

						if (type>0) printf("Send package type 0x%x\n",type);


						// Send packet
						write_done = false;
						write_io->ios2_Req.io_Message.mn_Node.ln_Type = NT_DEVICE; // remove reply_msg
						write_io->ios2_Req.io_Flags = SANA2IOF_RAW;
						BeginIO((IORequest *)write_io);
						break;
					}
				}
reply:			D(bug(" net_proc replying\n"));
				ReplyMsg(msg);
			}
		}

		// Packet received
		if (sig & read_mask)
		{
			D(bug(" packet received, triggering Ethernet interrupt\n"));
			SetInterruptFlag(INTFLAG_ETHER);
			TriggerInterrupt();
		}

		// Packet write completed
		if (sig & write_mask)
		{
			GetMsg(write_port);
			WriteMacInt32(ether_data + ed_Result, write_io->ios2_Req.io_Error ? excessCollsns : 0);
			write_done = true;

			Sana2ErrorMsg(write_io);

			printf(" packet write done, triggering Ethernet interrupt\n");
			SetInterruptFlag(INTFLAG_ETHER);
			TriggerInterrupt();
		}
	}
quit:
	printf("-- quiting eth --\n");
	// Close everything
	remove_all_protocols();
	if (opened)
	{
		if (CheckIO((struct IORequest *)write_io) == 0) {
			AbortIO((struct IORequest *)write_io);
			WaitIO((struct IORequest *)write_io);
		}
		CloseDevice((struct IORequest *)control_io);
	}


	printf("* deleting io ports\n");

	DeleteIORequest( (IORequest*) write_io);
	DeleteIORequest( (IORequest*) control_io);

	printf("* deleting io msgports\n");

	DeleteMsgPort(control_port);
	DeleteMsgPort(write_port);
	DeleteMsgPort(read_port);

	printf("* singal main task\n");

	printf("%x\n",MainTask);

	//Signal(MainTask, SIGF_SINGLE);

	net_proc_quit = 1;

	printf("-- quiting eth done --\n");
}


/*
 *  Ethernet interrupt - activate deferred tasks to call IODone or protocol handlers
 */

void EtherInterrupt(void)
{
	D(bug("EtherIRQ\n"));

	if (!ether_data)
	{
		D(bug("EtherInterrupt() Exit due ether_data = NULL\n"));
		return;
	}	

	D(bug("INFO -- ether_data %x\n",ether_data));
	D(bug("INFO -- ed_RHA %x\n", ed_RHA));

	// Packet write done, enqueue DT to call IODone
	if (write_done) {
		EnqueueMac(ether_data + ed_DeferredTask, 0xd92);
		write_done = false;
	}

	// Call protocol handler for received packets
	IOSana2Req *io;
	while ((io = (struct IOSana2Req *)GetMsg(read_port)))
	{

		// Get pointer to NetProtocol (hidden in node name)
		NetProtocol *p = (NetProtocol *)io->ios2_Req.io_Message.mn_Node.ln_Name;

		// No default handler
		if (p->handler == 0)
			continue;

		D(bug("IOS2_DATA ptr 0x%x (Amiga memory)\n",io->ios2_Data));

		// Copy header to RHA
		Host2Mac_memcpy(ether_data + ed_RHA, io->ios2_Data, 14);

		D(bug("*** header %08lx%04lx %08lx%04lx %04lx\n", 
			ReadMacInt32(ether_data + ed_RHA), 
			ReadMacInt16(ether_data + ed_RHA + 4), 
			ReadMacInt32(ether_data + ed_RHA + 6), 
			ReadMacInt16(ether_data + ed_RHA + 10), 
			ReadMacInt16(ether_data + ed_RHA + 12)));

	
		// Call protocol handler
		M68kRegisters r;
		r.d[0] =*(uint16 *) ((uint32) io->ios2_Data + 12) ;	// Packet type
		r.d[1] = io->ios2_DataLength - 18;		// Remaining packet length (without header, for ReadPacket) (-18 because the CRC is also included)
		r.a[0] =  (ULONG) ((uint32) io->ios2_Data + 14);	// Pointer to packet (host address, for ReadPacket)
		r.a[3] = ether_data + ed_RHA + 14;	// Pointer behind header in RHA
		r.a[4] = ether_data + ed_ReadPacket;	// Pointer to ReadPacket/ReadRest routines

		D(bug("*** calling protocol handler %08lx, type %08lx, length %08lx, data %08lx, rha %08lx, read_packet %08lx\n", p->handler, r.d[0], r.d[1], r.a[0], r.a[3], r.a[4]));

		printf("Execute68k handler\n");

		Execute68k(p->handler, &r);

		printf("Resend IORequest\n");

		// Resend IORequest
		io->ios2_Req.io_Message.mn_Node.ln_Type = NT_DEVICE; // remove reply_msg
		io->ios2_Req.io_Flags = SANA2IOF_RAW;
		BeginIO((struct IORequest *)io);
	}
	D(bug(" EtherIRQ done\n"));
}


void Sana2ErrorMsg(struct IOSana2Req *req)
{
  PutStr(" -- Error: ");
  switch(req->ios2_Req.io_Error)
    {
      case IOERR_OPENFAIL:
	PutStr("Device open failure");
	break;

      case IOERR_ABORTED:
	PutStr("IO Request aborted");
	break;

      case IOERR_NOCMD:
	PutStr("Command not supported");
	break;

      case IOERR_BADLENGTH:
	PutStr("Not a valid length");
	break;

      case IOERR_BADADDRESS:
	PutStr("Invalid address");
	break;

      case IOERR_UNITBUSY:
	PutStr("Unit is busy");
	break;

      case IOERR_SELFTEST:
	PutStr("Hardware failed self-test");
	break;

      case S2ERR_NO_ERROR:
	PutStr("No error (should not happen)");
	break;

      case S2ERR_NO_RESOURCES:
	PutStr("Resource allocation failure");
	break;

      case S2ERR_BAD_ARGUMENT:
	PutStr("Bad argument");
	break;

      case S2ERR_BAD_STATE:
	PutStr("Bad state");
	break;

      case S2ERR_BAD_ADDRESS:
	PutStr("Bad address");
	break;

      case S2ERR_MTU_EXCEEDED:
	PutStr("Maximum transfer length exceeded");
	break;

      case S2ERR_NOT_SUPPORTED:
	PutStr("Command not supported");
	break;

      case S2ERR_SOFTWARE:
	PutStr("Software error");
	break;

      case S2ERR_OUTOFSERVICE:
	PutStr("Driver is offline");
	break;

      default:
	Printf("%ld", req->ios2_Req.io_Error);
	break;
    }

  if(req->ios2_WireError != S2WERR_GENERIC_ERROR)
    {
      PutStr("\nWireError: ");

      switch(req->ios2_WireError)
	{
	  case S2WERR_GENERIC_ERROR:
	    PutStr("(no specific information)");
	    break;

	  case S2WERR_NOT_CONFIGURED:
	    PutStr("Unit is not configured");
	    break;

	  case S2WERR_UNIT_ONLINE:
	    PutStr("Unit is currently online");
	    break;

	  case S2WERR_UNIT_OFFLINE:
	    PutStr("Unit is currently offline");
	    break;

	  case S2WERR_ALREADY_TRACKED:
	    PutStr("Protocol is already being tracked");
	    break;

	  case S2WERR_NOT_TRACKED:
	    PutStr("Protocol is not being tracked");
	    break;

	  case S2WERR_BUFF_ERROR:
	    PutStr("Buffer management error");
	    break;

	  case S2WERR_SRC_ADDRESS:
	    PutStr("Source address error");
	    break;

	  case S2WERR_DST_ADDRESS:
	    PutStr("Destination address error");
	    break;

	  case S2WERR_BAD_BROADCAST:
	    PutStr("Broadcast error");
	    break;

	  case S2WERR_BAD_MULTICAST:
	    PutStr("Multicast error");
	    break;

	  case S2WERR_MULTICAST_FULL:
	    PutStr("Multicast address list full");
	    break;

	  case S2WERR_BAD_EVENT:
	    PutStr("Unsupported event class");
	    break;

	  case S2WERR_BAD_STATDATA:
	    PutStr("StatData failed sanity check");
	    break;

	  case S2WERR_IS_CONFIGURED:
	    PutStr("Attempt to configure twice");
	    break;

	  case S2WERR_NULL_POINTER:
	    PutStr("Null pointer");
	    break;

	  default:
	    Printf("%ld", req->ios2_WireError);
	    break;
	}
    }
  PutStr("\n");
}
