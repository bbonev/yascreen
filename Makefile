################################################################################
################### Makefile is preferred by any other make ####################
################################################################################

$(.TARGETS):
	@if [ ! -x "`which gmake 2>/dev/null`" ]; then echo gmake is required; false; fi
	@gmake -f GNUmakefile $(.TARGETS)

