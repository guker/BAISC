#define Makefile variable in multi-platform

#define some utilites
RM		:=/bin/rm -f
CP		:=/bin/cp
MV		:=/bin/mv -f
MKDIR	:=/bin/mkdir

prefix = /usr/local/epg/
#prefix = ./

#release = true
m64 = true
#V:=true

DEBUG_OUTPUT	= xdebug
RELEASE_OUTPUT	= xrelease

ifdef m64
	DEBUG_OUTPUT	= x64debug
	RELEASE_OUTPUT	= x64release
endif

OUTPUT = $(DEBUG_OUTPUT)

ifdef release
	OUTPUT = $(RELEASE_OUTPUT)
endif

ifdef m64
	COMMON_LIB_DIR	:= $(SLNDIR)/../../lib_x64
else
	COMMON_LIB_DIR	:= $(SLNDIR)/../../lib
endif

ifdef m64
	COMMON_BIN_DIR	:= $(SLNDIR)/../../bin_x64
else
	COMMON_BIN_DIR	:= $(SLNDIR)/../../bin
endif


MKPATH			:= $(SLNDIR)/prj.mak
SCRIPTS			:= $(SHELL) $(MKPATH)/utils.sh

MODULE_DIR		:= $(CURDIR)
MODULE_NAME		:= $(notdir $(MODULE_DIR))

MODULE_OUTPUT	= $(MODULE_DIR)/$(OUTPUT)

ifdef release
	MODULE_SO		= lib$(MODULE_NAME).so$(ver)
	MODULE_EXEC		= $(MODULE_NAME)
	MODULE_LIB		= $(MODULE_NAME).a
	MODULE_TARGET	= $(MODULE_LIB)
else
	MODULE_SO		= lib$(MODULE_NAME).so
	MODULE_EXEC		= $(MODULE_NAME)
	MODULE_LIB		= $(MODULE_NAME).a
	MODULE_TARGET	= $(MODULE_LIB)
endif

LOCAL_INCLUDE	=
LOCAL_LIBS		=
LOCAL_FLAG		=
LOCAL_FLAGS		= $(LOCAL_FLAG) $(LOCAL_INCLUDE)

HOST_TYPE	:=$(shell uname)
#if you do not define CC_TYPE, I'll choose it automatically.
ifeq "$(CC_TYPE)"	""
ifeq "$(HOST_TYPE)" "SunOS"
	CC_TYPE	:=	forte
else
	ifeq "$(HOST_TYPE)" "AIX"
		CC_TYPE := 	xlc
	else
		CC_TYPE	:=	gcc
	endif
endif
endif

ifeq "$(CC_TYPE)" "forte"
	include $(MKPATH)/configure.sun
else
	ifeq "$(CC_TYPE)" "xlc"
		include $(MKPATH)/configure.xlc
	else
		include $(MKPATH)/configure.gcc
	endif
endif
