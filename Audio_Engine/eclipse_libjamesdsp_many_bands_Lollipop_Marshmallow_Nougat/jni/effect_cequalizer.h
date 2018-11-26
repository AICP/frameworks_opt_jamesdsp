//Do NOT modify this file!!!
#ifndef ANDROID_EFFECT_EQUALIZER_H_
#define ANDROID_EFFECT_EQUALIZER_H_

#include <hardware/audio_effect.h>

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_EQUALIZER_ = { 0x0bed4300, 0xddd6, 0x11db, 0x8f34,
        { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };
const effect_uuid_t * const SL_IID_EQUALIZER = &SL_IID_EQUALIZER_;
#endif //OPENSL_ES_H_

#if __cplusplus
extern "C" {
#endif

/* enumerated parameters for Equalizer effect */
typedef enum
{
    EQ_PARAM_NUM_BANDS,             // Gets the number of frequency bands that the equalizer
                                    // supports.
    EQ_PARAM_LEVEL_RANGE,           // Returns the minimum and maximum band levels supported.
    EQ_PARAM_BAND_LEVEL,            // Gets/Sets the gain set for the given equalizer band.
    EQ_PARAM_CENTER_FREQ,           // Gets the center frequency of the given band.
    EQ_PARAM_BAND_FREQ_RANGE,       // Gets the frequency range of the given frequency band.
    EQ_PARAM_GET_BAND,              // Gets the band that has the most effect on the given
                                    // frequency.
    EQ_PARAM_CUR_PRESET,            // Gets/Sets the current preset.
    EQ_PARAM_GET_NUM_OF_PRESETS,    // Gets the total number of presets the equalizer supports.
    EQ_PARAM_GET_PRESET_NAME,       // Gets the preset name based on the index.
    EQ_PARAM_PROPERTIES,             // Gets/Sets all parameters at a time.
    EQ_PARAM_PREAMP_STRENGTH
} t_equalizer_params;

//t_equalizer_settings groups all current equalizer setting for backup and restore.
typedef struct s_equalizer_settings {
    uint16_t curPreset;
    uint16_t numBands;
    uint16_t bandLevels[];
} t_equalizer_settings;

#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECT_EQUALIZER_H_*/
