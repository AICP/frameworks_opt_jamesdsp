#pragma once
#include "iir.h"
#include <algorithm>
#include "effect_cequalizer.h"

#include "Biquad4proc.h"
#include "Effect.h"

#define CUSTOM_EQ_PARAM_LOUDNESS_CORRECTION 1000

class EffectEqualizer : public Effect {
    private:
    float mBand[12];
    Biquad4proc mSOS1Band1L, mSOS1Band1R, mSOS1Band2L, mSOS1Band2R, mSOS1Band3L, mSOS1Band3R, mSOS1Band4L, mSOS1Band4R, mSOS1Band5L, mSOS1Band5R, mSOS1Band6L, mSOS1Band6R, mSOS1Band7L, mSOS1Band7R, mSOS1Band8L, mSOS1Band8R, mSOS1Band9L, mSOS1Band9R, mSOS1Band10L, mSOS1Band10R, mPeakFilter11L, mPeakFilter11R, mHSFilter12L, mHSFilter12R;
    Biquad4proc mSOS2Band1L, mSOS2Band1R, mSOS2Band2L, mSOS2Band2R, mSOS2Band3L, mSOS2Band3R, mSOS2Band4L, mSOS2Band4R, mSOS2Band5L, mSOS2Band5R, mSOS2Band6L, mSOS2Band6R, mSOS2Band7L, mSOS2Band7R, mSOS2Band8L, mSOS2Band8R, mSOS2Band9L, mSOS2Band9R, mSOS2Band10L, mSOS2Band10R;
    Biquad4proc mSOS3Band1L, mSOS3Band1R, mSOS3Band2L, mSOS3Band2R, mSOS3Band3L, mSOS3Band3R, mSOS3Band4L, mSOS3Band4R, mSOS3Band5L, mSOS3Band5R, mSOS3Band6L, mSOS3Band6R, mSOS3Band7L, mSOS3Band7R, mSOS3Band8L, mSOS3Band8R, mSOS3Band9L, mSOS3Band9R, mSOS3Band10L, mSOS3Band10R;
    Biquad4proc mSOS4Band9L, mSOS4Band9R, mSOS4Band10L, mSOS4Band10R;
    Biquad4proc mSOS5Band9L, mSOS5Band9R, mSOS5Band10L, mSOS5Band10R;
    int16_t mPreAmp;
    /* Smooth enable/disable */
    int32_t mFade;
    void refreshBands();

    public:
    EffectEqualizer();
    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
