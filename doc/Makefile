#
# Makefile for doc dir of m68kboot
#
# Copyright (c) 1997-98 by Geert Uytterhoeven and Roman Hodek
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# $Id: Makefile,v 1.1 1998-02-26 11:39:57 rnhodek Exp $
#
# $Log: Makefile,v $
# Revision 1.1  1998-02-26 11:39:57  rnhodek
# Written Makefile for doc/; partially copied from Geert's doc-exp/Makefile
# from amiboot-3.0.
#
#

all: lilo.html lilo.dvi lilo.ps lilo.info lilo.txt

install: lilo.8 lilo.info
	install -m 644 lilo.8 $(PREFIX)/usr/man/man8
	install -m 644 lilo.info $(PREFIX)/usr/info

clean:
		rm -f *.html *.dvi *.log *.aux *.toc *.ps *.info lilo.txt

distclean: clean

# General rules

%.html:		%.sgml
		sgml2html $<

%.dvi:		%.sgml
		sgml2latex --output=dvi $<

%.ps:		%.sgml
		sgml2latex --output=ps $<

%.info:		%.sgml
		sgml2info $<

%.txt:		%.sgml
		sgml2txt $<

