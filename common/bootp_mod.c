/*
 * bootp.c -- Receive kernel image via BOOTP/TFTP
 *
 * Copyright 1996-97 by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 * 
 * $Id: bootp_mod.c,v 1.4 1998-02-26 10:01:04 rnhodek Exp $
 * 
 * $Log: bootp_mod.c,v $
 * Revision 1.4  1998-02-26 10:01:04  rnhodek
 * Handle "bootp:" prefix from Lilo.
 * Don't print error messages about Ethernet packages from a wrong server
 * or port; these are no real errors, they can be caused by normal net activity.
 *
 * Revision 1.3  1997/07/18 11:07:07  rnhodek
 * Added sfilesize() call & Co. to streams
 *
 * Revision 1.2  1997/07/16 15:06:22  rnhodek
 * Replaced all call to libc functions puts, printf, malloc, ... in common code
 * by the capitalized generic function/macros. New generic function ReAlloc, need
 * by load_ramdisk.
 *
 * Revision 1.1.1.1  1997/07/15 09:45:37  rnhodek
 * Import sources into CVS
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bootstrap.h"
#include "bootp.h"
#include "stream.h"

/* --------------------------------------------------------------------- */
/*						  Protocol Header Structures					 */

struct etherhdr {
	HWADDR			dst_addr;
	HWADDR			src_addr;
	unsigned short	type;
};

struct arphdr {
	unsigned short	hrd;		/* format of hardware address	*/
	unsigned short	pro;		/* format of protocol address	*/
	unsigned char	hln;		/* length of hardware address	*/
	unsigned char	pln;		/* length of protocol address	*/
	unsigned short	op;			/* ARP opcode (command)			*/
	unsigned char	addr[0];	/* addresses (var len)			*/
};
	
struct iphdr {
	unsigned char	version : 4;
	unsigned char	ihl : 4;
	unsigned char	tos;
	unsigned short	tot_len;
	unsigned short	id;
	unsigned short	frag_off;
	unsigned char	ttl;
	unsigned char	protocol;
	unsigned short	chksum;
	IPADDR			src_addr;
	IPADDR			dst_addr;
};

struct udphdr {
	unsigned short	src_port;
	unsigned short	dst_port;
	unsigned short	len;
	unsigned short	chksum;
};

struct bootp {
	unsigned char	op;			/* packet opcode type */
	unsigned char	htype;		/* hardware addr type */
	unsigned char	hlen;		/* hardware addr length */
	unsigned char	hops;		/* gateway hops */
	unsigned long	xid;		/* transaction ID */
	unsigned short	secs;		/* seconds since boot began */
	unsigned short	unused;
	IPADDR			ciaddr;		/* client IP address */
	IPADDR			yiaddr;		/* 'your' IP address */
	IPADDR			siaddr;		/* server IP address */
	IPADDR			giaddr; 	/* gateway IP address */
	unsigned char	chaddr[16];	/* client hardware address */
	unsigned char	sname[64];	/* server host name */
	unsigned char	file[128];	/* boot file name */
	unsigned char	vend[64];	/* vendor-specific area */
};

#define TFTP_PKTSIZE	512

struct tftp_req {
	unsigned short	opcode;
	char			name[TFTP_PKTSIZE];
};

struct tftp_data {
	unsigned short	opcode;
	unsigned short	nr;
	unsigned char	data[TFTP_PKTSIZE];
};

struct tftp_ack {
	unsigned short	opcode;
	unsigned short	nr;
};

struct tftp_error {
	unsigned short	opcode;
	unsigned short	errcode;
	char			str[TFTP_PKTSIZE];
};


typedef struct {
	struct etherhdr		ether;
	struct arphdr		arp;
} ARP;

typedef struct {
	struct etherhdr		ether;
	struct iphdr		ip;
	struct udphdr		udp;
} UDP;

#define	UDP_BOOTPS	67
#define	UDP_BOOTPC	68
#define	UDP_TFTP	69

typedef struct {
	struct etherhdr		ether;
	struct iphdr		ip;
	struct udphdr		udp;
	struct bootp		bootp;
} BOOTP;

#define	BOOTREQUEST		1
#define	BOOTREPLY		2
#define	BOOTP_RETRYS	5

typedef struct {
	struct etherhdr		ether;
	struct iphdr		ip;
	struct udphdr		udp;
	union tftp {
		unsigned short		opcode;
		struct tftp_req		req;
		struct tftp_data	data;
		struct tftp_ack		ack;
		struct tftp_error	error;
	} tftp;
} TFTP;
	
#define	TFTP_RRQ	1
#define	TFTP_WRQ	2
#define	TFTP_DATA	3
#define	TFTP_ACK	4
#define	TFTP_ERROR	5


/* --------------------------------------------------------------------- */
/*								  Addresses								 */

static HWADDR	MyHwaddr;
static HWADDR	ServerHwaddr;
static IPADDR	MyIPaddr;
static IPADDR	ServerIPaddr;

static IPADDR	IP_Unknown_Addr =   0x00000000;
static IPADDR	IP_Broadcast_Addr = 0xffffffff;
static HWADDR 	Eth_Broadcast_Addr = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


#define	HZ	200
#define	_hz_200	(*(volatile unsigned long *)0x4ba)

static int bootp_haveaddr = 0;	/* do we already know IP addrs? */
static int addr_printed = 0;	/* printed IP addresses */

int no_bootp = 0;

/* --------------------------------------------------------------------- */
/*								Error Strings							 */

static char *ErrStr[] = {
	"timeout",
	"general Ethernet transmit error",
	"general Ethernet receive error",
	"Ethernet framing error",
	"Ethernet overflow error",
	"Ethernet CRC error"
};


/* --------------------------------------------------------------------- */
/*					 Ethernet Interface Abstraction Layer				 */


#ifdef ETHLL_LANCE
#include "ethlance.h"
#endif

static ETHIF_SWITCH *PossibleInterfaces[] = {
#ifdef ETHLL_LANCE
	&LanceSwitch,
#endif
};

#define	N_PossibleInterfaces (sizeof(PossibleInterfaces)/sizeof(*PossibleInterfaces))

/* Detected interface */
static ETHIF_SWITCH *Ethif = NULL;

static __inline__ int eth_send( Packet *pkt, int len )
	{ return( Ethif->snd( pkt, len )); }
static __inline__ int eth_rcv( Packet *pkt, int *len )
	{ return( Ethif->rcv( pkt, len )); }


/***************************** Prototypes *****************************/

static int bootp_open( const char *name );
static long bootp_fillbuf( void *buf );
static int bootp_close( void );
static int bootp( char *image_name );
static int tftp_start( char *image_name );
static int tftp_rcv( char *buf );
static int udp_send( UDP *pkt, int len, int fromport, int toport );
static unsigned short ip_checksum( struct iphdr *buf );
static int udp_rcv( UDP *pkt, int *len, int fromport, int atport );
static void print_ip( IPADDR addr );
static void print_hw( HWADDR addr );
static int check_ethif( void );

/************************* End of Prototypes **************************/

/* --------------------------------------------------------------------- */
/*							 The module strcuture						 */

MODULE bootp_mod = {
	"bootp",					/* name */
	TFTP_PKTSIZE,				/* max. 512 bytes per TFTP packet */
	bootp_open,
	bootp_fillbuf,
	NULL,						/* cannot skip */
	bootp_close,
	NULL,						/* can't determine size of file at open time */
	MOD_REST_INIT
};



/* --------------------------------------------------------------------- */
/*						   Interface to bootstrap.c						 */


static int bootp_open( const char *name )
{
	int rv;
	char image_name[256];

	if (no_bootp)
		goto try_next;

#ifdef IN_LILO
	/* inside Lilo, only use BOOTP if the filename starts with "bootp:" */
	if (strncmp( name, "bootp:", 6 ) != 0)
		goto try_next;
	name += 6;
#else
	/* "local:" prefix in name means use local file, skip BOOTP */
	if (strncmp( name, "local:", 6 ) == 0)
		goto try_next;
#endif

	/* Check if a Ethernet interface is present and determine the Ethernet
	 * address */
	if (check_ethif() < 0) {
		Printf( "No Ethernet interface found -- no remote boot possible.\n" );
		goto try_next;
	}

	strcpy( image_name, name );
	/* Replace the default name "vmlinux" by an empty name; the BOOTP server
	 * will replace this by its default kernel for us */
	if (strcmp( image_name, "vmlinux" ) == 0)
		*image_name = 0;
	
	/* Do a BOOTP request to find out our IP address and the kernel image's
	 * name; we also learn the IP and Ethernet address of our server */
	if (bootp( image_name ) < 0)
		goto try_next;
	
	/* Now start a TFTP connection to receive the kernel image */
	if (tftp_start( image_name ) < 0)
		goto try_next;
	return( 0 );

  try_next:
	rv = sopen( name );
	return( rv < 0 ? rv : 1 );
}

static long bootp_fillbuf( void *buf )
{
	return( tftp_rcv( buf ) );
}

static int bootp_close( void )
{
	char buf[TFTP_PKTSIZE];
	
	/* must skip rest of data if EOF not yet reached */
	while( tftp_rcv( buf ) > 0 )
		;
	return( 0 );
}

/* --------------------------------------------------------------------- */
/*							   BOOTP Procedure							 */


static int bootp( char *image_name )
{
	BOOTP	req;
	Packet	_reply;
	BOOTP	*reply = (BOOTP *)_reply;
	static unsigned char mincookie[] = { 99, 130, 83, 99, 255 };
	unsigned long 	starttime, rancopy;
	int				err, len, retry;
	
	memset( (char *)&req, 0, sizeof(req) );
	/* Now fill in the packet... */
	req.bootp.op = BOOTREQUEST;
	req.bootp.htype = 1; /* 10Mb/s Ethernet */
	req.bootp.hlen = 6;
	memcpy( req.bootp.chaddr, &MyHwaddr, ETHADDRLEN );

	/* Put in the minimal RFC1497 Magic cookie */
	memcpy( req.bootp.vend, mincookie, sizeof(mincookie) );
	/* Put the user precified bootfile name in place */
	memcpy( req.bootp.file, image_name, strlen(image_name)+1);

	starttime = _hz_200;
	for( retry = 0; retry < BOOTP_RETRYS; ++retry ) {

		if (!bootp_haveaddr) {
			/* Initialize server addresses and own IP to defaults */
			ServerIPaddr = IP_Broadcast_Addr;  /* 255.255.255.255 */
			MyIPaddr     = IP_Unknown_Addr;    /* 0.0.0.0 */
			memcpy( ServerHwaddr, Eth_Broadcast_Addr, ETHADDRLEN );
		}
		
		if (retry)
			sleep( 3 );
		
		req.bootp.xid = rancopy = _hz_200;
		req.bootp.secs = (_hz_200 - starttime) / HZ;

		if ((err = udp_send( (UDP *)&req, sizeof(req.bootp),
							 UDP_BOOTPC, UDP_BOOTPS )) < 0) {
			Printf( "bootp send: %s\n", ErrStr[-err-1] );
			continue;
		}
		
		if ((err = udp_rcv( (UDP *)reply, &len,
							UDP_BOOTPS, UDP_BOOTPC )) < 0) {
			Printf( "bootp rcv: %s\n", ErrStr[-err-1] );
			continue;
		}
		if (len < sizeof(struct bootp)) {
			Printf( "received short BOOTP packet (%d bytes)\n", len );
			continue;
		}

		if (reply->bootp.xid == rancopy)
			/* Ok, got the answer */
			break;
		Printf( "bootp: xid mismatch\n" );
	}
	if (retry >= BOOTP_RETRYS) {
		Printf( "No response from a bootp server\n" );
		return( -1 );
	}
	
	ServerIPaddr = reply->bootp.siaddr;
	MyIPaddr = reply->bootp.yiaddr;
	memcpy( ServerHwaddr, reply->ether.src_addr, ETHADDRLEN );
	bootp_haveaddr = 1;
	
	if (!addr_printed) {
		Printf( "\nBoot server is " );
		if (strlen(reply->bootp.sname) > 0)
			Printf( "%s, IP ", reply->bootp.sname );
		print_ip( ServerIPaddr );
		Printf( ", HW address " );
		print_hw( ServerHwaddr );
		Printf( "\n" );

		Printf( "My IP address is " );
		print_ip( MyIPaddr );
		Printf( "\n" );

		addr_printed = 1;
	}

	strcpy( image_name, reply->bootp.file );
	return( 0 );
}


/* --------------------------------------------------------------------- */
/*								TFTP Procedure							 */

/* global vars for TFTP state */
static int tftp_blk;							/* block number */
static unsigned short tftp_mytid, tftp_rtid;	/* transfer IDs */
static int tftp_eof;							/* EOF indicator */
static char tftp_buf[TFTP_PKTSIZE];	/* TFTP buffer for first packet */
static int tftp_bufsize;			/* number of data in TFTP buffer */

/* start a TFTP session */
static int tftp_start( char *image_name )
{
	TFTP spkt;
	int retries, err;
	
	retries = 5;
	/* Construct and send a read request */
  repeat_req:
	spkt.tftp.req.opcode = TFTP_RRQ;
	strcpy( spkt.tftp.req.name, image_name );
	strcpy( spkt.tftp.req.name + strlen(spkt.tftp.req.name) + 1, "octet" );
	tftp_mytid = _hz_200 & 0xffff;
	
	if ((err = udp_send( (UDP *)&spkt, sizeof(spkt.tftp.req.opcode) +
									   strlen(image_name) + 1 +
									   strlen( "octect" ) +1,
						 tftp_mytid, UDP_TFTP )) < 0) {
		Printf( "TFTP RREQ: %s\n", ErrStr[-err-1] );
		if (--retries > 0)
			goto repeat_req;
		return( -1 );
	}

	/* Init global TFTP vars */
	tftp_rtid = tftp_eof = 0;
	tftp_blk = 1;
	
	/* must wait for one answer here; this is either the first data package or
	 * an error from the server */
	tftp_bufsize = -1;
	if ((tftp_bufsize = tftp_rcv( tftp_buf )) < 0)
		return( -1 );

	Printf( "Receiving file %s\n", image_name );
	return( 0 );
}
	
/* receive a TFTP packet */
static int tftp_rcv( char *buf )
{
	TFTP spkt;
	Packet _rpkt;
	TFTP *rpkt = (TFTP *)&_rpkt;
	int retries, err, len, datalen;

	if (tftp_eof)
		return( 0 );
	if (tftp_bufsize >= 0) {
		int n = tftp_bufsize;
		tftp_bufsize = -1;
		memcpy( buf, tftp_buf, n );
		return( n );
	}
	
	retries = 5;
  repeat_data:
	if ((err = udp_rcv( (UDP *)rpkt, &len, tftp_rtid, tftp_mytid )) < 0) {
		Printf( "TFTP rcv: %s\n", ErrStr[-err-1] );
		if (--retries > 0)
			goto repeat_data;
		return( -1 );
	}
	if (tftp_rtid == 0)
		/* Store the remote port at the first packet received */
		tftp_rtid = rpkt->udp.src_port;
	
	if (rpkt->tftp.opcode == TFTP_ERROR) {
		if (strlen(rpkt->tftp.error.str) > 0)
			Printf( "TFTP error: %s\n", rpkt->tftp.error.str );
		else
			Printf( "TFTP error #%d (no description)\n",
					rpkt->tftp.error.errcode );
		return( -1 );
	}
	else if (rpkt->tftp.opcode != TFTP_DATA) {
		Printf( "Bad TFTP packet type: %d\n", rpkt->tftp.opcode );
		if (--retries > 0)
			goto repeat_data;
		return( -1 );
	}

	if (rpkt->tftp.data.nr != tftp_blk) {
		/* doubled data packet; ignore it */
		goto repeat_data;
	}
	datalen = len - sizeof(rpkt->tftp.data.opcode) -
			  sizeof(rpkt->tftp.data.nr);
	
	/* store data */
	if (datalen > 0)
		memcpy( buf, rpkt->tftp.data.data, datalen );
		
	/* Send ACK packet */
  repeat_ack:
	spkt.tftp.ack.opcode = TFTP_ACK;
	spkt.tftp.ack.nr = tftp_blk;
	if ((err = udp_send( (UDP *)&spkt, sizeof(spkt.tftp.ack),
						 tftp_mytid, tftp_rtid )) < 0) {
		Printf( "TFTP ACK: %s\n", ErrStr[-err-1] );
		if (--retries > 0)
			goto repeat_ack;
		return( -1 );
	}
	
	++tftp_blk;
	if (datalen < TFTP_PKTSIZE)
		/* This was the last packet */
		tftp_eof = 1;

	return( datalen );
}



/* --------------------------------------------------------------------- */
/*				  UDP/IP Protocol Quick Hack Implementation				 */


static int udp_send( UDP *pkt, int len, int fromport, int toport )

{
	/* UDP layer */
	pkt->udp.src_port = fromport;
	pkt->udp.dst_port = toport;
	pkt->udp.len      = (len += sizeof(struct udphdr));
	pkt->udp.chksum   = 0; /* Too lazy to calculate :-) */

	/* IP layer */
	pkt->ip.version  = 4;
	pkt->ip.ihl      = 5;
	pkt->ip.tos      = 0;
	pkt->ip.tot_len  = (len += sizeof(struct iphdr));
	pkt->ip.id       = 0;
	pkt->ip.frag_off = 0;
	pkt->ip.ttl      = 255;
	pkt->ip.protocol = 17; /* UDP */
	pkt->ip.src_addr = MyIPaddr;
	pkt->ip.dst_addr = ServerIPaddr;
	pkt->ip.chksum   = 0;
	pkt->ip.chksum   = ip_checksum( &pkt->ip );

	/* Ethernet layer */
	memcpy( &pkt->ether.dst_addr, ServerHwaddr, ETHADDRLEN );
	memcpy( &pkt->ether.src_addr, MyHwaddr, ETHADDRLEN );
	pkt->ether.type     = 0x0800;
	len += sizeof(struct etherhdr);

	return( eth_send( (Packet *)pkt, len ) );
}


static unsigned short ip_checksum( struct iphdr *buf )

{	unsigned long sum = 0, wlen = 5;

	__asm__ ("subqw #1,%2\n"
			 "1:\t"
			 "movel %1@+,%/d0\n\t"
			 "addxl %/d0,%0\n\t"
			 "dbra %2,1b\n\t"
			 "movel %0,%/d0\n\t"
			 "swap %/d0\n\t"
			 "addxw %/d0,%0\n\t"
			 "clrw %/d0\n\t"
			 "addxw %/d0,%0"
			 : "=d" (sum), "=a" (buf), "=d" (wlen)
			 : "0" (sum), "1" (buf), "2" (wlen)
			 : "d0");
	return( (~sum) & 0xffff );
}


static int udp_rcv( UDP *pkt, int *len, int fromport, int atport )

{	int				err;

  repeat:
	if ((err = eth_rcv( (Packet *)pkt, len )))
		return( err );
	
	/* Ethernet layer */
	if (pkt->ether.type == 0x0806) {
		/* ARP */
		ARP *pk = (ARP *)pkt;
		unsigned char *shw, *sip, *thw, *tip;

		if (pk->arp.hrd != 1 || pk->arp.pro != 0x0800 ||
			pk->arp.op != 1 || MyIPaddr == IP_Unknown_Addr)
			/* Wrong hardware type or protocol; or reply -> ignore */
			goto repeat;
		shw = pk->arp.addr;
		sip = shw + pk->arp.hln;
		thw = sip + pk->arp.pln;
		tip = thw + pk->arp.hln;
		
		if (memcmp( tip, &MyIPaddr, pk->arp.pln ) == 0) {
			memcpy( thw, shw, pk->arp.hln );
			memcpy( tip, sip, pk->arp.pln );
			memcpy( shw, &MyHwaddr, pk->arp.hln );
			memcpy( sip, &MyIPaddr, pk->arp.pln );
			pk->arp.op = 2;		/* make it REPLY */
			
			memcpy( &pk->ether.dst_addr, thw, ETHADDRLEN );
			memcpy( &pk->ether.src_addr, &MyHwaddr, ETHADDRLEN );
			eth_send( (Packet *)pk, *len );
		}
		goto repeat;
	}
	else if (pkt->ether.type != 0x0800) {
		Printf( "Unknown Ethernet packet type %04x received\n",
				pkt->ether.type );
		goto repeat;
	}

	/* IP layer */
	if (MyIPaddr != IP_Unknown_Addr && pkt->ip.dst_addr != MyIPaddr) {
		Printf( "Received packet for wrong IP address\n" );
		goto repeat;
	}
	if (ServerIPaddr != IP_Unknown_Addr &&
		ServerIPaddr != IP_Broadcast_Addr &&
		pkt->ip.src_addr != ServerIPaddr) {
#if 0
		/* This is no real error, be quiet about it */
		Printf( "Received packet from wrong server\n" );
#endif
		goto repeat;
	}
	/* If IP header is longer than 5 longs, delete the options */
	if (pkt->ip.ihl > 5) {
		char *udpstart = (char *)((long *)&pkt->ip + pkt->ip.ihl);
		memmove( &pkt->udp, udpstart, *len - (udpstart-(char *)pkt) );
	}
	
	/* UDP layer */
	if (fromport != 0 && pkt->udp.src_port != fromport) {
#if 0
		/* This is no real error, be quiet about it */
		Printf( "Received packet from wrong port %d\n", pkt->udp.src_port );
#endif
		goto repeat;
	}
	if (pkt->udp.dst_port != atport) {
		Printf( "Received packet at wrong port %d\n", pkt->udp.dst_port );
		goto repeat;
	}

	*len = pkt->udp.len - sizeof(struct udphdr);
	return( 0 );
}


/* --------------------------------------------------------------------- */
/*							   Address Printing							 */


static void print_ip( IPADDR addr )

{
	Printf( "%ld.%ld.%ld.%ld",
			(addr >> 24) & 0xff,
			(addr >> 16) & 0xff,
			(addr >>  8) & 0xff,
			addr & 0xff );
}


static void print_hw( HWADDR addr )

{
	Printf( "%02x:%02x:%02x:%02x:%02x:%02x",
			addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] );
}


/* --------------------------------------------------------------------- */
/*					 Ethernet Interface Abstraction Layer				 */


static int check_ethif( void )

{	int i;

	/* Check for configured interfaces */
	Ethif = NULL;
	for( i = 0; i < N_PossibleInterfaces; ++i ) {
		if (PossibleInterfaces[i]->probe() >= 0) {
			Ethif = PossibleInterfaces[i];
			break;
		}
	}
	if (!Ethif)
		return( -1 );

	if (Ethif->init() < 0) {
		Printf( "Ethernet interface initialization failed\n" );
		return( -1 );
	}
	Ethif->get_hwaddr( &MyHwaddr );
	return( 0 );
}

#if 0
static void dump_packet( UDP *pkt )

{	int		i, l;
	unsigned char *p;
	
	Printf( "Packet dump:\n" );

	Printf( "Ethernet header:\n" );
	Printf( "  dst addr: " ); print_hw( pkt->ether.dst_addr ); Printf( "\n" );
	Printf( "  src addr: " ); print_hw( pkt->ether.src_addr ); Printf( "\n" );
	Printf( "  type: %04x\n", pkt->ether.type );

	Printf( "IP header:\n" );
	Printf( "  version: %d\n", pkt->ip.version );
	Printf( "  hdr len: %d\n", pkt->ip.ihl );
	Printf( "  tos: %d\n", pkt->ip.tos );
	Printf( "  tot_len: %d\n", pkt->ip.tot_len );
	Printf( "  id: %d\n", pkt->ip.id );
	Printf( "  frag_off: %d\n", pkt->ip.frag_off );
	Printf( "  ttl: %d\n", pkt->ip.ttl );
	Printf( "  prot: %d\n", pkt->ip.protocol );
	Printf( "  src addr: " ); print_ip( pkt->ip.src_addr ); Printf( "\n" );
	Printf( "  dst addr: " ); print_ip( pkt->ip.dst_addr ); Printf( "\n" );

	Printf( "UDP header:\n" );
	Printf( "  src port: %d\n", pkt->udp.src_port );
	Printf( "  dst port: %d\n", pkt->udp.dst_port );
	Printf( "  len: %d\n", pkt->udp.len );

	Printf( "Data:" );
	l = pkt->udp.len - sizeof(pkt->udp);
	p = (unsigned char *)&pkt->udp + sizeof(pkt->udp);
	for( i = 0; i < l; ++i ) {
		if ((i % 32) == 0)
			Printf( "\n  %04x ", i );
		Printf( "%02x ", *p++ );
	}
	Printf( "\n" );
}
#endif

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
