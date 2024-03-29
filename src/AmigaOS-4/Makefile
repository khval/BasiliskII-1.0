 # AmigaOS4 makefile for Basilisk II

## System specific configuration
STRIP = echo

CC = g++   -D__USE_INLINE__  # -DENABLE_EXCLUSIVE_SPCFLAGS
CXX = g++   -D__USE_INLINE__  # -DENABLE_EXCLUSIVE_SPCFLAGS
CXXFLAGS = -athread=native -g  -funroll-loops -finline-functions -ffast-math -mcpu=740  -mmultiple ## -Wall -Wno-multichar

CPUDIR = cpu_build

CPPFLAGS = -I../include -I${CPUDIR} -I.
DEFS = 
LDFLAGS =  -lpthread
LIBS = 
AS = as
ASFLAGS =
CATCOMP = catcomp
NULL = NIL:
target_lha = RAM:Basilisk2

ECHO = echo
ECHE = echo -e
BOLD = *"*E[1m*
NRML = *"*E[0m*

COMPILING = @$(ECHE) "compiling $(BOLD)$@$(NRML)..."
LINKING = @$(ECHE) "linking $(BOLD)$@$(NRML)..."
STRIPPING = @$(ECHE) "stripping $(BOLD)$@$(NRML)..."
ARCHIVING = @$(ECHE) "archiving $(BOLD)$@$(NRML)..."

## Files
CPUSRCS = $(CPUDIR)/basilisk_glue.cpp $(CPUDIR)/newcpu.cpp $(CPUDIR)/memory.cpp	\
	$(CPUDIR)/readcpu.cpp $(CPUDIR)/fpu/fpu_uae.cpp cpustbl.cpp $(CPUDIR)/cpudefs.cpp cpuemu.cpp

#CPUSRCS = ../powerrom_cpu/powerrom_cpu.cpp

SRCS = ../main.cpp main_amiga.cpp stccpy.cpp ../prefs.cpp ../prefs_items.cpp \
    prefs_amiga.cpp sys_amigaos4.cpp ../rom_patches.cpp sysdeps.cpp \
    ../slot_rom.cpp ../rsrc_patches.cpp ../emul_op.cpp \
    ../macos_util.cpp ../xpram.cpp xpram_amigaos4.cpp ../timer.cpp \
    timer_amigaos4.cpp clip_amigaos4.cpp ../adb.cpp ../serial.cpp \
    serial_amiga.cpp  ../ether.cpp  ../sony.cpp ../disk.cpp \
    ../cdrom.cpp ../scsi.cpp scsi_amiga.cpp ../video.cpp \
	video_amigaos4.cpp \
	window_icons.cpp \
	iconify.cpp \
	spawn.cpp \
	init.cpp \
	req.cpp \
	comp.cpp \
	asl.cpp \
	version.cpp \
	DeviceClip.cpp \
	../debug.cpp \
	video_driver_screen.cpp \
	video_driver_window.cpp \
	video_driver_window_comp.cpp \
    ../audio.cpp  ../extfs.cpp  ../extfs_amigaos4.cpp \
    ../user_strings.cpp user_strings_amigaos4.cpp prefs_editor_amigaos4.cpp   \
	$(CPUSRCS)

# SRCS +=	ether_amiga.cpp 
	SRCS += ../dummy/ether_dummy.cpp

	SRCS += audio_amigaos4.cpp
#	SRCS += ../dummy/audio_dummy.cpp

# SRCS += video_convert.cpp 
# SRCS += ../sdl/audio_sdl.cpp

APP = BasiliskII

## Rules
.PHONY: clean distclean
.SUFFIXES:
.SUFFIXES: .c .cpp .asm .o .h

all: update_builddate make_locale $(APP)

OBJ_DIR = obj
$(OBJ_DIR)::
	;@[ -d $(OBJ_DIR) ] || mkdir $(OBJ_DIR) > $(NULL) 

define SRCS_LIST_TO_OBJS
	$(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(foreach file, $(SRCS), \
	$(basename $(notdir $(file))))))
endef
OBJS =  $(SRCS_LIST_TO_OBJS) 

SRC_PATHS += $(sort $(foreach file, $(SRCS), $(dir $(file))))
VPATH :=
VPATH += $(addprefix :, $(subst  ,:, $(filter-out $($(subst, :, ,$(VPATH))), $(SRC_PATHS))))

$(APP): $(OBJ_DIR) $(OBJS)
	$(LINKING)
	$(CXX) -athread=native -o $(APP).db $(LDFLAGS) $(OBJS) $(LIBS)  -ggdb
	@;$(STRIPPING)
	@;$(STRIP) --remove-section=.comment $(APP).db -o $(APP)
	@;protect $(APP) +e

clean:
	rm -f $(APP) $(OBJ_DIR)/* *.db *.s obj.0000.*
	delete $(OBJ_DIR)/#? ALL

dump:
	objdump --disassemble BasiliskII.db >BasiliskII.s

make_locale: locale/locale.c locale/locale.h

locale/basilisk.ct: locale/basilisk.cd
	@$(CATCOMP) locale/basilisk.cd CTFILE locale/basilisk.ct

locale/locale.h: locale/basilisk.cd locale/basilisk.ct
	@$(CATCOMP) locale/basilisk.cd CFILE locale/locale.h NOSTRINGS NOARRAY NOBLOCK NOCODE

locale/locale.c: locale/basilisk.cd locale/basilisk.ct
	@$(CATCOMP) locale/basilisk.cd CFILE locale/locale.c NOBLOCK NOCODE

distclean: clean
	rm -rf $(OBJ_DIR)

obj/ether_amiga4.o:
	$(COMPILING)
	@echo ggdb
	$(CC) ether_amiga4.cpp $(CPPFLAGS) $(DEFS) $(CXXFLAGS) -D__USE_INLINE__ -c -o obj/ether_amiga4.o -ggdb

obj/ether_os4_infinity.o:
	$(COMPILING)
	$(CC) ether_os4_infinity.cpp $(CPPFLAGS) $(DEFS) $(CXXFLAGS) -D__USE_INLINE__ -c -o obj/ether_os4_infinity.o -ggdb

protocal_api.o:
	$(COMPILING)
	$(CC) protocal_api.c $(CPPFLAGS) $(DEFS) $(CXXFLAGS) -D__USE_INLINE__ -c  -o obj/protocal_api.o

##	gcc protocal_api.c -o $(OBJ_DIR)/protocal_api.o -D__USE_INLINE__ -c

obj/prefs_editor_amigaos4.o: prefs_editor_amigaos4.cpp gui_pages/page_disk.i gui_pages/page_add_diskimage.i gui_pages/page_cpu.i gui_pages/page_gfx.i locale/locale.c
	$(COMPILING)
	$(CXX) -dM $(CPPFLAGS) $(DEFS) $(CXXFLAGS)  -c $< -o $@

sdl/sdl_audio.o: ../SDL/sdl_audio.cpp
	$(COMPILING)
	$(CXX) -DITimer=_ITimer -dM $(CPPFLAGS) $(DEFS) $(CXXFLAGS)  -c $< -o $@

#obj/main.o: ../main.cpp
#	$(COMPILING)
#	$(CC) ../main.cpp $(CPPFLAGS) $(DEFS) $(CXXFLAGS) -c -o obj/main.o

$(OBJ_DIR)/%.o : %.cpp
	$(COMPILING)
	$(CXX) $(CPPFLAGS) $(DEFS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : %.c
	$(COMPILING)
	@$(CC)   $(CPPFLAGS) $(DEFS) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/cpuopti: $(OBJ_DIR)/cpuopti.o
	$(COMPILING)
	@$(CC) $(LDFLAGS) -o $(OBJ_DIR)/cpuopti $(OBJ_DIR)/cpuopti.o
$(CPUDIR)/build68k: $(OBJ_DIR)/build68k.o
	$(COMPILING)
	@$(CC) $(LDFLAGS) -o $(CPUDIR)/build68k $(OBJ_DIR)/build68k.o
$(OBJ_DIR)/gencpu: $(OBJ_DIR)/gencpu.o $(OBJ_DIR)/readcpu.o $(OBJ_DIR)/cpudefs.o
	$(COMPILING)
	$(CC)  -athread=native $(LDFLAGS) -o $(OBJ_DIR)/gencpu $(OBJ_DIR)/gencpu.o $(OBJ_DIR)/readcpu.o $(OBJ_DIR)/cpudefs.o

$(CPUDIR)/cpudefs.cpp: $(CPUDIR)/build68k $(CPUDIR)/table68k
	$(COMPILING)
	@$(CPUDIR)/build68k < $(CPUDIR)/table68k > $(CPUDIR)/cpudefs.cpp

$(CPUDIR)/table68k:
	copy /uae_cpu_OLD/#? $(CPUDIR) ALL

cpuemu.cpp: $(OBJ_DIR)/gencpu
	$(COMPILING)
	@$(OBJ_DIR)/gencpu

update_builddate: update_builddate1 version.o

update_builddate1:
	@echo "#include <version.h>" > version.cpp
	@c:date LFORMAT "const char **ScreenTitle=VERSION_STRING*" (%d.%m.%Y)*";" >> version.cpp
	@echo "" >> version.cpp

cpustbl.cpp: cpuemu.cpp
cputbl.h: cpuemu.cpp

$(OBJ_DIR)/cpuemu1.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_1 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu2.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_2 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu3.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_3 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu4.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_4 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu5.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_5 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu6.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_6 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu7.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_7 $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/cpuemu8.o: cpuemu.cpp
	$(CXX) $(CPPFLAGS) $(DEFS) -DPART_8 $(CXXFLAGS) -c $< -o $@

#cpufast.s: cpuemu.cpp $(OBJ_DIR)/cpuopti
#	$(CXX) $(INCLUDES) -S $(CFLAGS) $< -o cputmp.s
#	$(OBJ_DIR)/cpuopti <cputmp.s >$@ || mv cputmp.s $@
#	rm -f cputmp.s

#-------------------------------------------------------------------------
# DO NOT DELETE THIS LINE -- make depend depends on it.

lha:
		execute make_lha $(target_lha)
