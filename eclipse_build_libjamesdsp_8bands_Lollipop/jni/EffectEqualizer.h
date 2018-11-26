#pragma once

#include "effect_cequalizer.h"

#include "Biquad.h"
#include "Effect.h"

#define CUSTOM_EQ_PARAM_LOUDNESS_CORRECTION 1000

class EffectEqualizer : public Effect {
    private:
    float mBand[8];
    Biquad mFilterL[7], mFilterR[7];

    /* Automatic equalizer */
    float mLoudnessAdjustment;

    float mLoudnessL;
    float mLoudnessR;
    int16_t mPreAmp;
    int32_t mNextUpdate;
    int32_t mNextUpdateInterval;
    int64_t mPowerSquaredL;
    int64_t mPowerSquaredR;

    /* Smooth enable/disable */
    int32_t mFade;

    void setBand(int32_t idx, float dB);
    float getAdjustedBand(int32_t idx, float loudness);
    void refreshBands();
    void updateLoudnessEstimate(float& loudness, int64_t powerSquared);
    protected:
    uint8_t mPreviousRandom;

    /* High-passed triangular probability density function.
     * Output varies from -0xff to 0xff. */
    inline int32_t triangularDither8() {
        uint8_t newRandom = prng();
        int32_t rnd = int32_t(mPreviousRandom) - int32_t(newRandom);
        mPreviousRandom = newRandom;
        return rnd;
    }

    public:
    EffectEqualizer();
    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
