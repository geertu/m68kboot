#ifndef _bootp_h
#define _bootp_h

/*
 * $Id: bootp.h,v 1.1 1997-07-15 09:45:37 rnhodek Exp $
 * 
 * $Log: bootp.h,v $
 * Revision 1.1  1997-07-15 09:45:37  rnhodek
 * Initial revision
 *
 *
 */

/* --------------------------------------------------------------------- */
/*							 Ethernet Definitions						 */

#define	PKTLEN			1544
typedef unsigned char	Packet[PKTLEN];

#define	ETHADDRLEN		6
typedef unsigned char	HWADDR[ETHADDRLEN];

typedef struct {
	int		(*probe)( void );
	int		(*init)( void );
	void	(*get_hwaddr)( HWADDR *addr );
	int		(*snd)( Packet *pkt, int len );
	int		(*rcv)( Packet *pkt, int *len );
} ETHIF_SWITCH;


/* error codes */
#define	ETIMEO	-1		/* Timeout */
#define	ESEND	-2		/* General send error (carrier, abort, ...) */
#define	ERCV	-3		/* General receive error */
#define	EFRAM	-4		/* Framing error */
#define	EOVERFL	-5		/* Overflow (too long packet) */
#define	ECRC	-6		/* CRC error */

typedef unsigned long IPADDR;

#endif  /* _bootp_h */

/* Local Variables: */
/* tab-width: 4     */
/* End:             */
