################################################################################
################### Makefile is preferred by any other make ####################
################################################################################
#
# Copyright © 2015-2025 Boian Bonev (bbonev@ipacct.com) {{{
#
# SPDX-License-Identifer: LGPL-3.0-or-later
#
# This file is part of yascreen - yet another screen library.
#
# yascreen is free software, released under the terms of GNU Lesser General Public License v3.0 or later
#
# }}}

all .DEFAULT:
	@if [ ! -x "$$(command -v gmake 2>/dev/null)" ]; then echo gmake is required; false; fi
	@gmake --no-print-directory -f GNUmakefile $@

.PHONY: all
