/* 
 * $Id: do_test.c,v 1.2 1997-09-19 09:06:53 geert Exp $
 * 
 * $Log: do_test.c,v $
 * Revision 1.2  1997-09-19 09:06:53  geert
 * Big bunch of changes by Geert: make things work on Amiga; cosmetic things
 *
 * Revision 1.1  1997/08/12 15:27:03  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 */ 


#include <stdlib.h>
#include <sys/types.h>
#include <asm/amigahw.h>

#include "amigaos.h"
#include "loader.h"



#define IS_AGA			1
#define VGA70			1



    /*
     * Custom Chipset Definitions
     */

#define CUSTOM_OFS(fld) ((long)&((struct CUSTOM*)0)->fld)

    /*
     *  BPLCON0 -- Bitplane Control Register 0
     */

#define BPC0_HIRES	(0x8000)
#define BPC0_BPU2	(0x4000) /* Bit plane used count */
#define BPC0_BPU1	(0x2000)
#define BPC0_BPU0	(0x1000)
#define BPC0_HAM	(0x0800) /* HAM mode */
#define BPC0_DPF	(0x0400) /* Double playfield */
#define BPC0_COLOR	(0x0200) /* Enable colorburst */
#define BPC0_GAUD	(0x0100) /* Genlock audio enable */
#define BPC0_UHRES	(0x0080) /* Ultrahi res enable */
#define BPC0_SHRES	(0x0040) /* Super hi res mode */
#define BPC0_BYPASS	(0x0020) /* Bypass LUT - AGA */
#define BPC0_BPU3	(0x0010) /* AGA */
#define BPC0_LPEN	(0x0008) /* Light pen enable */
#define BPC0_LACE	(0x0004) /* Interlace */
#define BPC0_ERSY	(0x0002) /* External resync */
#define BPC0_ECSENA	(0x0001) /* ECS enable */

    /*
     *  BPLCON2 -- Bitplane Control Register 2
     */

#define BPC2_ZDBPSEL2	(0x4000) /* Bitplane to be used for ZD - AGA */
#define BPC2_ZDBPSEL1	(0x2000)
#define BPC2_ZDBPSEL0	(0x1000)
#define BPC2_ZDBPEN	(0x0800) /* Enable ZD with ZDBPSELx - AGA */
#define BPC2_ZDCTEN	(0x0400) /* Enable ZD with palette bit #31 - AGA */
#define BPC2_KILLEHB	(0x0200) /* Kill EHB mode - AGA */
#define BPC2_RDRAM	(0x0100) /* Color table accesses read, not write - AGA */
#define BPC2_SOGEN	(0x0080) /* SOG output pin high - AGA */
#define BPC2_PF2PRI	(0x0040) /* PF2 priority over PF1 */
#define BPC2_PF2P2	(0x0020) /* PF2 priority wrt sprites */
#define BPC2_PF2P1	(0x0010)
#define BPC2_PF2P0	(0x0008)
#define BPC2_PF1P2	(0x0004) /* ditto PF1 */
#define BPC2_PF1P1	(0x0002)
#define BPC2_PF1P0	(0x0001)

    /*
     *  BPLCON3 -- Bitplane Control Register 3 (AGA)
     */

#define BPC3_BANK2	(0x8000) /* Bits to select color register bank */
#define BPC3_BANK1	(0x4000)
#define BPC3_BANK0	(0x2000)
#define BPC3_PF2OF2	(0x1000) /* Bits for color table offset when PF2 */
#define BPC3_PF2OF1	(0x0800)
#define BPC3_PF2OF0	(0x0400)
#define BPC3_LOCT	(0x0200) /* Color register writes go to low bits */
#define BPC3_SPRES1	(0x0080) /* Sprite resolution bits */
#define BPC3_SPRES0	(0x0040)
#define BPC3_BRDRBLNK	(0x0020) /* Border blanked? */
#define BPC3_BRDRTRAN	(0x0010) /* Border transparent? */
#define BPC3_ZDCLKEN	(0x0004) /* ZD pin is 14 MHz (HIRES) clock output */
#define BPC3_BRDRSPRT	(0x0002) /* Sprites in border? */
#define BPC3_EXTBLKEN	(0x0001) /* BLANK programmable */

    /*
     *  BPLCON4 -- Bitplane Control Register 4 (AGA)
     */

#define BPC4_BPLAM7	(0x8000) /* bitplane color XOR field */
#define BPC4_BPLAM6	(0x4000)
#define BPC4_BPLAM5	(0x2000)
#define BPC4_BPLAM4	(0x1000)
#define BPC4_BPLAM3	(0x0800)
#define BPC4_BPLAM2	(0x0400)
#define BPC4_BPLAM1	(0x0200)
#define BPC4_BPLAM0	(0x0100)
#define BPC4_ESPRM7	(0x0080) /* 4 high bits for even sprite colors */
#define BPC4_ESPRM6	(0x0040)
#define BPC4_ESPRM5	(0x0020)
#define BPC4_ESPRM4	(0x0010)
#define BPC4_OSPRM7	(0x0008) /* 4 high bits for odd sprite colors */
#define BPC4_OSPRM6	(0x0004)
#define BPC4_OSPRM5	(0x0002)
#define BPC4_OSPRM4	(0x0001)

    /*
     *  BEAMCON0 -- Beam Control Register
     */

#define BMC0_HARDDIS	(0x4000) /* Disable hardware limits */
#define BMC0_LPENDIS	(0x2000) /* Disable light pen latch */
#define BMC0_VARVBEN	(0x1000) /* Enable variable vertical blank */
#define BMC0_LOLDIS	(0x0800) /* Disable long/short line toggle */
#define BMC0_CSCBEN	(0x0400) /* Composite sync/blank */
#define BMC0_VARVSYEN	(0x0200) /* Enable variable vertical sync */
#define BMC0_VARHSYEN	(0x0100) /* Enable variable horizontal sync */
#define BMC0_VARBEAMEN	(0x0080) /* Enable variable beam counters */
#define BMC0_DUAL	(0x0040) /* Enable alternate horizontal beam counter */
#define BMC0_PAL	(0x0020) /* Set decodes for PAL */
#define BMC0_VARCSYEN	(0x0010) /* Enable variable composite sync */
#define BMC0_BLANKEN	(0x0008) /* Blank enable (no longer used on AGA) */
#define BMC0_CSYTRUE	(0x0004) /* CSY polarity */
#define BMC0_VSYTRUE	(0x0002) /* VSY polarity */
#define BMC0_HSYTRUE	(0x0001) /* HSY polarity */

    /*
     *  FMODE -- Fetch Mode Control Register (AGA)
     */

#define FMODE_SSCAN2	(0x8000) /* Sprite scan-doubling */
#define FMODE_BSCAN2	(0x4000) /* Use PF2 modulus every other line */
#define FMODE_SPAGEM	(0x0008) /* Sprite page mode */
#define FMODE_SPR32	(0x0004) /* Sprite 32 bit fetch */
#define FMODE_BPAGEM	(0x0002) /* Bitplane page mode */
#define FMODE_BPL32	(0x0001) /* Bitplane 32 bit fetch */


    /*
     *  Video mode definitions
     */

#define XRES		640
#define YRES		(VGA70 ? 400 : 480)
#define DIWSTRT_H	0x0117
#define DIWSTOP_H	0x0397
#define DIWSTRT_V	(VGA70 ? 0x0062 : 0x0052)
#define DIWSTOP_V	(VGA70 ? 0x0382 : 0x0412)
#define DDFSTRT		0x00c0
#define DDFSTOP		0x0300
#define BPLCON0		(BPC0_BPU0 | BPC0_COLOR | BPC0_SHRES | BPC0_ECSENA)
#define BPLCON1		0x3344
#define HTOTAL		0x0390
#define VTOTAL		(VGA70 ? 0x0382 : 0x0412)
#define BPLCON3		(BPC3_SPRES1 | BPC3_SPRES0 | BPC3_EXTBLKEN)
#define BEAMCON0	(BMC0_HARDDIS | BMC0_VARVBEN | BMC0_LOLDIS | \
			 BMC0_VARVSYEN | BMC0_VARHSYEN | BMC0_VARBEAMEN | \
			 BMC0_PAL | BMC0_VARCSYEN | \
			 (VGA70 ? (BMC0_CSYTRUE | BMC0_VSYTRUE) : 0))
#define HSSTRT		0x0060
#define HSSTOP		0x00d0
#define HBSTRT		0x000b
#define HBSTOP		0x011b
#define VSSTRT		(VGA70 ? 0x0018 : 0x0012)
#define VSSTOP		(VGA70 ? 0x001c : 0x0016)
#define VBSTRT		(VGA70 ? 0x0380 : 0x0410)
#define VBSTOP		(VGA70 ? 0x0060 : 0x0050)
#define HCENTER		0x0228
#define FMODE		(FMODE_BPAGEM | FMODE_BPL32)
#define RED0		0xaaaa
#define GREEN0		0xaaaa
#define BLUE0		0xaaaa
#define RED1		0x0000
#define GREEN1		0x0000
#define BLUE1		0x0000


    /*
     *  Various macros
     */

#define div2(v)		((v)>>1)
#define div8(v)		((v)>>3)
#define highw(x)	((u_long)(x)>>16 & 0xffff)
#define loww(x)		((u_long)(x) & 0xffff)


/* diwstrt/diwstop/diwhigh (visible display window) */

#define diwstrt2hw(diwstrt_h, diwstrt_v) \
	(((diwstrt_v)<<7 & 0xff00) | ((diwstrt_h)>>2 & 0x00ff))
#define diwstop2hw(diwstop_h, diwstop_v) \
	(((diwstop_v)<<7 & 0xff00) | ((diwstop_h)>>2 & 0x00ff))
#define diwhigh2hw(diwstrt_h, diwstrt_v, diwstop_h, diwstop_v) \
	(((diwstop_h)<<3 & 0x2000) | ((diwstop_h)<<11 & 0x1800) | \
	 ((diwstop_v)>>1 & 0x0700) | ((diwstrt_h)>>5 & 0x0020) | \
	 ((diwstrt_h)<<3 & 0x0018) | ((diwstrt_v)>>9 & 0x0007))

/* ddfstrt/ddfstop (display DMA) */

#define ddfstrt2hw(ddfstrt)	div8(ddfstrt)
#define ddfstop2hw(ddfstop)	div8(ddfstop)

/* hsstrt/hsstop/htotal/vsstrt/vsstop/vtotal/hcenter (sync timings) */

#define hsstrt2hw(hsstrt)	(div8(hsstrt))
#define hsstop2hw(hsstop)	(div8(hsstop))
#define htotal2hw(htotal)	(div8(htotal)-1)
#define vsstrt2hw(vsstrt)	(div2(vsstrt))
#define vsstop2hw(vsstop)	(div2(vsstop))
#define vtotal2hw(vtotal)	(div2(vtotal)-1)
#define hcenter2hw(hcenter)	(div8(hcenter))

/* hbstrt/hbstop/vbstrt/vbstop (blanking timings) */

#define hbstrt2hw(hbstrt)	(((hbstrt)<<8 & 0x0700) | ((hbstrt)>>3 & 0x00ff))
#define hbstop2hw(hbstop)	(((hbstop)<<8 & 0x0700) | ((hbstop)>>3 & 0x00ff))
#define vbstrt2hw(vbstrt)	(div2(vbstrt))
#define vbstop2hw(vbstop)	(div2(vbstop))

/* color */

#define rgb2hw8_high(red, green, blue) \
    (((red)<<4 & 0xf00) | ((green) & 0x0f0) | ((blue)>>4 & 0x00f))
#define rgb2hw8_low(red, green, blue) \
    (((red)<<8 & 0xf00) | ((green)<<4 & 0x0f0) | ((blue) & 0x00f))
#define rgb2hw2(red, green, blue) \
    (((red)<<10 & 0xc00) | ((green)<<6 & 0x0c0) | ((blue)<<2 & 0x00c))


    /*
     *  Copper Instructions
     */

#define CMOVE(val, reg)		(CUSTOM_OFS(reg)<<16 | (val))
#define CMOVE2(val, reg)	((CUSTOM_OFS(reg)+2)<<16 | (val))
#define CWAIT(x, y)		(((y) & 0x1fe)<<23 | ((x) & 0x7f0)<<13 | \
				 0x0001fffe)
#define CEND			(0xfffffffe)

typedef union {
    u_long l;
    u_short w[2];
} copins;

#define SCREENSIZE		(XRES*YRES/8)
#define DUMMYSPRITEMEMSIZE	8
#define COPMEMSIZE		(39*sizeof(copins))

static void *screen = NULL;
static u_short *dummysprite = NULL;
static copins *copinit = NULL;
static copins *copwait;
static copins *coplist;


static u_short ecs_palette[32];


static void init_copper(void)
{
    copins *cop = copinit;
    int i;

    (cop++)->l = CMOVE(BPC0_COLOR | BPC0_SHRES | BPC0_ECSENA, bplcon0);
    (cop++)->l = CMOVE(0x0181, diwstrt);
    (cop++)->l = CMOVE(0x0281, diwstop);
    (cop++)->l = CMOVE(0x0000, diwhigh);
    for (i = 0; i < 8; i++) {
	(cop++)->l = CMOVE(0, spr[i].pos);
	(cop++)->l = CMOVE(highw(dummysprite), sprpt[i]);
	(cop++)->l = CMOVE2(loww(dummysprite), sprpt[i]);
    }
    copwait = cop;
    (cop++)->l = CEND;
    (cop++)->l = CMOVE(0, copjmp2);
    (cop++)->l = CEND;
    coplist = cop;
    custom.cop1lc = (u_short *)copinit;
    custom.copjmp1 = 0;
}


static void build_copper(void)
{
    copins *cop = coplist;

    (cop++)->l = CWAIT(0, 10);
    (cop++)->l = CMOVE(BPLCON0, bplcon0);
    (cop++)->l = CMOVE(diwstrt2hw(DIWSTRT_H, DIWSTRT_V), diwstrt);
    (cop++)->l = CMOVE(diwstop2hw(DIWSTOP_H, DIWSTOP_V), diwstop);
    (cop++)->l = CMOVE(diwhigh2hw(DIWSTRT_H, DIWSTRT_V,
				   DIWSTOP_H, DIWSTOP_V), diwhigh);
    (cop++)->l = CMOVE(highw(screen), bplpt[0]);
    (cop++)->l = CMOVE2(loww(screen), bplpt[0]);
    cop->l = CEND;
}


static void init_display(void)
{
    custom.bplcon1 = BPLCON1;
    custom.bpl1mod = 0;
    custom.bpl2mod = 0;
    custom.ddfstrt = ddfstrt2hw(DDFSTRT);
    custom.ddfstop = ddfstop2hw(DDFSTOP);
    custom.bplcon0 = BPLCON0;
    custom.bplcon2 = BPC2_KILLEHB | BPC2_PF2P2 | BPC2_PF1P2;
    custom.bplcon3 = BPLCON3;
    if (IS_AGA)
	custom.bplcon4 = BPC4_ESPRM4 | BPC4_OSPRM4;
    custom.htotal = htotal2hw(HTOTAL);
    custom.hbstrt = hbstrt2hw(HBSTRT);
    custom.hbstop = hbstop2hw(HBSTOP);
    custom.hsstrt = hsstrt2hw(HSSTRT);
    custom.hsstop = hsstop2hw(HSSTOP);
    custom.hcenter = hcenter2hw(HCENTER);
    custom.vtotal = vtotal2hw(VTOTAL);
    custom.vbstrt = vbstrt2hw(VBSTRT);
    custom.vbstop = vbstop2hw(VBSTOP);
    custom.vsstrt = vsstrt2hw(VSSTRT);
    custom.vsstop = vsstop2hw(VSSTOP);
    custom.beamcon0 = BEAMCON0;
    if (IS_AGA)
	custom.fmode = FMODE;
    custom.vposw = custom.vposr | 0x8000;
    custom.cop2lc = (u_short *)coplist;
    if (IS_AGA) {
	custom.bplcon3 = BPLCON3;
	custom.color[0] = rgb2hw8_high(RED0, GREEN0, BLUE0);
	custom.color[1] = rgb2hw8_high(RED1, GREEN1, BLUE1);
	custom.bplcon3 = BPLCON3 | BPC3_LOCT;
	custom.color[0] = rgb2hw8_low(RED0, GREEN0, BLUE0);
	custom.color[1] = rgb2hw8_low(RED1, GREEN1, BLUE1);
	custom.bplcon3 = BPLCON3;
    } else {
        u_short color, mask;
        int i;

        mask = 0x3333;
        color = rgb2hw2(RED0, GREEN0, BLUE0);
        for (i = 12; i >= 0; i -= 4)
          custom.color[i] = ecs_palette[i] = (ecs_palette[i] & mask) | color;
        mask <<= 2; color >>= 2;
        for (i = 3; i >= 0; i--)
          custom.color[i] = ecs_palette[i] = (ecs_palette[i] & mask) | color;
        mask = 0x3333;
        color = rgb2hw2(RED1, GREEN1, BLUE1);
        for (i = 13; i >= 1; i -= 4)
          custom.color[i] = ecs_palette[i] = (ecs_palette[i] & mask) | color;
        mask <<= 2; color >>= 2;
        for (i = 7; i >= 4; i--)
          custom.color[i] = ecs_palette[i] = (ecs_palette[i] & mask) | color;
    }
    copwait->l = CWAIT(32, DIWSTRT_V-4);
}


static void draw_pattern(void)
{
    char *p;
    u_long nextline = XRES/8;
    int i, j;

    p = (char *)screen;
    for (i = 0; i < nextline; i++)
	p[i] = 0xff;
    p = (char *)(screen+nextline-1);
    for (i = 1; i < YRES-1; i++)
	p[i*nextline] = 0x01;
    p = (char *)(screen+(YRES-1)*nextline);
    for (i = nextline-1; i >= 0; i++)
	p[i] = 0xff;
    p = (char *)screen;
    for (i = YRES-2; i >= 1; i++)
	p[i*nextline] = 0x80;
    p = (char *)screen;
    for (i = 8; i < YRES-8; i++)
	for (j = 1; j < nextline-1; j++)
	    p[i*nextline+j] = i & 1 ? 0x55 : 0xaa;
}

static void init(void)
{
    LoadView(NULL);
    OwnBlitter();
    screen = AllocMem(SCREENSIZE, MEMF_CHIP | MEMF_CLEAR);
    dummysprite = AllocMem(DUMMYSPRITEMEMSIZE, MEMF_CHIP | MEMF_CLEAR);
    copinit = AllocMem(COPMEMSIZE, MEMF_CHIP | MEMF_CLEAR);

    /*
     *  Enable display DMA
     */
    custom.dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER |
    		    DMAF_SPRITE;

    /*
     *  Make sure the Copper has something to do
     */

    init_copper();

    /*
     *  Create a copper list
     */

    build_copper();

    /*
     *  Set the video mode
     */

    init_display();

    draw_pattern();
}

static void cleanup(void)
{
    FreeMem(copinit, COPMEMSIZE);
    FreeMem(dummysprite, DUMMYSPRITEMEMSIZE);
    FreeMem(screen, SCREENSIZE);
    DisOwnBlitter();
    LoadView(NULL);
}


void do_test(void)
{
    init();
    Sleep(5000000);
    cleanup();
}
