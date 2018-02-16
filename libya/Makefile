##
 # Makefile
 # Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 #
 # This file is part of libya.
 #
 # libya is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # libya is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with libya.  If not, see <http://www.gnu.org/licenses/>.
 ##
include config.mak
.PHONY: all install clean

###############################################################################
DOS2UX=$(shell echo '$(1)'|sed -e 's,^\(.\):/,/\1/,g')
export PATH+=:$(call DOS2UX,$(MSVS))/Common7/IDE

###############################################################################
CL='$(MSVC)/bin/cl.exe' -nologo
LIB='$(MSVC)/bin/lib.exe' -nologo -ignore:4221

# CRT Library Features
# https://msdn.microsoft.com/en-us/library/abx4dbyh.aspx
#CFLAGS+=-O2
#CFLAGS+=-MT
#CFLAGS+=-MD
#CFLAGS+=-WX

#CPPFLAGS+=-D_MT
#CPPFLAGS+=-D_DLL
CPPFLAGS+='-I$(SRCDIR)'
CPPFLAGS+='-I$(MSVC)/include'
CPPFLAGS+='-I$(MSSDK)/include'
CPPFLAGS+='-I$(WASDK)'

###############################################################################
all: libya.lib
#all: libya-sse2.lib
all: libya-debug.lib
#all: libya-debug-sse2.lib
install: $(INCLUDEDIR)/ya.h
install: $(LIBDIR)/libya.lib
#install: $(LIBDIR)/libya-sse2.lib
install: $(LIBDIR)/libya-debug.lib
#install: $(LIBDIR)/libya-debug-sse2.lib
clean:
	@rm -f *.exe *.dll *.lib *.obj

YA_H+=$(SRCDIR)/ya.h
YA_H+=Makefile

LIBYA_OBJ+=ya_player_stub.obj
LIBYA_OBJ+=ya_player.obj
LIBYA_OBJ+=ya_control.obj
LIBYA_OBJ+=ya_queue.obj
LIBYA_OBJ+=ya_queue_write.obj
LIBYA_OBJ+=ya_queue_read.obj
LIBYA_OBJ+=ya_store.obj
LIBYA_OBJ+=ya_result.obj
LIBYA_OBJ+=ya_property.obj
LIBYA_OBJ+=ya_wfxx.obj
LIBYA_OBJ+=str2wcscpy.obj
LIBYA_OBJ+=wcs2strcpy.obj
LIBYA_OBJ+=wcs2strdup.obj
LIBYA_OBJ+=ya_util.obj
LIBYA_OBJ+=ya_trace.obj
LIBYA_OBJ+=ya_dump.obj
LIBYA_OBJ+=ya_debug.obj

.INTERMEDIATE: $(LIBYA_OBJ)

YA_SSE2_OBJ=$(patsubst %.obj,%-sse2.obj,$(YA_OBJ))
LIBYA_SSE2_OBJ=$(patsubst %.obj,%-sse2.obj,$(LIBYA_OBJ))
.INTERMEDIATE: $(LIBYA_SSE2_OBJ)
$(YA_SSE2_OBJ) $(LIBYA_SSE2_OBJ): CFLAGS+=-arch:SSE -arch:SSE2
$(YA_SSE2_OBJ) $(LIBYA_SSE2_OBJ): CPPFLAGS+=-DYA_SSE2

YA_DEBUG_OBJ=$(patsubst %.obj,%-debug.obj,$(YA_OBJ))
LIBYA_DEBUG_OBJ=$(patsubst %.obj,%-debug.obj,$(LIBYA_OBJ))
.INTERMEDIATE: $(LIBYA_DEBUG_OBJ)
#$(YA_DEBUG_OBJ) $(LIBYA_DEBUG_OBJ): CFLAGS+=-arch:IA32
$(YA_DEBUG_OBJ) $(LIBYA_DEBUG_OBJ): CPPFLAGS+=-DYA_DEBUG

YA_DEBUG_SSE2_OBJ=$(patsubst %.obj,%-debug-sse2.obj,$(YA_OBJ))
LIBYA_DEBUG_SSE2_OBJ=$(patsubst %.obj,%-debug-sse2.obj,$(LIBYA_OBJ))
.INTERMEDIATE: $(LIBYA_DEBUG_SSE2_OBJ)
$(YA_DEBUG_SSE2_OBJ) $(LIBYA_DEBUG_SSE2_OBJ): CFLAGS+=-arch:SSE -arch:SSE2
$(YA_DEBUG_SSE2_OBJ) $(LIBYA_DEBUG_SSE2_OBJ): CPPFLAGS+=-DYA_DEBUG -DYA_SSE2

ALL_OBJ+=$(LIBYA_OBJ)
ALL_OBJ+=$(LIBYA_SSE2_OBJ)
ALL_OBJ+=$(LIBYA_DEBUG_OBJ)
ALL_OBJ+=$(LIBYA_DEBUG_SSE2_OBJ)
$(ALL_OBJ): $(YA_H)

libya.lib: $(patsubst %.obj,libya.lib(%.obj),$(LIBYA_OBJ))
libya-sse2.lib: $(patsubst %.obj,libya-sse2.lib(%.obj),$(LIBYA_SSE2_OBJ))
libya-debug.lib: $(patsubst %.obj,libya-debug.lib(%.obj),$(LIBYA_DEBUG_OBJ))
libya-debug-sse2.lib: $(patsubst %.obj,libya-debug-sse2.lib(%.obj),\
		$(LIBYA_DEBUG_SSE2_OBJ))

###############################################################################
$(INCLUDEDIR)/%: $(SRCDIR)/%
	mkdir -p $(@D)
	cp $< $@
$(LIBDIR)/%: %
	mkdir -p $(@D)
	cp $< $@

###############################################################################
# From a command's output remove
# * empty lines,
# * lines consisting of the source name,
# * lines starting with "Creating", and
# * lines starting with "Replacing".
RE=\(^[ \t]*$$\)\|\(^$(<F)$$\)\|\(^[ \t]*Creating\)\|\(^[ \t]*Replacing\)

define exec
	$(1) > x.tmp; y=$$?; sed -n '/$(RE)/!p' x.tmp; rm x.tmp; exit $$y
endef

define CL.obj
	@echo 'CL	$(@F)'
	@$(call exec,$(CL) $(CFLAGS) $(CPPFLAGS) -c -Fo$@ $<)
endef

define LIB.lib
	@echo 'LIB	$(@F)($(<F))'
	@$(call exec, \
	  if test -e $@; then $(LIB) -out:$@ $@ $<; else $(LIB) -out:$@ $<; fi)
endef

(%.obj): %.obj
	$(LIB.lib)
%.obj: $(SRCDIR)/%.c
	$(CL.obj)
%.obj: $(SRCDIR)/%.cpp
	$(CL.obj)
%-sse2.obj: $(SRCDIR)/%.c
	$(CL.obj)
%-debug.obj: $(SRCDIR)/%.c
	$(CL.obj)
%-debug-sse2.obj: $(SRCDIR)/%.c
	$(CL.obj)
