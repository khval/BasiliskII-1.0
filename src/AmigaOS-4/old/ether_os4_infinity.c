/*
 *  ether_dummy.cpp - Ethernet device driver, dummy implementation
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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"
#include "macos_util.h"
#include "prefs.h"
#include "user_strings.h"
#include "ether.h"
#include "ether_defs.h"

#include <proto/exec.h>
#include <proto/dos.h> 
#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>


#include "/infinity/includes/link_layer.h"

#include "/infinity/includes/protocol_api.h"

//#include "/infinity/includes/ip4.h"
//#include "/infinity/includes/ip6.h"


void show_mac(char *m);

#define DEBUG 0

#include "debug.h"

#define MONITOR 1

#define ntohs(n) n

struct List		multicast_list;

struct mutlicast_node 
{
	struct Node node;
	char mac[6];
};

struct MsgPort  *link_port;
struct MsgPort	*device_port = NULL;
struct Task	*receive_task =NULL;
struct Task 	*me = NULL;
int			wakeup =-1 ; 

struct eth_inerface	*eth = NULL;
struct List		prot_list;
struct network_device_api device;

char *device_name = NULL ;
char fake_mac[]={0xFE,0xED,0xF0,0x0D,0x00,0x00};
char broadcast_mac[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

char link_layer_name[] = "network_link_layer";

char *new_device_name()
{
	int n = 1;
	struct MsgPort *q;
	char *device_name = NULL;

	if ( device_name = (char*) malloc(100))
	{
		do
		{
			sprintf(device_name,"BasiliskII-Eth%d",n);
			fake_mac[5] = (char) n;
			n++;

			Forbid();
			q = FindPort( device_name );
			Permit();
		}
		while (q);
	}

	printf("device name: %s\n",device_name);
	show_mac(fake_mac);

	return device_name;
}

static void *receive_func(void *arg);


int int_ack;


int is_symbol(char c)
{
	char *p;
	char symbols[]="/'!#%&()=+-\"\\_,.;:* ";

	for (p = symbols; *p != 0; p++)
	{
		if (*p == c) return 1;
	}

	return 0;
}

void hex_dump(char *d, int size)
{
	char asc[32];

	int n,p;

	asc[0]=0;

	for (n = 0;n< size  ; n++ )
	{
		if (( (n % 12) == 0 ) && (n>0) )
		{
			printf(" %s\n",asc);
		}

		if ( (n % 12) == 0 )
		{
			printf("%06d ",n);
		}

		printf("%02X ", d[n]);

		if ((d[n] >= '0')&&(d[n]<='9'))
		{
			asc[ n % 12 ] = d[n];
		}
		else if ((d[n] >= 'a')&&(d[n]<='z'))
		{
			asc[ n % 12 ] = d[n];
		}
		else if ((d[n] >= 'A')&&(d[n]<='Z'))
		{
			asc[ n % 12 ] = d[n];
		}
		else if ( is_symbol(d[n]) )
		{
			asc[ n % 12 ] = d[n];
		}
		else
		{
			asc[ n % 12 ] = '.';
		}

		asc[ (n % 12) + 1] = 0;
	}

	if (size>0)
	{
		if ((n%12)!=0) for (p = 0;p < (12-(n % 12));p++) printf("__ ", d[n]); 
		
		printf(" %s\n",asc);
	}
}

struct NetProtocol
{
	struct Node node;
	int type;
	ULONG handler;
}  ;

void show_mac(char *m)
{
	char buff[200];

	int n,cnt;

	cnt = 0;

	sprintf(buff,"");
	for (n=0;n<6;n++) { sprintf(buff,"%s%02X",buff,m[n]);  }

	printf("Mac: %s\n",buff);
}


int check_mac(char *p,char *m)
{
	int n,cnt;

	cnt = 0;
	for (n=0;n<6;n++) {if ( m[n] == p[n] ) cnt++; }
	return (cnt ==6);
}

int set_broadcast_mac(char *p)
{
	int n;
	for (n=0;n<6;n++) {p[n] = 255; }
}

int mac_filtering ( void *eth_ptr, struct Node *node)
{
	struct eth_inerface	*eth = (struct eth_inerface *) eth_ptr;
	unsigned char		*pkt = ((unsigned char *) node) + sizeof(struct pkg_node) ;

	struct Node *next ;
	struct Node *mnode;
	struct mutlicast_node *m;
	int n,cnt;

	// comes from this device?
	if (check_mac( eth -> mac , (char *) pkt + 6 )) return 0;

	// is this device?
	if (check_mac( eth -> mac , (char *) pkt )) return 1;

	// is broadcast mac?
	if (check_mac( broadcast_mac, (char *) pkt) ) return 1;

	printf(" pkt: ");
	for (n=0;n<6;n++) printf("%02X", pkt[n]);
	printf("\n");

	mnode = GetHead(&multicast_list);
	while (mnode) {
		next = GetSucc( mnode ) ;
		m = 	(struct mutlicast_node *) mnode;

		printf("multicast: ");
		for (n=0;n<6;n++) printf("%02X", m -> mac[n]);
		printf("\n");

		if ( check_mac( m -> mac, (char *) pkt ) ) return 1;
		mnode = next;
	}

	return 0;
}

void close_device()
{
//	quit = 1;
}

void link_confirms_quit(void)
{
	Signal( me ,1<<wakeup);	
}

int send_raw_package(struct MsgPort *port,char *raw_package, int package_size )
{
	struct link_msg		*send_msg;
	struct pkg_node	*pkg;
	char				*p;

	printf("packsize whit mac %d\n",package_size);
	printf("node size %d\n",sizeof(struct pkg_node));

	if (package_size>0)
	{
		if (pkg = (struct pkg_node *) IExec -> AllocSysObjectTags(ASOT_NODE
			,ASOMSG_Size, sizeof(struct pkg_node) + package_size
			,TAG_DONE))
		{
			bcopy( raw_package , ((char *) pkg) +sizeof(struct pkg_node), package_size );

			p = ((char *) pkg) +sizeof(struct pkg_node);

			// I fix mac address on outgoing, they will return correctly
			// I'm thinking the local mac can fixed by using it as gateway for mac?

			pkg -> size = package_size;

			if (send_msg = (struct link_msg *) IExec -> AllocSysObjectTags(ASOT_MESSAGE
				,ASOMSG_Size, sizeof(struct link_msg)
				,TAG_DONE))
			{
				send_msg -> type = type_raw_package;
				send_msg -> pkg_node = pkg;
				PutMsg(  port, (struct Message *) send_msg );

				return 0;
			}
		}
	}

	return -1;
}


/*
 *  Find protocol in list
 */

static NetProtocol *find_protocol(uint16 type)
{
	struct NetProtocol *node;
	struct NetProtocol *next;

	// All 802.2 types are the same
	if (type <= 1500)
		type = 0;

	// Search list (we could use hashing here but there are usually only three
	// handlers installed: 0x0000 for AppleTalk and 0x0800/0x0806 for TCP/IP)

	 for( node = (struct NetProtocol *) GetHead(&prot_list) ;node != NULL ; ) 
	{
		next = (struct NetProtocol *) GetSucc( (struct Node *) node );

		printf("find %04x, protocol type %04x\n",type,  node  ->type );

		if ( node  ->type == type)
			return  node;

		node = next;
	}


	return NULL;
}


/*
 *  Remove all eth->protocols
 */

static void remove_all_protocols(void)
{
	struct Node *next ;
	struct Node *node = GetHead(&prot_list) ;

	printf("remove_all_protocols(void)\n");

	while (node) {
		next = GetSucc( node ) ;
		Remove(node);
		node = next;
	}

	printf("remove_all_protocols(void) done\n");

}

/*
 *  Initialization
 */

void send_msg(struct MsgPort	*port, int type, struct pkg_node *pkg )
{
	struct link_msg		*msg;

	msg = (struct link_msg *) IExec->AllocSysObjectTags(ASOT_MESSAGE
		,ASOMSG_Size, sizeof(struct link_msg)
		,TAG_DONE);

	if  (msg)
	{
		msg -> type = type;
		msg -> pkg_node = pkg;
		PutMsg(  port,  (struct Message *) msg );
	}
}


void EtherInit(void)
{
	printf("** EtherInit(void) **\n");

	net_open = false; 		// default value

	printf("setup lists\n");

	NewList(&prot_list);
	NewList(&multicast_list);

	printf("find link layer\n");

//	Forbid();
	link_port = FindPort( link_layer_name );
//	Permit();

	printf("set device name\n");

	device_name = new_device_name();

	printf("device name ready\n");

	if  ((device_name)&&(link_port))
	{
		printf("setup network process...\n");

		wakeup = AllocSignal( -1 );
		me = FindTask( NULL );


		CreateNewProcTags(
			NP_Entry, (ULONG) receive_func ,
			NP_Name, (ULONG) "Basilisk II Ethernet - receive task",
			NP_Priority, 1,
			TAG_END	
		);

		// wait for process to be ready.
		Wait(1 << wakeup);

		device.close_device = close_device;
		device.link_confirms_quit = link_confirms_quit;

		eth = (struct eth_inerface *) IExec -> AllocSysObjectTags(ASOT_NODE
				,ASONODE_Size, sizeof( struct eth_inerface )
				,TAG_DONE);

		if (eth)
		{		
			// defaults
			eth->protocols = 0;
			eth -> device = &device;

			if (eth -> protocol[eth->protocols]	= (struct protocol_api *) get_protocol( (char *) "ipv4", wakeup))
			{
				eth->protocol[eth->protocols] -> load_parm(device_name,(char *) "ipv4",&eth -> parm[eth->protocols]);
				eth->protocols++;
			}

			eth -> protocol[eth->protocols]	= NULL;
			eth -> parm[eth->protocols].data	= NULL;

			eth -> port		= device_port;
			eth -> mac		= fake_mac;
			eth -> mac_filtering	= mac_filtering;

			printf("found %d eth->protocols\n",eth->protocols);

			// send device config
			send_msg( link_port , net_interface_add , (struct pkg_node *) eth );

			memcpy( ether_addr, fake_mac, 6);
			net_open = true;
		}
	}
	else
	{
		printf("Ethernet disabled, link layer not found\n");

	}
}


/*
 *  Deinitialization
 */

void EtherExit(void)
{
	printf("** EtherExit(void) ** START\n");	

	// send signal to quit recive task 
	if (receive_task) Signal( receive_task, SIGBREAKF_CTRL_C);


	printf("send a msg to link_port\n");

	if (FindPort( "network_link_layer" ))
	{
		if (eth) { send_msg( link_port , net_interface_remove , (struct pkg_node *) eth ); }

		printf("** EtherExit(void) ** WAIT FOR LINK LAYER \n");
		Wait( 1 << wakeup );	// we get wakeup from link layer when its done.
	}

	printf("Free device_port\n");

	if (device_port)	FreeSysObject( ASOT_PORT, device_port );


	printf("Free wakeup signal\n");

	if (wakeup)	FreeSignal(wakeup);

	printf("Remove protocals\n");

	// Remove all eth->protocols
	remove_all_protocols();

	printf("** EtherExit(void) ** END\n");
}


/*
 *  Reset
 */

void EtherReset(void)
{
	remove_all_protocols();
	printf("** EtherReset(void) **\n");
}


/*
 *  Add multicast address
 */

int16 ether_add_multicast(uint32 pb)
{
	char *dat;
	char *packet;
	struct mutlicast_node *m;
	int n;

	printf("** ether_add_multicast ** pd %x\n",pb);

	dat = (char *) Mac2HostAddr(pb + eMultiAddr);

	packet = (char *) Mac2HostAddr(pb);

	printf("Multicast: %0X %0X %0X %0X %0X %0X\n",dat[0],dat[1],dat[2],dat[3],dat[4],dat[5]);
	printf("eMultiAddr offset: %d\n",eMultiAddr);

	m = (struct mutlicast_node *) IExec->AllocSysObjectTags(ASOT_NODE
		,ASOMSG_Size, sizeof(struct mutlicast_node)
		,TAG_DONE);

	for (n=0;n<6;n++) m->mac[n] = dat[n];

	AddHead( &multicast_list, (struct Node *) m );

#if MONITOR
	bug("Ethernet packet:\n");
	hex_dump( packet, 1516);
#endif

	return noErr;
}


/*
 *  Delete multicast address
 */

int16 ether_del_multicast(uint32 pb)
{
	struct Node *mnode = GetHead(&multicast_list) ;
	struct Node *next;
	struct mutlicast_node *m;
	int n,cnt;
	char *dat;

	printf("** ether_del_multicast ** pd %x\n",pb);

	dat = (char *) Mac2HostAddr(pb + eMultiAddr);

	while (mnode)
	{
		next = GetSucc( mnode ) ;
		
		m = (struct mutlicast_node *)  mnode;

		if ( check_mac( m -> mac , dat ) ) 
		{
			Remove( (struct Node *) mnode );
			break;
		}

		mnode = next;
	}

	return noErr;
}


/*
 *  Attach protocol handler
 */

int16 ether_attach_ph(uint16 type, uint32 handler)
{

	printf("** ether_attach_ph ** type %x handler %x\n",type,handler);


	// Already attached?
	NetProtocol *p = find_protocol(type);

	if (p != NULL)
		return lapProtErr;
	else {

	if (type != 0x0024)	// I don't know what it is, I just hate this packages!!
	{

		// No, create and attach

		p = (struct NetProtocol *) IExec->AllocSysObjectTags(ASOT_NODE
			,ASOMSG_Size, sizeof(struct NetProtocol)
			,TAG_DONE);

		p->type = type;
		p->handler = handler;

		printf("** ether_attach_ph ** type %x handler %x\n",p -> type,p -> handler);

		AddHead( &prot_list, (struct Node *) p );

	}

		return noErr;
	}

	return noErr;
}


/*
 *  Detach protocol handler
 */

int16 ether_detach_ph(uint16 type)
{
	printf("** ether_detach_ph ** type %x\n",type);

	NetProtocol *p = find_protocol(type);

	if (p)
	{
		Remove( (struct Node *) p);
		FreeSysObject( ASOT_NODE, p );
	}

	return noErr;
}


/*
 *  Transmit raw ethernet packet
 */

int16 ether_write(uint32 wds)
{
	// Set source address
	uint32 hdr = ReadMacInt32(wds + 2);
	Host2Mac_memcpy(hdr + 6, ether_addr, 6);

	// Copy packet to buffer
	uint8 packet[1516], *p = packet;
	int len = 0;
#if defined(__linux__)
	if (is_ethertap) {
		*p++ = 0;	// Linux ethertap discards the first 2 bytes
		*p++ = 0;
		len += 2;
	}
#endif
	for (;;) {
		int w = ReadMacInt16(wds);
		if (w == 0)
			break;
		Mac2Host_memcpy(p, ReadMacInt32(wds + 2), w);
		len += w;
		p += w;
		wds += 6;
	}

#if MONITOR
	printf("ether_write\n");
	hex_dump( (char *) packet, len );
#endif

	// Transmit packet
	if (send_raw_package( link_port, (char*) packet, len) < 0) {
		D(bug("WARNING: Couldn't transmit packet\n"));
		return excessCollsns;
	} else
		return noErr;
}

/*
 *  Packet reception thread
 */

static void *receive_func(void *arg)
{
	int sig_receive =0;
	int sig_ack = 0;
	int quit_receive = 0 ;

	receive_task = FindTask( NULL );

	device_port = (struct MsgPort *)  IExec->AllocSysObjectTags(ASOT_PORT
					,ASOPORT_Name, device_name 
					,TAG_DONE);

	int_ack = AllocSignal( -1 );
	if (int_ack>-1) sig_ack =  1 << int_ack;

	sig_receive =  1 << device_port -> mp_SigBit ;
	sig_receive |= SIGBREAKF_CTRL_C;

	Signal( me ,1<<wakeup);	// wakeup main app

	while ( !quit_receive ) {

		if ( Wait( sig_receive ) & SIGBREAKF_CTRL_C) 
		{
			quit_receive = 1;
		}
		else
		{
			SetInterruptFlag(INTFLAG_ETHER);
			TriggerInterrupt();

			// Wait for interrupt to be acknowledge by EtherInterrupt()
			 Wait( sig_ack );
		}
	}

	if (int_ack>-1) FreeSignal(int_ack);	

	return NULL;
}

/*
 *  Ethernet interrupt - activate deferred tasks to call IODone or protocol handlers
 */

void EtherInterrupt(void)
{
	struct link_msg	 *msg;

	D(bug("EtherIRQ\n"));

	// Call protocol handler for received packets
	uint8 packet[1516];

	while (msg = (struct link_msg *) GetMsg( device_port ))
	{
		// Read packet from sheep_net device

		ssize_t length =msg -> pkg_node -> size;

		if (length >= 14) bcopy( ((char *) msg -> pkg_node) +sizeof(struct pkg_node), packet , length );

		FreeSysObject( ASOT_NODE, msg -> pkg_node );
		ReplyMsg( (struct Message *)  msg );

		if (length < 14)
		{
			printf("ERROR: package length is %d, package ignored (14 or more ok)\n",length);
			break;
		}

#if MONITOR
		bug("Receiving Ethernet packet:\n");
		hex_dump( (char *) packet,length);
#endif

		// Pointer to packet data (Ethernet header)
		uint8 *p = packet;

		// Get packet type
		uint16 type = ntohs(*(uint16 *)(p + 12));

		// Look for protocol
		NetProtocol *prot = find_protocol(type);

		if (check_mac( (char *) p + 6, fake_mac ) )
		{
			printf("fuck kjetil your link layer is broken ;-)!!\n");
			printf("stop the loop infinity shit\n");
			continue;
		}

		printf("found protocol? %s\n",  (prot->handler == 0) ? "No" : "Yes" );

		if (prot == NULL)
			continue;

		printf("have handler? %s\n",  (prot->handler == 0) ? "No" : "Yes" );

		// No default handler
		if (prot->handler == 0)
			continue;

		// Copy header to RHA
		Host2Mac_memcpy(ether_data + ed_RHA, p, 14);

		D(bug(" header %08lx%04lx %08lx%04lx %04lx\n", ReadMacInt32(ether_data + ed_RHA), ReadMacInt16(ether_data + ed_RHA + 4), ReadMacInt32(ether_data + ed_RHA + 6), ReadMacInt16(ether_data + ed_RHA + 10), ReadMacInt16(ether_data + ed_RHA + 12)));

		printf("executing 68k handler done\n");

		// Call protocol handler
		M68kRegisters r;
		r.d[0] = type;								// Packet type
		r.d[1] = length - 14;							// Remaining packet length (without header, for ReadPacket)
		r.a[0] = (uint32)p + 14;						// Pointer to packet (host address, for ReadPacket)
		r.a[3] = ether_data + ed_RHA + 14;				// Pointer behind header in RHA
		r.a[4] = ether_data + ed_ReadPacket;			// Pointer to ReadPacket/ReadRest routines
		D(bug(" calling protocol handler %08lx, type %08lx, length %08lx, data %08lx, rha %08lx, read_packet %08lx\n", prot->handler, r.d[0], r.d[1], r.a[0], r.a[3], r.a[4]));
		Execute68k(prot->handler, &r);

		printf("68k hander done\n");
	}

	// Acknowledge interrupt to reception thread
	D(bug(" EtherIRQ done\n"));

	Signal( receive_task ,1<<int_ack);	

	printf("exit interupt.\n");
}
