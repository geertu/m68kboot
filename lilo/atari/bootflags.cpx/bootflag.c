#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <aes.h>
#include <cpxdata.h>

#include "bootflag.h"
#include "bootflag.rsh"

#define main_tree (rs_trindex[MAIN])
#define help_tree (rs_trindex[HELPT])

#define	NUM_FLAGS 6
int pbuttons[NUM_FLAGS] = {
  MP_80, MP_40, MP_20, MP_10, MP_08, MP_NONE
};
int tbuttons[NUM_FLAGS] = {
  MT_80, MT_40, MT_20, MT_10, MT_08, MT_NONE
};
int flagval[NUM_FLAGS] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0
};

int pflag, tflag;
int gpip_present;
int errno;

#define	SCU_GPIP1	  (*(unsigned char*)0xffff8e09L)

#define obstate(t,o)  (t[o].ob_state)

CPXINFO * cdecl cpx_init( XCPB *Xcpb );
int       cdecl cpx_call( GRECT *work );
void do_maindial( void );
int  show_help( void );
void extract_data( void );
void get_data( void );
void set_data( void );
void test_gpip( void );

/* defined in testgpip.s */
extern test_register(  char *addr );

XCPB    *xcpb;          /* XControl Parameter Block   */

CPXINFO cpxinfo = {     /* CPX Information Structure  */
  cpx_call, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


CPXINFO * cdecl cpx_init( XCPB *Xcpb )

{ int i;

  xcpb = Xcpb;
  appl_init();
        
  if (xcpb->booting) {
    return( (CPXINFO *)1 );  
  }
  else {
    if( !xcpb->SkipRshFix ) {
      for( i = 0; i < sizeof(rs_object)/sizeof(*rs_object); ++i )
        (*Xcpb->rsh_obfix)( rs_object, i );
    } 
    return( &cpxinfo );
  }
}


int cdecl cpx_call( GRECT *work )

{
  int i;
  GRECT world;

  main_tree[0].ob_x = help_tree[0].ob_x = work->g_x;
  main_tree[0].ob_y = help_tree[0].ob_y = work->g_y;
  main_tree[0].ob_width = help_tree[0].ob_width = work->g_w;
  main_tree[0].ob_height = help_tree[0].ob_height = work->g_h;

  test_gpip();
  get_data();
  for( i = 0; i < NUM_FLAGS; ++i ) {
    obstate( main_tree, pbuttons[i] ) &= ~SELECTED;
    obstate( main_tree, tbuttons[i] ) &= ~SELECTED;
	if (pflag == flagval[i])
      obstate( main_tree, pbuttons[i] ) |= SELECTED;
	if (!gpip_present)
	  obstate( main_tree, tbuttons[i] ) |= DISABLED;
	else if (tflag == flagval[i])
      obstate( main_tree, tbuttons[i] ) |= SELECTED;
  }

  objc_offset( main_tree, 0, &(world.g_x), &(world.g_y) );
  world.g_w = main_tree[0].ob_width;
  world.g_h = main_tree[0].ob_height;
  objc_draw( main_tree, 0, MAX_DEPTH, ELTS(world));

  do_maindial();
  return( 0 );
}


void do_maindial( void )

{ int msgbuf[8], ret;

  for(;;) {
  
    ret = (*xcpb->Xform_do)( main_tree, 0, msgbuf );
    if (ret >= 0) {
      ret &= ~0x8000;
      obstate( main_tree, ret ) &= ~SELECTED;
      if (obstate( main_tree, ret ) & DISABLED) continue;
    }

    switch( ret ) {
      
      case M_HELP:
        ret = show_help();
        main_tree[0].ob_x = help_tree[0].ob_x;
        main_tree[0].ob_y = help_tree[0].ob_y;
        break;
      
      case M_OK:
        extract_data();
        set_data();
        return;
      
      case M_CANCEL:
        return;
      
      case -1:
        if (msgbuf[0] == AC_CLOSE) 
          return;
        else if (msgbuf[0] == WM_CLOSED) {
          set_data();
          return;
        }
        break;
      
    }
  }
}
 
        
int show_help( void )

{
  int ret, msgbuf[8];
  GRECT world;

  help_tree[0].ob_x = main_tree[0].ob_x;
  help_tree[0].ob_y = main_tree[0].ob_y;

  objc_offset( help_tree, 0, &(world.g_x), &(world.g_y) );
  world.g_w = help_tree[0].ob_width;
  world.g_h = help_tree[0].ob_height;
  objc_draw( help_tree, 0, MAX_DEPTH, ELTS(world));
    
  for(;;) {
  
    ret = (*xcpb->Xform_do)( help_tree, 0, msgbuf );
    if (ret >= 0) {
      ret &= ~0x8000;
      obstate( help_tree, ret ) &= ~SELECTED;
      if (obstate( help_tree, ret ) & DISABLED) continue;
    }
    
    switch( ret ) {
    
      case I_EXIT:
        obstate( help_tree, I_EXIT ) &= ~SELECTED;
        objc_draw( main_tree, 0, MAX_DEPTH, ELTS(world) );
        return( 0 );
      
      case -1:
        if (msgbuf[0] == AC_CLOSE)
          return( 1 );
        else if (msgbuf[0] == WM_CLOSED)
          return( 2 );
        break;
    }
  }
}


void extract_data( void )

{
	int i;

	for( i = 0; i < NUM_FLAGS; ++i ) {
		if (obstate( main_tree, pbuttons[i] ) & SELECTED)
			pflag = flagval[i];
		if (obstate( main_tree, tbuttons[i] ) & SELECTED)
			tflag = flagval[i];
	}
}
 

void get_data( void )

{	char c;
	long err, savessp;

	err = NVMaccess( 0, 1, 1, &c );
	if (err == -12) {
		form_alert( 1, "[3][ Bad NVRAM checksum! ][ OK ]" );
		c = 0;
	}
	else if (err) {
		char buf[128];
		sprintf( buf, "[3][ TOS error %ld on NVMaccess ][ OK ]", err );
		form_alert( 1, buf );
		c = 0;
	}
	pflag = (unsigned char)c & 0xf8;

	if (gpip_present) {
		savessp = Super(0L);
		tflag = SCU_GPIP1 & 0xf8;
		Super( savessp );
	}
	else {
		tflag = 0;
	}
}


void set_data( void )

{	char c;
	long err, savessp;

	err = NVMaccess( 0, 1, 1, &c );
	if (err == -12) {
		form_alert( 1, "[3][ Bad NVRAM checksum! ][ OK ]" );
		c = 0;
	}
	else if (err) {
		char buf[128];
		sprintf( buf, "[3][ TOS error %ld on NVMaccess ][ OK ]", err );
		form_alert( 1, buf );
		c = 0;
	}

	c = (c & 0x07) | (pflag & 0xf8);

	err = NVMaccess( 1, 1, 1, &c );
	if (err == -12) {
		form_alert( 1, "[3][ Bad NVRAM checksum! ][ OK ]" );
	}
	else if (err) {
		char buf[128];
		sprintf( buf, "[3][ TOS error %ld on NVMaccess ][ OK ]", err );
		form_alert( 1, buf );
	}

	if (gpip_present) {
		savessp = Super(0L);
		SCU_GPIP1 = (SCU_GPIP1 & 0x07) | (tflag & 0xf8);
		Super( savessp );
	}
}


void test_gpip( void )

{	long savessp;

	savessp = Super(0L);
	gpip_present = test_register( &SCU_GPIP1 );
	Super( savessp );
}
