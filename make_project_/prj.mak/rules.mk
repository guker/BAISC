#common makefile rules
depon-libs := $(shell md="$(MODULE_DEPONS)"; al=""; for i in $$md; do al="$$i $$al"; done; echo $$al)
LOCAL_LIBS := $(depon-libs) $(LOCAL_LIBS)
MODULE_DEPONS_BUILD := $(foreach depon, $(MODULE_DEPONS), $(depon)-build)
MODULE_DEPONS_CLEAN := $(foreach depon, $(MODULE_DEPONS), $(depon)-clean)

ifeq "$(CC_TYPE)" "forte"
	include $(MKPATH)/rules.sun
else
	ifeq "$(CC_TYPE)" "xlc"
		include $(MKPATH)/rules.xlc
	else
		include $(MKPATH)/rules.gcc
	endif
endif
