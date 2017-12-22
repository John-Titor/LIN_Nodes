#
# Top-level makefile
#

TOPDIR		:= $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILDDIR	:= $(TOPDIR)/build

.PHONY: build
build:
	make -C common
	make -C nodes
	make -C lintool

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)
	rm -f common/lin_defs.cpp common/lin_defs.h

.PHONY: release
release: clean
	rm -rf $(BUILDDIR)
	make -C common
	make -C nodes
	make -C lintool	

.PHONY: program
program: release
	build/lintool/lintool update build/*/*.fw
