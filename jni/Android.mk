LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libjamesdsp
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	kissfft/kiss_fft.c \
	kissfft/kiss_fftr.c \
	jamesdsp.cpp \
	Effect.cpp \
	EffectDSPMain.cpp \
	JLimiter.c \
	reverb.c \
	compressor.c \
	AutoConvolver.c \
	mnspline.c \
	ArbFIRGen.c \
	vdc.c \
	bs2b.c \
	valve/12ax7amp/Tube.c \
	valve/12ax7amp/wdfcircuits_triode.c

LOCAL_CPPFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-unused-parameter

ifeq ($(BUILD_WITHOUT_VENDOR), true)
LOCAL_POST_INSTALL_CMD := \
	$(eval _overlay_path := $(TARGET_OUT_PRODUCT)/vendor_overlay/$(PRODUCT_TARGET_VNDK_VERSION)) \
	$(foreach l, lib $(if $(filter true,$(TARGET_IS_64_BIT)),lib64), \
	  $(eval _src := $(TARGET_OUT_VENDOR)/$(l)/soundfx/libjamesdsp.so) \
	  $(eval _dst := $(_overlay_path)/$(l)/soundfx/libjamesdsp.so) \
	  mkdir -p $(dir $(_dst)) && $(ACP) $(_src) $(_dst) ; \
	)
endif

include $(BUILD_SHARED_LIBRARY)
