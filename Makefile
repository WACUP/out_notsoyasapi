##
 # Makefile
 # Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 #
 # This file is part of out_yasapi.
 #
 # out_yasapi is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # out_yasapi is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with out_yasapi.  If not, see <http://www.gnu.org/licenses/>.
 ##
include config.mak
include $(SRCDIR)/out_yasapi/version.mak

.PHONY: all release release-bin release-src

UX2DOS=$(shell echo '$(1)' | sed -e 's,^/\(.\)/,\1:/,g')

###############################################################################
BUILDDIR=$(PWD)/build/out_yasapi
OUT_YASAPI=out_yasapi-$(OUT_YASAPI_VERSION)
#OUT_YASAPI_ALL+=build/$(OUT_YASAPI)/out_yasapi
OUT_YASAPI_ALL+=build/out_yasapi/out_yasapi.dll
OUT_YASAPI_ALL+=build/out_yasapi/out_yasapi-debug.dll
all: $(OUT_YASAPI_ALL)
all: $(BUILDDIR)/out_yasapi.nsi
OUT_YASAPI_INSTALL+=$(WINAMP)/Plugins/out_yasapi.dll

###############################################################################
OUT_YASAPI_OPTS+=--prefix=$(PREFIX)
OUT_YASAPI_OPTS+=--bindir=$(BINDIR)
OUT_YASAPI_OPTS+='--with-msvs=$(MSVS)'
OUT_YASAPI_OPTS+='--with-mssdk=$(MSSDK)'
OUT_YASAPI_OPTS+='--with-wasdk=$(WASDK)'
OUT_YASAPI_OPTS+='--with-winamp=$(WINAMP)'
OUT_YASAPI_OPTS+='CFLAGS=$(CFLAGS)'
OUT_YASAPI_OPTS+='CPPFLAGS=$(CPPFLAGS)'
$(OUT_YASAPI_ALL): $(INCLUDEDIR)/ya.h
$(OUT_YASAPI_ALL): build/out_yasapi/Makefile FORCE
	@echo 'MAKE	all'
	@cd $(<D) && make --no-print-directory all
#	@cd $(<D) && $(MAKE) all
build/out_yasapi/Makefile: $(SRCDIR)/out_yasapi/Makefile
build/out_yasapi/Makefile: $(SRCDIR)/out_yasapi/configure
	@echo CONFIGURE
	@mkdir -p $(@D)
	@cd $(@D) && $< $(OUT_YASAPI_OPTS)
	@touch $@

###############################################################################
LIBYA_OPTS+=--prefix=$(PREFIX)
LIBYA_OPTS+=--bindir=$(BINDIR)
LIBYA_OPTS+='--with-msvs=$(MSVS)'
LIBYA_OPTS+='--with-mssdk=$(MSSDK)'
LIBYA_OPTS+='--with-wasdk=$(WASDK)'
LIBYA_OPTS+='--with-winamp=$(WINAMP)'
LIBYA_OPTS+='CPPFLAGS=$(CPPFLAGS)'
LIBYA_OPTS+='CFLAGS=$(CFLAGS)'
$(INCLUDEDIR)/ya.h: build/libya/libya.lib FORCE
	@echo 'MAKE	install'
	@cd build/libya && $(MAKE) install
build/libya/libya.lib: build/libya/Makefile FORCE
	@echo 'MAKE	all'
	@cd $(<D) && $(MAKE) all
build/libya/Makefile: $(SRCDIR)/libya/Makefile
build/libya/Makefile: $(SRCDIR)/libya/configure
	@echo CONFIGURE
	@mkdir -p $(@D)
	@cd $(@D) && $< $(LIBYA_OPTS)
	@touch $@

###############################################################################
RELEASE=out_yasapi-$(OUT_YASAPI_VERSION)
RELEASE_SSE2=out_yasapi-sse2-$(OUT_YASAPI_VERSION)
RELEASE_DEBUG=out_yasapi-debug-$(OUT_YASAPI_VERSION)
RELEASE_DEBUG_SSE2=out_yasapi-debug-sse2-$(OUT_YASAPI_VERSION)

RELDIR=$(SRCDIR)/../release

RELPFX=$(RELDIR)/$(RELEASE)/$(RELEASE)
RELPFX_SSE2=$(RELDIR)/$(RELEASE)/$(RELEASE_SSE2)
RELPFX_DEBUG=$(RELDIR)/$(RELEASE)/$(RELEASE_DEBUG)
RELPFX_DEBUG_SSE2=$(RELDIR)/$(RELEASE)/$(RELEASE_DEBUG_SSE2)

RELEASE_BIN+=-C $(SRCDIR)/out_yasapi
RELEASE_BIN+=COPYING
RELEASE_BIN+=doc
RELEASE_BIN+=-C $(PWD)/build/out_yasapi
RELEASE_BIN+=out_yasapi.dll

RELEASE_DEBUG_BIN+=-C $(SRCDIR)/out_yasapi
RELEASE_DEBUG_BIN+=doc
RELEASE_DEBUG_BIN+=-C $(PWD)/build/out_yasapi
RELEASE_DEBUG_BIN+=out_yasapi-debug.dll

#RELEASE_SSE2_BIN+=-C $(SRCDIR)/out_yasapi
#RELEASE_SSE2_BIN+=COPYING
#RELEASE_SSE2_BIN+=doc
#RELEASE_SSE2_BIN+=-C $(PWD)/build/out_yasapi
#RELEASE_SSE2_BIN+=out_yasapi-sse2.dll

#RELEASE_DEBUG_SSE2_BIN+=-C $(SRCDIR)/out_yasapi
#RELEASE_DEBUG_SSE2_BIN+=COPYING
#RELEASE_DEBUG_SSE2_BIN+=doc
#RELEASE_DEBUG_SSE2_BIN+=-C $(PWD)/build/out_yasapi
#RELEASE_DEBUG_SSE2_BIN+=out_yasapi-debug-sse2.dll

release-bin: $(OUT_YASAPI_ALL)
release-bin: $(RELPFX).exe
release-bin: $(RELPFX).7z
#release-bin: $(RELPFX_SSE2).7z
release-bin: $(RELPFX_DEBUG).7z
#release-bin: $(RELPFX_DEBUG_SSE2).7z

RELEASE_SRC+=-C $(SRCDIR)
RELEASE_SRC+=COPYING
RELEASE_SRC+=bits
RELEASE_SRC+=configure
RELEASE_SRC+=Makefile
RELEASE_SRC+=out_yasapi
RELEASE_SRC+=libya
release-src: $(RELPFX)-src.tar.gz

%.7z: %.tar.bz2
	rm -rf $@ tmp
	mkdir -p tmp
	tar xfvj $< -C tmp
	cd tmp && 7za a $@ *
	rm -rf tmp $<
$(RELPFX).exe: FORCE
	@mkdir -p $(@D)
	cd $(BUILDDIR) && '$(NSIS)/makensis.exe' out_yasapi.nsi
$(RELPFX).tar.bz2: FORCE
	mkdir -p $(@D)
	tar cfvj $@ $(RELEASE_BIN) '--transform=s,^,$(RELEASE)/,g'
#$(RELPFX_SSE2).tar.bz2: FORCE
#	mkdir -p $(@D)
#	tar cfvj $@ $(RELEASE_SSE2_BIN) '--transform=s,^,$(RELEASE)/,g'
$(RELPFX_DEBUG).tar.bz2: FORCE
	mkdir -p $(@D)
	tar cfvj $@ $(RELEASE_DEBUG_BIN) '--transform=s,^,$(RELEASE)/,g'
#$(RELPFX_DEBUG_SSE2).tar.bz2: FORCE
#	mkdir -p $(@D)
#	tar cfvj $@ $(RELEASE_DEBUG_SSE2_BIN) '--transform=s,^,$(RELEASE)/,g'
$(RELPFX)-src.tar.gz: all FORCE
	mkdir -p $(@D)
	tar cfvz $@ $(RELEASE_SRC) '--transform=s,^,$(RELEASE)/,g'

###############################################################################
NSIOPTS+=-e 's,@@VERSION@@,$(OUT_YASAPI_VERSION),g'
NSIOPTS+=-e 's,@@RELPFX@@,$(call UX2DOS,$(RELPFX)),g'

#build/$(OUT_YASAPI)/out_yasapi.nsi: $(SRCDIR)/out_yasapi/out_yasapi.nsi FORCE
$(BUILDDIR)/out_yasapi.nsi: $(SRCDIR)/out_yasapi/out_yasapi.nsi FORCE
	@echo 'SED	$(@F)'
	@mkdir -p $(@D)
	@sed $(NSIOPTS) $< > $@

###############################################################################
$(WINAMP)/Plugins/%.dll: build/out_yasapi/out_yasapi-debug.dll
	@mkdir -p $(@D)
	cp $< $(@D)

###############################################################################
#release: $(RELPFX)-src.tar.gz
#release: $(RELPFX).exe
#release: $(RELPFX).7z
#release: $(RELPFX_DEBUG).7z
release: release-src
release: release-bin
$(RELPFX).exe $(RELPFX).7z: build/out_yasapi/out_yasapi.dll
$(RELPFX).exe: $(BUILDDIR)/out_yasapi.nsi
$(RELPFX_DEBUG).7z: build/out_yasapi/out_yasapi-debug.dll

###############################################################################
.PHONY: FORCE
FORCE:
