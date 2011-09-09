## Process this file with automake to create Makefile.in
#
# Copyright (c) 2011 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
script_DATA = less-global global-client README

scriptdir = ${datadir}/gtags/script

EXTRA_DIST = ${script_DATA}

install-data-hook:
	if cd $(DESTDIR)$(datadir)/gtags/script; then \
		chmod 755 less-global global-client; \
		ln -f global-client gtags-client; \
		ln -f global-client htags-client; \
	fi
