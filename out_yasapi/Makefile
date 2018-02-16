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
include $(SRCDIR)/version.mak
.PHONY: all install clean

###############################################################################
DOS2UX=$(shell echo '$(1)'|sed -e 's,^\(.\):/,/\1/,g')
export PATH+=:$(call DOS2UX,$(MSVS))/Common7/IDE

###############################################################################
#OUT_YASAPI_SFX=-2
OUT_YASAPI=out_yasapi$(OUT_YASAPI_SFX)

###############################################################################
RC='$(MSSDK)/bin/rc.exe' -nologo
CL='$(MSVC)/bin/cl.exe' -nologo
LIB='$(MSVC)/bin/lib.exe' -nologo -ignore:4221

RCFLAGS+=$(CPPFLAGS)
RCFLAGS+='-I$(MSVC)/include'
RCFLAGS+='-I$(MSSDK)/include'

# CRT Library Features
# https://msdn.microsoft.com/en-us/library/abx4dbyh.aspx
#CFLAGS+=-O2
#CFLAGS+=-MT
#CFLAGS+=-MD
#CFLAGS+=-WX

#CPPFLAGS+=-D_MT
#CPPFLAGS+=-D_DLL
ifneq (,$(OUT_YASAPI_SFX)) # {
CPPFLAGS+='-DOUT_YASAPI_SFX=$(OUT_YASAPI_SFX)'
endif # }
CPPFLAGS+='-DYASAPI_VER=$(OUT_YASAPI_VERSION)'
CPPFLAGS+='-I$(SRCDIR)'
CPPFLAGS+='-I$(SRCDIR)/rc'
CPPFLAGS+='-I$(INCLUDEDIR)'
CPPFLAGS+='-I$(WASDK)'
CPPFLAGS+='-I$(MSVC)/include'
CPPFLAGS+='-I$(MSSDK)/include'

#LDFLAGS+=-subsystem:windows
LDFLAGS+='-libpath:$(MSVC)/lib'
LDFLAGS+='-libpath:$(MSSDK)/lib'

LDLIBS+=uxtheme.lib
LDLIBS+=winmm.lib
LDLIBS+=ksguid.lib
LDLIBS+=mfplat.lib
LDLIBS+=avrt.lib
LDLIBS+=comctl32.lib
LDLIBS+=ole32.lib
LDLIBS+=gdi32.lib
LDLIBS+=user32.lib
LDLIBS+=kernel32.lib

###############################################################################
ALL+=$(OUT_YASAPI).dll
ALL+=$(OUT_YASAPI)-debug.dll
all: $(ALL)
all: guid.exe
all: model.exe
guid.exe: guid.obj
clean:
	@rm -f *.exe *.dll *.lib *.obj

YASAPI_H+=$(INCLUDEDIR)/ya.h
YASAPI_H+=$(SRCDIR)/yasapi.h
YASAPI_H+=$(SRCDIR)/yasapi_guid.h
YASAPI_H+=resources.h
YASAPI_H+=Makefile
YASAPI_H+=$(SRCDIR)/version.mak

YASAPI_OBJ+=out_yasapi.obj
YASAPI_OBJ+=resources.res

LIBYASAPI_OBJ+=yasapi_about.obj
LIBYASAPI_OBJ+=yasapi_config.obj
LIBYASAPI_OBJ+=yasapi_control_list.obj
LIBYASAPI_OBJ+=yasapi_notify.obj
LIBYASAPI_OBJ+=yasapi_player.obj
LIBYASAPI_OBJ+=yasapi_connect.obj
LIBYASAPI_OBJ+=yasapi_device.obj
LIBYASAPI_OBJ+=yasapi_disconnect.obj
LIBYASAPI_OBJ+=yasapi_strategy.obj
LIBYASAPI_OBJ+=yasapi_kill.obj
LIBYASAPI_OBJ+=yasapi_time.obj
LIBYASAPI_OBJ+=yasapi_options_device.obj
LIBYASAPI_OBJ+=yasapi_options_common.obj
LIBYASAPI_OBJ+=yasapi_ring.obj
LIBYASAPI_OBJ+=yasapi_util.obj
LIBYASAPI_OBJ+=yasapi_guid.obj

.INTERMEDIATE: $(LIBYASAPI_OBJ)

#$(YASAPI_OBJ) $(LIBYASAPI_OBJ): CFLAGS+=-arch:IA32

YASAPI_SSE2_OBJ=$(patsubst %.obj,%-sse2.obj,$(YASAPI_OBJ))
LIBYASAPI_SSE2_OBJ=$(patsubst %.obj,%-sse2.obj,$(LIBYASAPI_OBJ))
.INTERMEDIATE: $(LIBYASAPI_SSE2_OBJ)
$(YASAPI_SSE2_OBJ) $(LIBYASAPI_SSE2_OBJ): CFLAGS+=-arch:SSE -arch:SSE2
$(YASAPI_SSE2_OBJ) $(LIBYASAPI_SSE2_OBJ): CPPFLAGS+=-DYA_SSE2

YASAPI_DEBUG_OBJ=$(patsubst %.obj,%-debug.obj,$(YASAPI_OBJ))
LIBYASAPI_DEBUG_OBJ=$(patsubst %.obj,%-debug.obj,$(LIBYASAPI_OBJ))
.INTERMEDIATE: $(LIBYASAPI_DEBUG_OBJ)
#$(YASAPI_DEBUG_OBJ) $(LIBYASAPI_DEBUG_OBJ): CFLAGS+=-arch:IA32
$(YASAPI_DEBUG_OBJ) $(LIBYASAPI_DEBUG_OBJ): CPPFLAGS+=-DYA_DEBUG

YASAPI_DEBUG_SSE2_OBJ=$(patsubst %.obj,%-debug-sse2.obj,$(YASAPI_OBJ))
LIBYASAPI_DEBUG_SSE2_OBJ=$(patsubst %.obj,%-debug-sse2.obj,$(LIBYASAPI_OBJ))
.INTERMEDIATE: $(LIBYASAPI_DEBUG_SSE2_OBJ)
$(YASAPI_DEBUG_SSE2_OBJ) $(LIBYASAPI_DEBUG_SSE2_OBJ): CFLAGS+=-arch:SSE -arch:SSE2
$(YASAPI_DEBUG_SSE2_OBJ) $(LIBYASAPI_DEBUG_SSE2_OBJ): CPPFLAGS+=-DYA_DEBUG -DYA_SSE2

ALL_OBJ+=$(LIBYASAPI_OBJ)
ALL_OBJ+=$(YASAPI_OBJ)
ALL_OBJ+=$(LIBYASAPI_SSE2_OBJ)
ALL_OBJ+=$(YASAPI_SSE2_OBJ)
ALL_OBJ+=$(LIBYASAPI_DEBUG_OBJ)
ALL_OBJ+=$(YASAPI_DEBUG_OBJ)
ALL_OBJ+=$(LIBYASAPI_DEBUG_SSE2_OBJ)
ALL_OBJ+=$(YASAPI_DEBUG_SSE2_OBJ)
$(ALL_OBJ): $(YASAPI_H)

$(OUT_YASAPI).dll: $(YASAPI_OBJ) libyasapi.lib $(LIBDIR)/libya.lib
$(OUT_YASAPI)-sse2.dll: $(YASAPI_SSE2_OBJ) libyasapi-sse2.lib \
		$(LIBDIR)/libya-sse2.lib
$(OUT_YASAPI)-debug.dll: $(YASAPI_DEBUG_OBJ) libyasapi-debug.lib \
		$(LIBDIR)/libya-debug.lib
$(OUT_YASAPI)-debug-sse2.dll: $(YASAPI_DEBUG_SSE2_OBJ) \
		libyasapi-debug-sse2.lib $(LIBDIR)/libya-debug-sse2.lib
model.exe: model.obj

libyasapi.lib: $(patsubst %.obj,libyasapi.lib(%.obj),$(LIBYASAPI_OBJ))
libyasapi-sse2.lib: $(patsubst %.obj,libyasapi-sse2.lib(%.obj),$(LIBYASAPI_SSE2_OBJ))
libyasapi-debug.lib: $(patsubst %.obj,libyasapi-debug.lib(%.obj),$(LIBYASAPI_DEBUG_OBJ))
libyasapi-debug-sse2.lib: $(patsubst %.obj,libyasapi-debug-sse2.lib(%.obj),$(LIBYASAPI_DEBUG_SSE2_OBJ))

###############################################################################
resources.res: resources.rc resources.h manifest.xml
resources.rc: Makefile $(SRCDIR)/version.mak

RCFILTER+=-e 's/\(PRODUCTVERSION \+\)0,0,0,0/\1$(YASAPI_VERSION_MAJOR),$(YASAPI_VERSION_MINOR),$(YASAPI_VERSION_PATCH),0/g'
RCFILTER+=-e 's/\(FILEVERSION \+\)0,0,0,0/\1$(YASAPI_VERSION_MAJOR),$(YASAPI_VERSION_MINOR),$(YASAPI_VERSION_PATCH),0/g'
RCFILTER+=-e 's/VOS_UNKNOWN/VOS__WINDOWS32/g'
RCFILTER+=-e 's/VFT_UNKNOWN/VFT_DLL/g'
RCFILTER+=-e 's/\(VALUE "ProductVersion",\) ""/\1 "$(OUT_YASAPI_VERSION)"/g'
#RCFILTER+=-e 's/\(LTEXT\)\( *"Copy", \)\(IDC_STATIC_COPY\)/PUSHBUTTON "SysLink", IDC_STATIC_COPY/g'
#RCFILTER+=-e 's/\(LTEXT\)\( *"Copy", \)\(IDC_STATIC_COPY\)/CONTROL  "Copy", IDC_STATIC_COPY, "SysLink" /g'
#RCFILTER+=-e 's/\(LTEXT\)\( *"Copy", \)\(IDC_STATIC_COPY\)/CONTROL  "Copyright \xA9 2015-2016 Peter Belkner.\r\n\r\nFor further details refer to <a href=\"http://out-yasapi.sourceforge.net/\">http://out-yasapi.sourceforge.net/</a>.", IDC_STATIC_COPY, "SysLink" /g'
#RCFILTER+=-e 's/LTEXT\(.*, \)IDC_STATIC_COPY/CONTROL \1 IDC_STATIC_COPY, "SysLink" /g'
RCFILTER+=-e 's|LTEXT\(.*\)"\(.*\)", IDC_STATIC_COPY|LTEXT\1"Copyright (C) 2015-2016 Peter Belkner.\\n\\r\\n\\rFor further details refer to \"\"http://out-yasapi.sourceforge.net/\"\"", IDC_STATIC_COPY|g'
#RCFILTER+=-e 's|LTEXT  \(.*\)"\(.*\)", IDC_STATIC_COPY|CONTROL\1"Copyright (C) 2015-2016 Peter Belkner.\\n\\r\\n\\rFor further details refer to \"\"http://out-yasapi.sourceforge.net/\"\"", IDC_STATIC_COPY, "SysLink"|g'

%.rc: $(SRCDIR)/rc/%.rc
	@echo 'SED	$(@F)'
	@sed $(RCFILTER) $< > $@
%.h: $(SRCDIR)/rc/%.h
	cp $< $@
manifest.xml: $(SRCDIR)/rc/manifest.xml
	cp $< $@

###############################################################################
$(BINDIR)/%: %
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

define CL.exe
	@echo 'LINK	$(@F)'
	@$(call exec, \
	  $(CL) $(CFLAGS) $(CPPFLAGS) -Fe$@ $^ -link $(LDFLAGS) $(LDLIBS))
endef

define CL.dll
	@echo 'LINK	$(@F)'
	@$(call exec, \
	  $(CL) -LD $(CFLAGS) $(CPPFLAGS) -Fe$@ $^ -link $(LDFLAGS) $(LDLIBS))
endef

define LIB.lib
	@echo 'LIB	$(@F)($(<F))'
	@$(call exec, \
	  if test -e $@; then $(LIB) -out:$@ $@ $<; else $(LIB) -out:$@ $<; fi)
endef

define RC.res
	@echo 'RC	$(@F)'
	@$(call exec,$(RC) $(RCFLAGS) -fo$@ $<)
endef

%.dll:
	$(CL.dll)
%.exe:
	$(CL.exe)
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
%.res: %.rc
	$(RC.res)
