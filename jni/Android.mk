LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libjamesdsp

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

include $(BUILD_SHARED_LIBRARY)
