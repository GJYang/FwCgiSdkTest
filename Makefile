#-------------------------------------------------------------------------------
# Makefile for the seyeon make file template.
#-------------------------------------------------------------------------------
# Author: YK Jung <ykjung@seyeon.co.kr>
# Please edit the followed lines of the "vvvvvvvvvvvvvvv" for your project
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Add the command lines for setting buile envirenmnt in ".bashrc" of your home 
#-------------------------------------------------------------------------------
#	export FW_SDK_ROOT_V40=~/FlexWatchV41Sdk
#	PATH=$PATH:/opt/arm-2009q1/bin
#-------------------------------------------------------------------------------
FW_SDK_ROOT_V40 = /home/sung/FlexWatchV41Sdk
#-------------------------------------------------------------------------------

#GFV_TMN			= FW_DM36XV2
#GFV_OEM 		= FV_OEM_SEYEON
#GFV_PLATFORM 	= FV_PFM_DM36XV2
#GFV_CROSS 		= /opt/arm-2009q1/bin/arm-none-linux-gnueabi-
#GFV_KERNEL 		= /home/h2y/ti-dvsdk_dm368-evm_4_02_00_06/psp/linux-2.6.32.17-sy

GFV_TMN			= PC_5K
GFV_OEM 		= FV_OEM_SEYEON
GFV_PLATFORM 	= FV_PFM_X86
GFV_CROSS 		= 
GFV_KERNEL 		= /usr/src/linux-headers-3.8.0.29

#-------------------------------------------------------------------------------
# global
# path
ROOT_SRC_DIR = $(FW_SDK_ROOT_V40)/source
ROOT_LIB_DIR = $(FW_SDK_ROOT_V40)/lib/$(GFV_TMN)
ROOT_BIN_DIR = $(FW_SDK_ROOT_V40)/bin/$(GFV_TMN)
LIB_DIR += -L$(ROOT_LIB_DIR)
INC_DIR += -I. -I$(ROOT_SRC_DIR)/Common -I$(ROOT_SRC_DIR)/Drivers/include 
INC_DIR += -I$(ROOT_SRC_DIR) -I$(FW_SDK_ROOT_V40)/include
DEFINES += -D$(GFV_PLATFORM)
#-------------------------------------------------------------------------------


################################################################################
# Add Local Definitions
# EX: DEFINES += -D__DEFINITION__=1
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# DEFINES += -m32

################################################################################
# Include & Library Directorys
# EX: INC_DIRS += -I../IncDir 
# EX: LIB_DIRS += -L../LibDir 
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#INC_DIRS += -I..
#LIB_DIRS += 
#//// tweaked by Sungbo Kang ////////////////////////////////////
INC_DIRS += -I$(ROOT_SRC_DIR)/FFmpegLib/include
LIB_DIRS += -L$(ROOT_SRC_DIR)/FFmpegLib/lib
#////////////////////////////////////////////////////////////////

################################################################################
# Sub Directorys
# EX: SUB_DIRS += ./SubDirs
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#SUB_DIRS += ../

################################################################################
# Executable Target name : Executable File Target
# If you want to build multiple targets, then you can use EXC_TARGETS
# At this time, the ".c" file name must be the same as the targets name
#EXC_TARGETS = ExcTarget1 ExcTarget2
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
EXC_TARGET = FwCgiSdkTest

################################################################################
# Library Target name : Static/Dynamic Library Target
# 1.Static  Library Target: libXxxxYyyy.a
# 2.Dynamic Library Target: libXxxxYyyy.so.1.0.1
# 3.Dynamic Library SoName: libSsssOooo.so
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#//// tweaked by Sungbo Kang ///////////////////////////////////////////////////////////////////
# LIB_TARGET = 
#///////////////////////////////////////////////////////////////////////////////////////////////
#SO_NAME = 

################################################################################
# Source Files
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
SRCS = FwCgiSdkTest.c
#LIB_SRCS = LibSource.c

################################################################################
# LIBS Definitions : libXxxxYyyy.a --> -lXxxxYyyy
#vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
LIBS += -lpthread -lFwCgiLib -lSockUtil -lBase64 -lMd5 -lJes
#//// tweaked by Sungbo Kang ////////////////////////////////////
LIBS += -lavformat -lavcodec -lavutil -lavfilter -lavdevice -lm
#////////////////////////////////////////////////////////////////


################################################################################
################################################################################
################################################################################
#-------------------------------------------------------------------------------
# MakefileTargetRule for the seyeon make file template.
#-------------------------------------------------------------------------------
# Author: YK Jung <ykjung@seyeon.co.kr>
#-------------------------------------------------------------------------------

################################################################################
#-------------------------------------------------------------------------------
# Common Definitions
#-------------------------------------------------------------------------------
CROSS = $(GFV_CROSS)
CC = $(CROSS)gcc
# // tweaked by SungboKang //////
# CC = $(CROSS)gcc -v
# ///////////////////////////////
#CC = $(CROSS)g++
LD = $(CROSS)ld
AR = $(CROSS)ar
STRIP = $(CROSS)strip --strip-all
RANLIB = $(CROSS)ranlib
LN = ln

#-------------------------------------------------------------------------------
# Include & Library Directorys
#-------------------------------------------------------------------------------
INC_DIRS += -I. -I$(ROOT_SRC_DIR)/Common -I$(ROOT_SRC_DIR)/Drivers/include 
LIB_DIRS += -L$(ROOT_LIB_DIR)

#-------------------------------------------------------------------------------
# FLAGS Definitions
#-------------------------------------------------------------------------------
# CFLAGS = -Wall $(INC_DIRS) $(DBGFLAGS) $(DEBFLAGS) $(DEFINES)
# // tweaked by Sungbo Kang /////// add -g option to eanble debugging ///
CFLAGS = -g -Wall $(INC_DIRS) $(DBGFLAGS) $(DEBFLAGS) $(DEFINES)
# ///////////////////////////////////////////////////////////////////////
ARFLAGS = crv

#-------------------------------------------------------------------------------
# Object Files
#-------------------------------------------------------------------------------
OBJS = ${SRCS:.c=.o}
LIB_OBJS = ${LIB_SRCS:.c=.o}


################################################################################
#-------------------------------------------------------------------------------
# Target Commands
#-------------------------------------------------------------------------------

all : lib_target exc_target exc_targets
	@echo "============================================================"
	@echo "= MAKE BUILD COMPLETED !! "
	@echo "============================================================"

lib_target: $(SUB_DIRS) $(LIB_TARGET)
	@for dir in $(SUB_DIRS); do \
		echo "==> Enter the $$dir for libs <=="; \
		(cd $$dir; $(MAKE) lib_target) \
	done 

exc_target: $(SUB_DIRS) $(EXC_TARGET)
	@for dir in $(SUB_DIRS); do \
		echo "==> Enter the $$dir for target <=="; \
		(cd $$dir; $(MAKE) exc_target) \
	done 

exc_targets: $(SUB_DIRS) $(EXC_TARGETS)
	@for dir in $(SUB_DIRS); do \
		echo "==> Enter the $$dir for target <=="; \
		(cd $$dir; $(MAKE) exc_targets) \
	done 

#########################################################################
#all : $(SUB_DIRS) $(LIB_TARGET) $(EXC_TARGET)
#	@for dir in $(SUB_DIRS); do \
#		echo "==> Enter the $$dir for doing make <=="; \
#		(cd $$dir; make) \
#	done
#########################################################################

#-------------------------------------------------------------------------------
clean: $(SUB_DIRS)
	@for dir in $(SUB_DIRS); do \
		echo "==> Enter the $$dir for doing make <=="; \
		(cd $$dir; make clean) \
	done  
	rm -rf *.o *~ *.bak $(OBJS) $(LIB_OBJS)	$(LIB_TARGET) $(EXC_TARGET) $(EXC_TARGETS) $(SO_NAME) core*
## tweaked by SungboKang #####
	rm -f VIDEO*
	rm -f playlist.m3u8
##############################
ifneq ($(LIB_TARGET),)
	rm -rf $(ROOT_LIB_DIR)/$(LIB_TARGET)
endif
ifneq ($(SO_NAME),)
	rm -rf $(ROOT_LIB_DIR)/$(SO_NAME)
endif
#ifneq ($(EXC_TARGET),)
#	rm -rf $(ROOT_BIN_DIR)/$(EXC_TARGET)
#endif

#-------------------------------------------------------------------------------
depend:
	@echo "=== Updating dependencies in $(CURDIR) ==="
	/bin/sh -ec '$(CC) -MM $(DEFINES) $(CFLAGS) $(INC_DIRS) $(SRCS) $(LIB_SRCS) \
	| sed '\''s/\($*\)\.o[ :]*/\1.o dependency.mak : /g'\'' > dependency.mak; \
	[ -s dependency.mak ] || rm -f dependency.mak' 
	@for dir in $(SUB_DIRS); do \
		echo "=== make depend $$dir ===";\
		(cd $$dir; \
		$(MAKE) depend; ) \
	done 

#-------------------------------------------------------------------------------
release:
	make clean
	make all "DEBFLAGS=-O2 -DNDEBUG"

#-------------------------------------------------------------------------------
debug:
	make clean
	make all "DBGFLAGS=-g -D_DEBUG"

#-------------------------------------------------------------------------------
help:
	@echo "================================================================================"
	@echo "= SEYEON BUILD SYSTEM !! "
	@echo "================================================================================"
	@echo "make [TMN=FW_2800|FW_ELAN|PC_5K|FW_PENTA|FW_PENTA_DECODER|FW_OXFORD|FW_DM365"
	@echo "     |FW_EPSON|S2PQ|S2ND|S2ND_DVR|WEBBOX|WEBCAM|WEBCAM_W|FW_NVS2200|FW_DM36XV2"
	@echo "     |FW_DM816X]"
	@echo "     [release | debug | depend | clean]"
	@echo "================================================================================"

################################################################################
#-------------------------------------------------------------------------------
# 1.Build for Executable File Target[$^ == $(OBJS) : Prerequest List]
#-------------------------------------------------------------------------------
$(EXC_TARGET): $(OBJS)
	@echo "============================================================"
	@echo "=  Build EXC: $@ for TMN=$(TMN)"
	@echo "============================================================"
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_DIRS) $(LIBS)
	cp -a $(EXC_TARGET) $(ROOT_BIN_DIR)
	$(STRIP) $(ROOT_BIN_DIR)/$(EXC_TARGET)

#-------------------------------------------------------------------------------
# 2.Build for Executable File Target[$^ == $(OBJS) : Prerequest List]
#-------------------------------------------------------------------------------
.SECONDEXPANSION:
$(EXC_TARGETS): $$@.o
	@echo "============================================================"
	@echo "=  Build EXC: $@ for TMN=$(TMN)"
	@echo "============================================================"
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_DIRS) $(LIBS)
	cp -a $@ $(ROOT_BIN_DIR)
	$(STRIP) $(ROOT_BIN_DIR)/$@

#-------------------------------------------------------------------------------
# 3.Build for Static/Dynamic Library Target
#-------------------------------------------------------------------------------
$(LIB_TARGET): $(LIB_OBJS)
	@echo "============================================================"
	@echo "=  Build LIB: $(LIB_TARGET) for TMN=$(TMN)"
	@echo "============================================================"
ifeq ($(SO_NAME),)
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@
	cp -a $(LIB_TARGET) $(ROOT_LIB_DIR)
else
	$(CC) -shared -Wl,-soname,$(SO_NAME) -o $@ $^
	$(LN) -fs $@ $(SO_NAME)
	cp -a $(LIB_TARGET) $(ROOT_LIB_DIR)
	cp -a $(SO_NAME) $(ROOT_LIB_DIR)
endif

################################################################################
#-------------------------------------------------------------------------------
# Include Dependency File
#-------------------------------------------------------------------------------
-include dependency.mak

################################################################################
#-------------------------------------------------------------------------------
# Dependency
#-------------------------------------------------------------------------------
# DO NOT DELETE
