/* 
 * $Id: do_test.c,v 1.1 1997-08-12 15:27:03 rnhodek Exp $
 * 
 * $Log: do_test.c,v $
 * Revision 1.1  1997-08-12 15:27:03  rnhodek
 * Import of Amiga and newly written Atari lilo sources, with many mods
 * to separate out common parts.
 *
 */ 


static struct copdisplay {
    copins *init;
    copins *wait;
    copins *list[2][2];
    copins *rebuild[2];
} copdisplay;


static void init_copper(void)
{
    copins *cop = copdisplay.init;

    (cop++)->l = CMOVE(BPC0_COLOR | BPC0_SHRES | BPC0_ECSENA, bplcon0);
    (cop++)->l = CMOVE(0x0181, diwstrt);
    (cop++)->l = CMOVE(0x0281, diwstop);
    (cop++)->l = CMOVE(0x0000, diwhigh);
    p = ZTWO_PADDR(dummysprite);
    for (i = 0; i < 8; i++) {
	(cop++)->l = CMOVE(0, spr[i].pos);
	(cop++)->l = CMOVE(highw(p), sprpt[i]);
	(cop++)->l = CMOVE2(loww(p), sprpt[i]);
    }
    (cop++)->l = CMOVE(IF_SETCLR | IF_COPER, intreq);
    copdisplay.wait = cop;
    (cop++)->l = CEND;
    (cop++)->l = CMOVE(0, copjmp2);
    cop->l = CEND;

    custom.cop1lc = (u_short *)ZTWO_PADDR(copdisplay.init);
    custom.copjmp1 = 0;
}


static void reinit_copper(void)
{
    copdisplay.init[cip_bplcon0].w[1] =
    	~(BPC0_BPU3 | BPC0_BPU2 | BPC0_BPU1 | BPC0_BPU0) & par->bplcon0;
    copdisplay.wait->l = CWAIT(32, par->diwstrt_v-4);
}


static void build_copper(void)
{
    copins *copl, *cops;

    copl = copdisplay.list[currentcop][1];
    (copl++)->l = CWAIT(0, 10);
    (copl++)->l = CMOVE(par->bplcon0, bplcon0);
    (copl++)->l = CMOVE(0, sprpt[0]);
    (copl++)->l = CMOVE2(0, sprpt[0]);

    (copl++)->l = CMOVE(diwstrt2hw(par->diwstrt_h, par->diwstrt_v), diwstrt);
    (copl++)->l = CMOVE(diwstop2hw(par->diwstop_h, par->diwstop_v), diwstop);
    (copl++)->l = CMOVE(diwhigh2hw(par->diwstrt_h, par->diwstrt_v,
				   par->diwstop_h, par->diwstop_v), diwhigh);
    copdisplay.rebuild[1] = copl;
    rebuild_copper();
}


static void rebuild_copper(void)
{
    copins *copl, *cops;

    u_short line, h_end1, h_end2;
    h_end1 = par->htotal-32;
    h_end2 = par->ddfstop+64;
    
    copl = copdisplay.rebuild[1];
    p = par->bplpt0;

    for (i = 0; i < (short)par->bpp; i++, p += par->next_plane) {
	(copl++)->l = CMOVE(highw(p), bplpt[i]);
	(copl++)->l = CMOVE2(loww(p), bplpt[i]);
    }
    copl->l = CEND;
}


static void irq_init_display(void)
{
    custom.vposw = custom.vposr | 0x8000;
    custom.cop2lc = (u_short *)ZTWO_PADDR(copdisplay.list[currentcop][1]);
}

void do_test(void)
{








}
