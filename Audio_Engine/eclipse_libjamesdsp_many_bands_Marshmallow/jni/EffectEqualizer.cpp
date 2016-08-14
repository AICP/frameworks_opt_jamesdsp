#define TAG "Equalizer16Band"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#include "EffectEqualizer.h"

//#include <math.h>

#define NUM_BANDS 14

/*      EQ presets      */
static int16_t gPresetAcoustic[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetBassBooster[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetBassReducer[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetClassical[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetDeep[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetFlat[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetRnB[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetRock[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetSmallSpeakers[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetTrebleBooster[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetTrebleReducer[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetVocalBooster[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

struct sPresetConfig {
    const char * name;
    const int16_t * bandConfigs;
};

static const sPresetConfig gEqualizerPresets[] = {
    { "Acoustic",       gPresetAcoustic      },
    { "Bass Booster",   gPresetBassBooster   },
    { "Bass Reducer",   gPresetBassReducer   },
    { "Classical",      gPresetClassical     },
    { "Deep",           gPresetDeep          },
    { "Flat",           gPresetFlat          },
    { "R&B",            gPresetRnB           },
    { "Rock",           gPresetRock          },
    { "Small Speakers", gPresetSmallSpeakers },
    { "Treble Booster", gPresetTrebleBooster },
    { "Treble Reducer", gPresetTrebleReducer },
    { "Vocal Booster",  gPresetVocalBooster  }
};

static const int16_t gNumPresets = sizeof(gEqualizerPresets)/sizeof(sPresetConfig);
static int16_t mCurPreset = 0;

/*      End of EQ presets      */

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data;
} reply1x4_1x2_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data1;
    int16_t data2;
} reply1x4_2x2_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t arg;
    int16_t data;
} reply2x4_1x2_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t arg;
    int32_t data;
} reply2x4_1x4_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t arg;
    int32_t data1;
    int32_t data2;
} reply2x4_2x4_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data[16]; // numbands (16) + 2
} reply1x4_props_t;

EffectEqualizer::EffectEqualizer()
    : mFade(0)
{
    for (int32_t i = 0; i < 14; i++) {
        mBand[i] = 0;
    }
}

int32_t EffectEqualizer::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
    if (cmdCode == EFFECT_CMD_SET_CONFIG) {
        int32_t ret = Effect::configure(pCmdData);
        if (ret != 0) {
            int32_t *replyData = (int32_t *) pReplyData;
            *replyData = ret;
            return 0;
        }

        int32_t *replyData = (int32_t *) pReplyData;
        *replyData = 0;
        return 0;
    }

    if (cmdCode == EFFECT_CMD_GET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == EQ_PARAM_NUM_BANDS) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = NUM_BANDS;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_LEVEL_RANGE) {
                reply1x4_2x2_t *replyData = (reply1x4_2x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data1 = -2200;
                replyData->data2 = 2200;
                *replySize = sizeof(reply1x4_2x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_GET_NUM_OF_PRESETS) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = gNumPresets;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_PROPERTIES) {
                reply1x4_props_t *replyData = (reply1x4_props_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2*8;
                replyData->data[0] = (int16_t)-1; // PRESET_CUSTOM
                replyData->data[1] = (int16_t)14;  // number of bands
                for (int i = 0; i < NUM_BANDS; i++) {
                    replyData->data[2 + i] = (int16_t)(mBand[i] * 120 + 0.5f); // band levels
                }
                *replySize = sizeof(reply1x4_props_t);
                return 0;
            }
            if (cmd == EQ_PARAM_PREAMP_STRENGTH) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mPreAmp;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_CUR_PRESET) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mCurPreset;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
        } else if (cep->psize == 8) {
            int32_t cmd = ((int32_t *) cep)[3];
            int32_t arg = ((int32_t *) cep)[4];
            if (cmd == EQ_PARAM_BAND_LEVEL && arg >= 0 && arg < NUM_BANDS) {
                reply2x4_1x2_t *replyData = (reply2x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = int16_t(mBand[arg] * 120 + 0.5f);
                *replySize = sizeof(reply2x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_CENTER_FREQ && arg >= 0 && arg < NUM_BANDS) {
                if(arg == 0)
                {
                float centerFrequency = 32.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 1)
                {
                float centerFrequency = 64.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 2)
                {
                float centerFrequency = 126.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 3)
                {
                float centerFrequency = 200.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 4)
                {
                float centerFrequency = 317.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 5)
                {
                float centerFrequency = 502.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 6)
                {
                float centerFrequency = 796.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 7)
                {
                float centerFrequency = 1260.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 8)
                {
                float centerFrequency = 2000.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 9)
                {
                float centerFrequency = 3170.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
				else if(arg == 10)
                {
                float centerFrequency = 5020.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 11)
                {
                float centerFrequency = 7960.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 12)
                {
                float centerFrequency = 13500.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
				else if(arg == 13)
                {
                float centerFrequency = 18500.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else
                {
                // Should not happen
                }
                return 0;
            }
            if (cmd == EQ_PARAM_GET_BAND) {
		int16_t band = 14;
                reply2x4_1x2_t *replyData = (reply2x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = int16_t(band);
                *replySize = sizeof(reply2x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_GET_PRESET_NAME && arg >= 0 && arg < int32_t(gNumPresets)) {
                effect_param_t *replyData = (effect_param_t *)pReplyData;
                memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + cep->psize);
                uint32_t *pValueSize = &replyData->vsize;
                int voffset = ((replyData->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
                void *pValue = replyData->data + voffset;

                char *name = (char *)pValue;
                strncpy(name, gEqualizerPresets[arg].name, *pValueSize - 1);
                name[*pValueSize - 1] = 0;
                *pValueSize = strlen(name) + 1;
                *replySize = sizeof(effect_param_t) + voffset + replyData->vsize;
                replyData->status = 0;
                return 0;
            }
        }

        effect_param_t *replyData = (effect_param_t *) pReplyData;
        replyData->status = -EINVAL;
        replyData->vsize = 0;
        *replySize = sizeof(effect_param_t);
        return 0;
    }

    if (cmdCode == EFFECT_CMD_SET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        int32_t *replyData = (int32_t *) pReplyData;

        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == CUSTOM_EQ_PARAM_LOUDNESS_CORRECTION) {
                int16_t value = ((int16_t *) cep)[8];
//                mLoudnessAdjustment = value / 100.0f;
                *replyData = 0;
                return 0;
            }
        }

        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = *((int32_t *) cep->data);
            int16_t arg = *((int16_t *) (cep->data + sizeof(int32_t)));

            if (cmd == EQ_PARAM_CUR_PRESET && arg >= 0 && arg < gNumPresets) {
                    sizeof(const int16_t *);
                int16_t i = 0;
                for (i = 0; i < NUM_BANDS; i++) {
                    mBand[i] = gEqualizerPresets[arg].bandConfigs[i] / 100.0f;
                }
                mCurPreset = arg;
                *replyData = 0;
                refreshBands();
                return 0;
            } else {
            }
        }

        if (cep->psize == 8 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            int32_t arg = ((int32_t *) cep)[4];

            if (cmd == EQ_PARAM_BAND_LEVEL && arg >= 0 && arg < NUM_BANDS) {
                *replyData = 0;
                int16_t value = ((int16_t *) cep)[10];
                mBand[arg] = value / 100.0f;
//                //LOGI("Band number: %d, gain value:%f",arg+1,mBand[arg]);
                refreshBands();
                return 0;
            }
        }

        if (cep->psize == 4 && cep->vsize >= 4 && cep->vsize <= 16) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == EQ_PARAM_PROPERTIES) {
                *replyData = 0;
                if ((((int16_t *) cep)[8]) >= 0) {
                    *replyData = -EINVAL;
                    return 0;
                }
                if ((((int16_t *) cep)[9]) != NUM_BANDS) {
                    *replyData = -EINVAL;
                    return 0;
                }
                for (int i = 0; i < NUM_BANDS; i++) {
                    mBand[i] = ((int16_t *) cep)[10 + i] / 100.0f;
                }

                return 0;
            }
        }
        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == EQ_PARAM_PREAMP_STRENGTH) {
                mPreAmp = ((int16_t *) cep)[8];
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
        }
        *replyData = -EINVAL;
        return 0;
    }

    return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}

void EffectEqualizer::refreshBands()
{
            Iir::Butterworth::LowShelf<8> ls;
            ls.setup (8, mSamplingRate, 32.0, (mBand[0]*1.8));
            mSOS1Band1L.setSOS(0, ls[0].getA0(), ls[0].getA1(), ls[0].getA2(), ls[0].getB0(), ls[0].getB1(), ls[0].getB2());
            mSOS1Band1R.setSOS(0, ls[0].getA0(), ls[0].getA1(), ls[0].getA2(), ls[0].getB0(), ls[0].getB1(), ls[0].getB2());
            mSOS2Band1L.setSOS(0, ls[1].getA0(), ls[1].getA1(), ls[1].getA2(), ls[1].getB0(), ls[1].getB1(), ls[1].getB2());
            mSOS2Band1R.setSOS(0, ls[1].getA0(), ls[1].getA1(), ls[1].getA2(), ls[1].getB0(), ls[1].getB1(), ls[1].getB2());
            mSOS2Band1L.setSOS(0, ls[2].getA0(), ls[2].getA1(), ls[2].getA2(), ls[2].getB0(), ls[2].getB1(), ls[2].getB2());
            mSOS2Band1R.setSOS(0, ls[2].getA0(), ls[2].getA1(), ls[2].getA2(), ls[2].getB0(), ls[2].getB1(), ls[2].getB2());
            mSOS3Band1L.setSOS(0, ls[3].getA0(), ls[3].getA1(), ls[3].getA2(), ls[3].getB0(), ls[3].getB1(), ls[3].getB2());
            mSOS3Band1R.setSOS(0, ls[3].getA0(), ls[3].getA1(), ls[3].getA2(), ls[3].getB0(), ls[3].getB1(), ls[3].getB2());
            Iir::Butterworth::BandShelf<6> bs1;
            bs1.setup (6, mSamplingRate, 64.0f, 34.0, (mBand[1]*1.5));
            mSOS1Band2L.setSOS(0, bs1[0].getA0(), bs1[0].getA1(), bs1[0].getA2(), bs1[0].getB0(), bs1[0].getB1(), bs1[0].getB2());
            mSOS1Band2R.setSOS(0, bs1[0].getA0(), bs1[0].getA1(), bs1[0].getA2(), bs1[0].getB0(), bs1[0].getB1(), bs1[0].getB2());
            mSOS2Band2L.setSOS(0, bs1[1].getA0(), bs1[1].getA1(), bs1[1].getA2(), bs1[1].getB0(), bs1[1].getB1(), bs1[1].getB2());
            mSOS2Band2R.setSOS(0, bs1[1].getA0(), bs1[1].getA1(), bs1[1].getA2(), bs1[1].getB0(), bs1[1].getB1(), bs1[1].getB2());
            mSOS3Band3L.setSOS(0, bs1[2].getA0(), bs1[2].getA1(), bs1[2].getA2(), bs1[2].getB0(), bs1[2].getB1(), bs1[2].getB2());
            mSOS3Band3R.setSOS(0, bs1[2].getA0(), bs1[2].getA1(), bs1[2].getA2(), bs1[2].getB0(), bs1[2].getB1(), bs1[2].getB2());
            Iir::Butterworth::BandShelf<6> bs2;
            bs2.setup (6, mSamplingRate, 126.0f, 50.0, (mBand[2]*1.45));
            mSOS1Band3L.setSOS(0, bs2[0].getA0(), bs2[0].getA1(), bs2[0].getA2(), bs2[0].getB0(), bs2[0].getB1(), bs2[0].getB2());
            mSOS1Band3R.setSOS(0, bs2[0].getA0(), bs2[0].getA1(), bs2[0].getA2(), bs2[0].getB0(), bs2[0].getB1(), bs2[0].getB2());
            mSOS2Band3L.setSOS(0, bs2[1].getA0(), bs2[1].getA1(), bs2[1].getA2(), bs2[1].getB0(), bs2[1].getB1(), bs2[1].getB2());
            mSOS2Band3R.setSOS(0, bs2[1].getA0(), bs2[1].getA1(), bs2[1].getA2(), bs2[1].getB0(), bs2[1].getB1(), bs2[1].getB2());
            mSOS3Band3L.setSOS(0, bs2[2].getA0(), bs2[2].getA1(), bs2[2].getA2(), bs2[2].getB0(), bs2[2].getB1(), bs2[2].getB2());
            mSOS3Band3R.setSOS(0, bs2[2].getA0(), bs2[2].getA1(), bs2[2].getA2(), bs2[2].getB0(), bs2[2].getB1(), bs2[2].getB2());
            Iir::Butterworth::BandShelf<6> bs3;
            bs3.setup (6, mSamplingRate, 200.0f, 74.0, (mBand[3]*1.4));
            mSOS1Band4L.setSOS(0, bs3[0].getA0(), bs3[0].getA1(), bs3[0].getA2(), bs3[0].getB0(), bs3[0].getB1(), bs3[0].getB2());
            mSOS1Band4R.setSOS(0, bs3[0].getA0(), bs3[0].getA1(), bs3[0].getA2(), bs3[0].getB0(), bs3[0].getB1(), bs3[0].getB2());
            mSOS2Band4L.setSOS(0, bs3[1].getA0(), bs3[1].getA1(), bs3[1].getA2(), bs3[1].getB0(), bs3[1].getB1(), bs3[1].getB2());
            mSOS2Band4R.setSOS(0, bs3[1].getA0(), bs3[1].getA1(), bs3[1].getA2(), bs3[1].getB0(), bs3[1].getB1(), bs3[1].getB2());
            mSOS3Band4L.setSOS(0, bs3[2].getA0(), bs3[2].getA1(), bs3[2].getA2(), bs3[2].getB0(), bs3[2].getB1(), bs3[2].getB2());
            mSOS3Band4R.setSOS(0, bs3[2].getA0(), bs3[2].getA1(), bs3[2].getA2(), bs3[2].getB0(), bs3[2].getB1(), bs3[2].getB2());
            Iir::Butterworth::BandShelf<6> bs4;
            bs4.setup (6, mSamplingRate, 317.0f, 110.0, (mBand[4]*1.3));
            mSOS1Band5L.setSOS(0, bs4[0].getA0(), bs4[0].getA1(), bs4[0].getA2(), bs4[0].getB0(), bs4[0].getB1(), bs4[0].getB2());
            mSOS1Band5R.setSOS(0, bs4[0].getA0(), bs4[0].getA1(), bs4[0].getA2(), bs4[0].getB0(), bs4[0].getB1(), bs4[0].getB2());
            mSOS2Band5L.setSOS(0, bs4[1].getA0(), bs4[1].getA1(), bs4[1].getA2(), bs4[1].getB0(), bs4[1].getB1(), bs4[1].getB2());
            mSOS2Band5R.setSOS(0, bs4[1].getA0(), bs4[1].getA1(), bs4[1].getA2(), bs4[1].getB0(), bs4[1].getB1(), bs4[1].getB2());
            mSOS3Band5L.setSOS(0, bs4[2].getA0(), bs4[2].getA1(), bs4[2].getA2(), bs4[2].getB0(), bs4[2].getB1(), bs4[2].getB2());
            mSOS3Band5R.setSOS(0, bs4[2].getA0(), bs4[2].getA1(), bs4[2].getA2(), bs4[2].getB0(), bs4[2].getB1(), bs4[2].getB2());
            Iir::Butterworth::BandShelf<6> bs5;
            bs5.setup (6, mSamplingRate, 502.0f, 180.0, (mBand[5]*1.2));
            mSOS1Band6L.setSOS(0, bs5[0].getA0(), bs5[0].getA1(), bs5[0].getA2(), bs5[0].getB0(), bs5[0].getB1(), bs5[0].getB2());
            mSOS1Band6R.setSOS(0, bs5[0].getA0(), bs5[0].getA1(), bs5[0].getA2(), bs5[0].getB0(), bs5[0].getB1(), bs5[0].getB2());
            mSOS2Band6L.setSOS(0, bs5[1].getA0(), bs5[1].getA1(), bs5[1].getA2(), bs5[1].getB0(), bs5[1].getB1(), bs5[1].getB2());
            mSOS2Band6R.setSOS(0, bs5[1].getA0(), bs5[1].getA1(), bs5[1].getA2(), bs5[1].getB0(), bs5[1].getB1(), bs5[1].getB2());
            mSOS3Band6L.setSOS(0, bs5[2].getA0(), bs5[2].getA1(), bs5[2].getA2(), bs5[2].getB0(), bs5[2].getB1(), bs5[2].getB2());
            mSOS3Band6R.setSOS(0, bs5[2].getA0(), bs5[2].getA1(), bs5[2].getA2(), bs5[2].getB0(), bs5[2].getB1(), bs5[2].getB2());
            Iir::Butterworth::BandShelf<6> bs6;
            bs6.setup (6, mSamplingRate, 796.0f, 260.0, mBand[6]);
            mSOS1Band7L.setSOS(0, bs6[0].getA0(), bs6[0].getA1(), bs6[0].getA2(), bs6[0].getB0(), bs6[0].getB1(), bs6[0].getB2());
            mSOS1Band7R.setSOS(0, bs6[0].getA0(), bs6[0].getA1(), bs6[0].getA2(), bs6[0].getB0(), bs6[0].getB1(), bs6[0].getB2());
            mSOS2Band7L.setSOS(0, bs6[1].getA0(), bs6[1].getA1(), bs6[1].getA2(), bs6[1].getB0(), bs6[1].getB1(), bs6[1].getB2());
            mSOS2Band7R.setSOS(0, bs6[1].getA0(), bs6[1].getA1(), bs6[1].getA2(), bs6[1].getB0(), bs6[1].getB1(), bs6[1].getB2());
            mSOS3Band7L.setSOS(0, bs6[2].getA0(), bs6[2].getA1(), bs6[2].getA2(), bs6[2].getB0(), bs6[2].getB1(), bs6[2].getB2());
            mSOS3Band7R.setSOS(0, bs6[2].getA0(), bs6[2].getA1(), bs6[2].getA2(), bs6[2].getB0(), bs6[2].getB1(), bs6[2].getB2());
            Iir::Butterworth::BandShelf<6> bs7;
            bs7.setup (6, mSamplingRate, 1200.0f, 450.0, mBand[7]);
            mSOS1Band8L.setSOS(0, bs7[0].getA0(), bs7[0].getA1(), bs7[0].getA2(), bs7[0].getB0(), bs7[0].getB1(), bs7[0].getB2());
            mSOS1Band8R.setSOS(0, bs7[0].getA0(), bs7[0].getA1(), bs7[0].getA2(), bs7[0].getB0(), bs7[0].getB1(), bs7[0].getB2());
            mSOS2Band8L.setSOS(0, bs7[1].getA0(), bs7[1].getA1(), bs7[1].getA2(), bs7[1].getB0(), bs7[1].getB1(), bs7[1].getB2());
            mSOS2Band8R.setSOS(0, bs7[1].getA0(), bs7[1].getA1(), bs7[1].getA2(), bs7[1].getB0(), bs7[1].getB1(), bs7[1].getB2());
            mSOS3Band8L.setSOS(0, bs7[2].getA0(), bs7[2].getA1(), bs7[2].getA2(), bs7[2].getB0(), bs7[2].getB1(), bs7[2].getB2());
            mSOS3Band8R.setSOS(0, bs7[2].getA0(), bs7[2].getA1(), bs7[2].getA2(), bs7[2].getB0(), bs7[2].getB1(), bs7[2].getB2());
            Iir::Butterworth::BandShelf<12> bs8;
            bs8.setup (12, mSamplingRate, 2000.0f, 850.0, mBand[8]);
            mSOS1Band9L.setSOS(0, bs8[0].getA0(), bs8[0].getA1(), bs8[0].getA2(), bs8[0].getB0(), bs8[0].getB1(), bs8[0].getB2());
            mSOS1Band9R.setSOS(0, bs8[0].getA0(), bs8[0].getA1(), bs8[0].getA2(), bs8[0].getB0(), bs8[0].getB1(), bs8[0].getB2());
            mSOS2Band9L.setSOS(0, bs8[1].getA0(), bs8[1].getA1(), bs8[1].getA2(), bs8[1].getB0(), bs8[1].getB1(), bs8[1].getB2());
            mSOS2Band9R.setSOS(0, bs8[1].getA0(), bs8[1].getA1(), bs8[1].getA2(), bs8[1].getB0(), bs8[1].getB1(), bs8[1].getB2());
            mSOS3Band9L.setSOS(0, bs8[2].getA0(), bs8[2].getA1(), bs8[2].getA2(), bs8[2].getB0(), bs8[2].getB1(), bs8[2].getB2());
            mSOS3Band9R.setSOS(0, bs8[2].getA0(), bs8[2].getA1(), bs8[2].getA2(), bs8[2].getB0(), bs8[2].getB1(), bs8[2].getB2());
            mSOS4Band9L.setSOS(0, bs8[3].getA0(), bs8[3].getA1(), bs8[3].getA2(), bs8[3].getB0(), bs8[3].getB1(), bs8[3].getB2());
            mSOS4Band9R.setSOS(0, bs8[3].getA0(), bs8[3].getA1(), bs8[3].getA2(), bs8[3].getB0(), bs8[3].getB1(), bs8[3].getB2());
            mSOS5Band9L.setSOS(0, bs8[4].getA0(), bs8[4].getA1(), bs8[4].getA2(), bs8[4].getB0(), bs8[4].getB1(), bs8[4].getB2());
            mSOS5Band9R.setSOS(0, bs8[4].getA0(), bs8[4].getA1(), bs8[4].getA2(), bs8[4].getB0(), bs8[4].getB1(), bs8[4].getB2());
            mSOS6Band9L.setSOS(0, bs8[5].getA0(), bs8[5].getA1(), bs8[5].getA2(), bs8[5].getB0(), bs8[5].getB1(), bs8[5].getB2());
            mSOS6Band9R.setSOS(0, bs8[5].getA0(), bs8[5].getA1(), bs8[5].getA2(), bs8[5].getB0(), bs8[5].getB1(), bs8[5].getB2());
            Iir::Butterworth::BandShelf<12> bs9;
            bs9.setup (12, mSamplingRate, 3200.0f, 1450.0, mBand[9]);
            mSOS1Band10L.setSOS(0, bs9[0].getA0(), bs9[0].getA1(), bs9[0].getA2(), bs9[0].getB0(), bs9[0].getB1(), bs9[0].getB2());
            mSOS1Band10R.setSOS(0, bs9[0].getA0(), bs9[0].getA1(), bs9[0].getA2(), bs9[0].getB0(), bs9[0].getB1(), bs9[0].getB2());
            mSOS2Band10L.setSOS(0, bs9[1].getA0(), bs9[1].getA1(), bs9[1].getA2(), bs9[1].getB0(), bs9[1].getB1(), bs9[1].getB2());
            mSOS2Band10R.setSOS(0, bs9[1].getA0(), bs9[1].getA1(), bs9[1].getA2(), bs9[1].getB0(), bs9[1].getB1(), bs9[1].getB2());
            mSOS3Band10L.setSOS(0, bs9[2].getA0(), bs9[2].getA1(), bs9[2].getA2(), bs9[2].getB0(), bs9[2].getB1(), bs9[2].getB2());
            mSOS3Band10R.setSOS(0, bs9[2].getA0(), bs9[2].getA1(), bs9[2].getA2(), bs9[2].getB0(), bs9[2].getB1(), bs9[2].getB2());
            mSOS4Band10L.setSOS(0, bs9[3].getA0(), bs9[3].getA1(), bs9[3].getA2(), bs9[3].getB0(), bs9[3].getB1(), bs9[3].getB2());
            mSOS4Band10R.setSOS(0, bs9[3].getA0(), bs9[3].getA1(), bs9[3].getA2(), bs9[3].getB0(), bs9[3].getB1(), bs9[3].getB2());
            mSOS5Band10L.setSOS(0, bs9[4].getA0(), bs9[4].getA1(), bs9[4].getA2(), bs9[4].getB0(), bs9[4].getB1(), bs9[4].getB2());
            mSOS5Band10R.setSOS(0, bs9[4].getA0(), bs9[4].getA1(), bs9[4].getA2(), bs9[4].getB0(), bs9[4].getB1(), bs9[4].getB2());
            mSOS6Band10L.setSOS(0, bs9[5].getA0(), bs9[5].getA1(), bs9[5].getA2(), bs9[5].getB0(), bs9[5].getB1(), bs9[5].getB2());
            mSOS6Band10R.setSOS(0, bs9[5].getA0(), bs9[5].getA1(), bs9[5].getA2(), bs9[5].getB0(), bs9[5].getB1(), bs9[5].getB2());
            Iir::Butterworth::BandShelf<14> bs10;
            bs10.setup (14, mSamplingRate, 5020.0f, 1800.0, mBand[10]);
            mSOS1Band11L.setSOS(0, bs10[0].getA0(), bs10[0].getA1(), bs10[0].getA2(), bs10[0].getB0(), bs10[0].getB1(), bs10[0].getB2());
            mSOS1Band11R.setSOS(0, bs10[0].getA0(), bs10[0].getA1(), bs10[0].getA2(), bs10[0].getB0(), bs10[0].getB1(), bs10[0].getB2());
            mSOS2Band11L.setSOS(0, bs10[1].getA0(), bs10[1].getA1(), bs10[1].getA2(), bs10[1].getB0(), bs10[1].getB1(), bs10[1].getB2());
            mSOS2Band11R.setSOS(0, bs10[1].getA0(), bs10[1].getA1(), bs10[1].getA2(), bs10[1].getB0(), bs10[1].getB1(), bs10[1].getB2());
            mSOS3Band11L.setSOS(0, bs10[2].getA0(), bs10[2].getA1(), bs10[2].getA2(), bs10[2].getB0(), bs10[2].getB1(), bs10[2].getB2());
            mSOS3Band11R.setSOS(0, bs10[2].getA0(), bs10[2].getA1(), bs10[2].getA2(), bs10[2].getB0(), bs10[2].getB1(), bs10[2].getB2());
            mSOS4Band11L.setSOS(0, bs10[3].getA0(), bs10[3].getA1(), bs10[3].getA2(), bs10[3].getB0(), bs10[3].getB1(), bs10[3].getB2());
            mSOS4Band11R.setSOS(0, bs10[3].getA0(), bs10[3].getA1(), bs10[3].getA2(), bs10[3].getB0(), bs10[3].getB1(), bs10[3].getB2());
            mSOS5Band11L.setSOS(0, bs10[4].getA0(), bs10[4].getA1(), bs10[4].getA2(), bs10[4].getB0(), bs10[4].getB1(), bs10[4].getB2());
            mSOS5Band11R.setSOS(0, bs10[4].getA0(), bs10[4].getA1(), bs10[4].getA2(), bs10[4].getB0(), bs10[4].getB1(), bs10[4].getB2());
            mSOS6Band11L.setSOS(0, bs10[5].getA0(), bs10[5].getA1(), bs10[5].getA2(), bs10[5].getB0(), bs10[5].getB1(), bs10[5].getB2());
            mSOS6Band11R.setSOS(0, bs10[5].getA0(), bs10[5].getA1(), bs10[5].getA2(), bs10[5].getB0(), bs10[5].getB1(), bs10[5].getB2());
            mSOS7Band11L.setSOS(0, bs10[6].getA0(), bs10[6].getA1(), bs10[6].getA2(), bs10[6].getB0(), bs10[6].getB1(), bs10[6].getB2());
            mSOS7Band11R.setSOS(0, bs10[6].getA0(), bs10[6].getA1(), bs10[6].getA2(), bs10[6].getB0(), bs10[6].getB1(), bs10[6].getB2());
            mHSFilter12L.setHighShelf(0, 7960.0f, mSamplingRate, mBand[11]*0.8, 1.0f, 0);
            mHSFilter12R.setHighShelf(0, 7960.0f, mSamplingRate, mBand[11]*0.8, 1.0f, 0);
            float compensate = (mBand[11] > 0) ? mBand[11] * -1: mBand[11];
            mHSFilter13L.setHighShelf(0, 13500.0f, mSamplingRate, (mBand[12]+compensate)*0.8, 1.0f, 0);
            mHSFilter13R.setHighShelf(0, 13500.0f, mSamplingRate, (mBand[12]+compensate)*0.8, 1.0f, 0);
            mHSFilter14L.setHighShelf(0, 17500.0f, mSamplingRate, mBand[13]*0.9, 1.0f, 0);
            mHSFilter14R.setHighShelf(0, 18500.0f, mSamplingRate, mBand[13]*0.9, 1.0f, 0);
}
int32_t EffectEqualizer::process(audio_buffer_t *in, audio_buffer_t *out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
        int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);

        tmpL = dryL * mPreAmp;
        tmpR = dryR * mPreAmp;

        tmpL = mSOS1Band1L.process(tmpL);
        tmpL = mSOS2Band1L.process(tmpL);
        tmpL = mSOS3Band1L.process(tmpL);
        tmpL = mSOS4Band1L.process(tmpL);
        tmpR = mSOS1Band1R.process(tmpR);
        tmpR = mSOS2Band1R.process(tmpR);
        tmpR = mSOS3Band1R.process(tmpR);
        tmpR = mSOS4Band1R.process(tmpR);
        tmpL = mSOS1Band2L.process(tmpL);
        tmpL = mSOS2Band2L.process(tmpL);
        tmpL = mSOS3Band2L.process(tmpL);
        tmpR = mSOS1Band2R.process(tmpR);
        tmpR = mSOS2Band2R.process(tmpR);
        tmpR = mSOS3Band2R.process(tmpR);
        tmpL = mSOS1Band3L.process(tmpL);
        tmpL = mSOS2Band3L.process(tmpL);
        tmpL = mSOS3Band3L.process(tmpL);
        tmpR = mSOS1Band3R.process(tmpR);
        tmpR = mSOS2Band3R.process(tmpR);
        tmpR = mSOS3Band3R.process(tmpR);
        tmpL = mSOS1Band4L.process(tmpL);
        tmpL = mSOS2Band4L.process(tmpL);
        tmpL = mSOS3Band4L.process(tmpL);
        tmpR = mSOS1Band4R.process(tmpR);
        tmpR = mSOS2Band4R.process(tmpR);
        tmpR = mSOS3Band4R.process(tmpR);
        tmpL = mSOS1Band5L.process(tmpL);
        tmpL = mSOS2Band5L.process(tmpL);
        tmpL = mSOS3Band5L.process(tmpL);
        tmpR = mSOS1Band5R.process(tmpR);
        tmpR = mSOS2Band5R.process(tmpR);
        tmpR = mSOS3Band5R.process(tmpR);
        tmpL = mSOS1Band6L.process(tmpL);
        tmpL = mSOS2Band6L.process(tmpL);
        tmpL = mSOS3Band6L.process(tmpL);
        tmpR = mSOS1Band6R.process(tmpR);
        tmpR = mSOS2Band6R.process(tmpR);
        tmpR = mSOS3Band6R.process(tmpR);
        tmpL = mSOS1Band7L.process(tmpL);
        tmpL = mSOS2Band7L.process(tmpL);
        tmpL = mSOS3Band7L.process(tmpL);
        tmpR = mSOS1Band7R.process(tmpR);
        tmpR = mSOS2Band7R.process(tmpR);
        tmpR = mSOS3Band7R.process(tmpR);
        tmpL = mSOS1Band8L.process(tmpL);
        tmpL = mSOS2Band8L.process(tmpL);
        tmpL = mSOS3Band8L.process(tmpL);
        tmpR = mSOS1Band8R.process(tmpR);
        tmpR = mSOS2Band8R.process(tmpR);
        tmpR = mSOS3Band8R.process(tmpR);
        tmpL = mSOS1Band9L.process(tmpL);
        tmpL = mSOS2Band9L.process(tmpL);
        tmpL = mSOS3Band9L.process(tmpL);
        tmpL = mSOS4Band9L.process(tmpL);
        tmpL = mSOS5Band9L.process(tmpL);
        tmpL = mSOS6Band9L.process(tmpL);
        tmpR = mSOS1Band9R.process(tmpR);
        tmpR = mSOS2Band9R.process(tmpR);
        tmpR = mSOS3Band9R.process(tmpR);
        tmpR = mSOS4Band9R.process(tmpR);
        tmpR = mSOS5Band9R.process(tmpR);
        tmpR = mSOS6Band9R.process(tmpR);
        tmpL = mSOS1Band10L.process(tmpL);
        tmpL = mSOS2Band10L.process(tmpL);
        tmpL = mSOS3Band10L.process(tmpL);
        tmpL = mSOS4Band10L.process(tmpL);
        tmpL = mSOS5Band10L.process(tmpL);
        tmpL = mSOS6Band10L.process(tmpL);
        tmpR = mSOS1Band10R.process(tmpR);
        tmpR = mSOS2Band10R.process(tmpR);
        tmpR = mSOS3Band10R.process(tmpR);
        tmpR = mSOS4Band10R.process(tmpR);
        tmpR = mSOS5Band10R.process(tmpR);
        tmpR = mSOS6Band10R.process(tmpR);
        tmpL = mSOS1Band11L.process(tmpL);
        tmpL = mSOS2Band11L.process(tmpL);
        tmpL = mSOS3Band11L.process(tmpL);
        tmpL = mSOS4Band11L.process(tmpL);
        tmpL = mSOS5Band11L.process(tmpL);
        tmpL = mSOS6Band11L.process(tmpL);
        tmpL = mSOS7Band11L.process(tmpL);
        tmpR = mSOS1Band11R.process(tmpR);
        tmpR = mSOS2Band11R.process(tmpR);
        tmpR = mSOS3Band11R.process(tmpR);
        tmpR = mSOS4Band11R.process(tmpR);
        tmpR = mSOS5Band11R.process(tmpR);
        tmpR = mSOS6Band11R.process(tmpR);
        tmpR = mSOS7Band11R.process(tmpR);
        tmpL = mHSFilter12L.process(tmpL);
        tmpR = mHSFilter12R.process(tmpR);
        tmpL = mHSFilter13L.process(tmpL);
        tmpR = mHSFilter13R.process(tmpR);
        tmpL = mHSFilter14L.process(tmpL);
        tmpL = mHSFilter14R.process(tmpL);
        write(out, i * 2, tmpL);
        write(out, i * 2 + 1, tmpR);
    }

    return mEnable || mFade != 0 ? 0 : -ENODATA;
}