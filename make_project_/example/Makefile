#common makefile header
SLNDIR := $(CURDIR)/../
include $(SLNDIR)/prj.mak/configure.mk

# source code path
BASE_DIR      := $(CURDIR)/../../source
SOURCE_DIR := $(BASE_DIR)/
#MODULE_TARGET    := $(MODULE_LIB)
MODULE_TARGET    := $(MODULE_SO)
#MODULE_TARGET    := $(MODULE_EXEC)

LOCAL_INCLUDE   :=    -I$(BASE_DIR) \
                    -I/fullpath1 \
                    -I/fullpath2 \

                    
LOCAL_FLAG        := -L$(SLNDIR)/../ -DLINUX -DSTDC


EXTERN_OBJS = 

#LOCAL_LIBS = -limg_helper

MODULE_DEPONS = 

LOCAL_SRC = \
            $(SOURCE_DIR)/IMR.cpp \
            
#common makefile foot

include $(SLNDIR)/prj.mak/rules.mk

#automatically trace files.
.KEEP_STATE_FILE: $(MODULE_OUTPUT)
