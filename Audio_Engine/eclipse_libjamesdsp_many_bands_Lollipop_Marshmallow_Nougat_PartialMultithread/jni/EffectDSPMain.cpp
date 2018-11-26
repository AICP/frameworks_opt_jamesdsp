#ifdef DEBUG
#define TAG "EffectDSPMain"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#include "MemoryUsage.h"
#endif
#include <unistd.h>
#include "EffectDSPMain.h"
typedef struct
{
	int32_t status;
	uint32_t psize;
	uint32_t vsize;
	int32_t cmd;
	int32_t data;
} reply1x4_1x4_t;

EffectDSPMain::EffectDSPMain()
	: equalizerEnabled(0), ramp(1.0f), pregain(12.0f), threshold(-60.0f), knee(30.0f), ratio(12.0f), attack(0.001f), release(0.24f), inputBuffer(0), isBenchData(0), mPreset(0), mReverbMode(1), roomSize(50.0f), reverbEnabled(0), threadResult(0)
	, fxreTime(0.5f), damping(0.5f), inBandwidth(0.8f), earlyLv(0.5f), tailLv(0.5f), verbL(0), mMatrixMCoeff(1.0), mMatrixSCoeff(1.0), bassBoostLp(0), FIREq(0), convolver(0), fullStereoConvolver(0)//, compressor670(0)
	, tempImpulseIncoming(0), tempImpulseFloat(0), finalImpulse(0), convolverReady(-1), bassLpReady(-1), analogModelEnable(0), tubedrive(2.0f), finalGain(1.0f), eqFilterType(0), arbEq(0), xaxis(0), yaxis(0), eqFIRReady(0)
{
	double c0[15] = { 2.138018534150542e-5, 4.0608501987194246e-5, 7.950414700590711e-5, 1.4049065318523225e-4, 2.988065284903209e-4, 0.0013061668170781858, 0.0036204239724680425, 0.008959629624060151, 0.027083658741258742, 0.08156916666666666, 0.1978822177777778, 0.4410733777777778, 1.0418565333333332, 2.1131378, 4.6 };
	double c1[15] = { 5.88199398839289e-6, 1.1786813951189911e-5, 2.5600214528512222e-5, 8.53041086120132e-5, 2.656291374239004e-4, 5.047717001008378e-4, 8.214255850540808e-4, 0.0016754651127819551, 0.0033478867132867136, 0.006705333333333334, 0.013496382222222221, 0.02673028888888889, 0.054979466666666664, 0.11634819999999998, 0.35878 };
	if (ACFFTWThreadInit())
		threadResult = 2;
	else
		threadResult = 0;
	benchmarkValue[0] = (double*)malloc(15 * sizeof(double));
	benchmarkValue[1] = (double*)malloc(15 * sizeof(double));
	memcpy(benchmarkValue[0], c0, sizeof(c0));
	memcpy(benchmarkValue[1], c1, sizeof(c1));
}
EffectDSPMain::~EffectDSPMain()
{
	unsigned int i;
	if (inputBuffer)
	{
		free(inputBuffer[0]);
		free(inputBuffer[1]);
		free(inputBuffer);
		free(outputBuffer[0]);
		free(outputBuffer[1]);
		free(outputBuffer);
	}
	if (verbL)
	{
		gverb_free(verbL);
		gverb_free(verbR);
	}
	FreeBassBoost();
	FreeEq();
	FreeConvolver();
	if (finalImpulse)
	{
		free(finalImpulse[0]);
		free(finalImpulse[1]);
		free(finalImpulse);
	}
	if (tempImpulseIncoming)
		free(tempImpulseIncoming);
	if (tempImpulseFloat)
		free(tempImpulseFloat);
	if (threadResult)
		ACFFTWClean(1);
	else
		ACFFTWClean(0);
	if (benchmarkValue[0])
		free(benchmarkValue[0]);
	if (benchmarkValue[1])
		free(benchmarkValue[1]);
#ifdef DEBUG
	LOGI("Buffer freed");
#endif
}
void EffectDSPMain::FreeBassBoost()
{
	if (bassBoostLp)
	{
		for (unsigned int i = 0; i < NUMCHANNEL; i++)
		{
			AutoConvolverMonoFree(bassBoostLp[i]);
			free(bassBoostLp[i]);
		}
		free(bassBoostLp);
		bassBoostLp = 0;
		ramp = 0.4f;
	}
}
void EffectDSPMain::FreeEq()
{
	if (xaxis)
	{
		free(xaxis);
		xaxis = 0;
	}
	if (yaxis)
	{
		free(yaxis);
		yaxis = 0;
	}
	if (arbEq)
	{
		ArbitraryEqFree(arbEq);
		free(arbEq);
		arbEq = 0;
	}
	if (FIREq)
	{
		for (unsigned int i = 0; i < NUMCHANNEL; i++)
		{
			AutoConvolverMonoFree(FIREq[i]);
			free(FIREq[i]);
		}
		free(FIREq);
		FIREq = 0;
	}
}
void EffectDSPMain::FreeConvolver()
{
	int i;
	if (convolver)
	{
		for (i = 0; i < 2; i++)
		{
			AutoConvolverMonoFree(convolver[i]);
			free(convolver[i]);
		}
		free(convolver);
		convolver = 0;
	}
	if (fullStereoConvolver)
	{
		for (i = 0; i < 4; i++)
		{
			AutoConvolverMonoFree(fullStereoConvolver[i]);
			free(fullStereoConvolver[i]);
		}
		free(fullStereoConvolver);
		fullStereoConvolver = 0;
	}
	if (fullStereoBuf)
	{
		free(fullStereoBuf[0]);
		free(fullStereoBuf[1]);
		free(fullStereoBuf);
		fullStereoBuf = 0;
	}
}
int32_t EffectDSPMain::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
#ifdef DEBUG
		LOGI("Memory used: %lf Mb", (double)getCurrentRSS() / 1024.0 / 1024.0);
#endif
	if (cmdCode == EFFECT_CMD_SET_CONFIG)
	{
		size_t frameCountInit;
		effect_buffer_access_e mAccessMode;
		int32_t *replyData = (int32_t *)pReplyData;
		int32_t ret = Effect::configure(pCmdData, &frameCountInit, &mAccessMode);
		if (ret != 0)
		{
			*replyData = ret;
			return 0;
		}
		memSize = frameCountInit * sizeof(float);
		if (!inputBuffer)
		{
			currentframeCountInit = frameCountInit;
			inputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			inputBuffer[0] = (float*)malloc(memSize);
			inputBuffer[1] = (float*)malloc(memSize);
			outputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			outputBuffer[0] = (float*)malloc(memSize);
			outputBuffer[1] = (float*)malloc(memSize);
#ifdef DEBUG
			LOGI("%d space allocated", frameCountInit);
#endif
		}
		if (frameCountInit != currentframeCountInit)
		{
			if (inputBuffer)
			{
				free(inputBuffer[0]);
				free(inputBuffer[1]);
				free(inputBuffer);
				free(outputBuffer[0]);
				free(outputBuffer[1]);
				free(outputBuffer);
			}
			inputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			inputBuffer[0] = (float*)malloc(memSize);
			inputBuffer[1] = (float*)malloc(memSize);
			outputBuffer = (float**)malloc(sizeof(float*) * NUMCHANNEL);
			outputBuffer[0] = (float*)malloc(memSize);
			outputBuffer[1] = (float*)malloc(memSize);
			currentframeCountInit = frameCountInit;
#ifdef DEBUG
			LOGI("%d space reallocated", frameCountInit);
#endif
		}
		fullStconvparams.in = inputBuffer;
		fullStconvparams.frameCount = frameCountInit;
		fullStconvparams1.in = inputBuffer;
		fullStconvparams1.frameCount = frameCountInit;
		rightparams2.in = inputBuffer;
		rightparams2.frameCount = frameCountInit;
		*replyData = 0;
		return 0;
	}
	if (cmdCode == EFFECT_CMD_GET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
		int32_t *replyData = (int32_t *)pReplyData;
		if (cep->psize == 4 && cep->vsize == 4)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 20000)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20000;
				replyData->data = (int32_t)mSamplingRate;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20001)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20001;
				if (!convolver)
					replyData->data = (int32_t)1;
				else
					replyData->data = (int32_t)0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20002)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20002;
				replyData->data = (int32_t)getpid();
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20003)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20003;
				if (isBenchData == 2)
					replyData->data = isBenchData;
				else
					replyData->data = 0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
			else if (cmd == 20004)
			{
				reply1x4_1x4_t *replyData = (reply1x4_1x4_t *)pReplyData;
				replyData->status = 0;
				replyData->psize = 4;
				replyData->vsize = 4;
				replyData->cmd = 20003;
				if (convolver)
					replyData->data = convolver[0]->methods;
				else if (fullStereoConvolver)
					replyData->data = fullStereoConvolver[0]->methods;
				else
					replyData->data = 0;
				*replySize = sizeof(reply1x4_1x4_t);
				return 0;
			}
		}
	}
	if (cmdCode == EFFECT_CMD_SET_PARAM)
	{
		effect_param_t *cep = (effect_param_t *)pCmdData;
		int32_t *replyData = (int32_t *)pReplyData;
		if (cep->psize == 4 && cep->vsize == 2)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 100)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = pregain;
				pregain = (float)value;
				if (oldVal != pregain && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 101)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = threshold;
				threshold = (float)-value;
				if (oldVal != threshold && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 102)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = knee;
				knee = (float)value;
				if (oldVal != knee && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 103)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = ratio;
				ratio = (float)value;
				if (oldVal != ratio && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 104)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = attack;
				attack = value / 1000.0f;
				if (oldVal != attack && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 105)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = release;
				release = value / 1000.0f;
				if (oldVal != release && compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 112)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = bassBoostStrength;
				bassBoostStrength = value;
				if (oldVal != bassBoostStrength)
					bassLpReady = 0;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 113)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = bassBoostFilterType;
				bassBoostFilterType = value;
				if (oldVal != bassBoostFilterType)
				{
					FreeBassBoost();
					bassLpReady = 0;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 114)
			{
				int16_t value = ((int16_t *)cep)[8];
				float bassBoostCentreFreq;
				float oldVal = bassBoostCentreFreq;
				if (bassBoostCentreFreq < 55.0f)
					bassBoostCentreFreq = 55.0f;
				bassBoostCentreFreq = (float)value;
				if ((oldVal != bassBoostCentreFreq) || !bassLpReady)
				{
					bassLpReady = 0;
					if (!bassBoostFilterType)
						refreshBassLinearPhase(currentframeCountInit, 2048, bassBoostCentreFreq);
					else
						refreshBassLinearPhase(currentframeCountInit, 4096, bassBoostCentreFreq);
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 127)
			{
				int16_t oldVal = mReverbMode;
				mReverbMode = ((int16_t *)cep)[8];
				if (oldVal != mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 128)
			{
				int16_t oldVal = mPreset;
				mPreset = ((int16_t *)cep)[8];
				if (oldVal != mPreset && !mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 129)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = roomSize;
				roomSize = (float)value;
				if (oldVal != roomSize && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 130)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = fxreTime;
				fxreTime = value / 100.0f;
				if (oldVal != fxreTime && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 131)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = damping;
				damping = value / 100.0f;
				if (oldVal != damping && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 133)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = inBandwidth;
				inBandwidth = value / 100.0f;
				if (oldVal != inBandwidth && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 134)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = earlyLv;
				earlyLv = value / 100.0f;
				if (oldVal != earlyLv && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 135)
			{
				int16_t value = ((int16_t *)cep)[8];
				float oldVal = tailLv;
				tailLv = value / 100.0f;
				if (oldVal != tailLv && mReverbMode)
					refreshReverb();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 137)
			{
				int16_t value = ((int16_t *)cep)[8];
				refreshStereoWiden(value);
				*replyData = 0;
				return 0;
			}
			else if (cmd == 188)
			{
				int16_t value = ((int16_t *)cep)[8];
				if (bs2bEnabled == 2)
				{
					if (value == 0)
						BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_JMEIER_CLEVEL);
					else if (value == 1)
						BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_CMOY_CLEVEL);
					else if (value == 2)
						BS2BInit(&bs2b, (unsigned int)mSamplingRate, BS2B_DEFAULT_CLEVEL);
					bs2bEnabled = 1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 150)
			{
				float oldVal = tubedrive;
				tubedrive = ((int16_t *)cep)[8] / 1000.0f;
				if (analogModelEnable && oldVal != tubedrive)
					refreshTubeAmp();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 154)
			{
				int16_t oldVal = eqFilterType;
				int16_t value = ((int16_t *)cep)[8];
				if (oldVal != value)
				{
					eqFilterType = value;
					eqFIRReady = 2;
#ifdef DEBUG
					LOGI("EQ filter type: %d", eqFilterType);
#endif
					FreeEq();
#ifdef DEBUG
					LOGI("FIR EQ reseted caused by filter type change");
#endif
				}
				*replyData = 0;
				return 0;
			}
			/*			else if (cmd == 808)
			{
			float oldVal = tubedrive;
			tubedrive = ((int16_t *)cep)[8] / 1000.0f;
			if(wavechild670Enabled == -1)
			{
			Real inputLevelA = tubedrive;
			Real ACThresholdA = 0.35; // This require 0 < ACThresholdA < 1.0
			uint timeConstantSelectA = 1; // Integer from 1-6
			Real DCThresholdA = 0.35; // This require 0 < DCThresholdA < 1.0
			Real outputGain = 1.0 / inputLevelA;
			int sidechainLink = 0;
			int isMidSide = 0;
			int useFeedbackTopology = 1;
			Real inputLevelB = inputLevelA;
			Real ACThresholdB = ACThresholdA;
			uint timeConstantSelectB = timeConstantSelectA;
			Real DCThresholdB = DCThresholdA;
			Wavechild670Parameters params = Wavechild670ParametersInit(inputLevelA, ACThresholdA, timeConstantSelectA, DCThresholdA,
			inputLevelB, ACThresholdB, timeConstantSelectB, DCThresholdB,
			sidechainLink, isMidSide, useFeedbackTopology, outputGain);
			if (compressor670)
			free(compressor670);
			compressor670 = Wavechild670Init((Real)mSamplingRate, &params);
			Wavechild670WarmUp(compressor670, 0.5);
			wavechild670Enabled = 1;
			#ifdef DEBUG
			LOGI("Compressor670 Initialised");
			#endif
			}
			*replyData = 0;
			return 0;
			}*/
			else if (cmd == 1200)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = compressionEnabled;
				compressionEnabled = value;
				if (oldVal != compressionEnabled)
					refreshCompressor();
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1201)
			{
				int16_t oldVal = bassBoostEnabled;
				bassBoostEnabled = ((int16_t *)cep)[8];
				if (!bassBoostEnabled && (oldVal != bassBoostEnabled))
				{
					FreeBassBoost();
					bassLpReady = 0;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1202)
			{
				int16_t oldVal = equalizerEnabled;
				equalizerEnabled = ((int16_t *)cep)[8];
				if ((equalizerEnabled == 1 && (oldVal != equalizerEnabled)) || eqFIRReady == 2)
				{
					const float interpFreq[15] = { 25.0f, 40.0f, 63.0f, 100.0f, 160.0f, 250.0f, 400.0f, 630.0f, 1000.0f, 1600.0f, 2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f };
					xaxis = (float*)malloc(1024 * sizeof(float));
					yaxis = (float*)malloc(1024 * sizeof(float));
					linspace(xaxis, 1024, interpFreq[0], interpFreq[NUM_BANDSM1]);
					arbEq = (ArbitraryEq*)malloc(sizeof(ArbitraryEq));
					eqfilterLength = 8192;
					InitArbitraryEq(arbEq, &eqfilterLength, eqFilterType);
					for (int i = 0; i < 1024; i++)
						ArbitraryEqInsertNode(arbEq, xaxis[i], 0.0f, 0);
#ifdef DEBUG
					LOGI("FIR EQ Initialised");
#endif
				}
				else if (!equalizerEnabled && (oldVal != equalizerEnabled))
				{
					FreeEq();
					eqFIRReady = 0;
#ifdef DEBUG
					LOGI("FIR EQ destroyed");
#endif
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1203)
			{
				int16_t value = ((int16_t *)cep)[8];
				int16_t oldVal = reverbEnabled;
				reverbEnabled = value;
				if (oldVal != reverbEnabled)
					refreshReverb();
				if (verbL && !reverbEnabled)
				{
					gverb_free(verbL);
					verbL = 0;
					gverb_free(verbR);
					verbR = 0;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1204)
			{
				stereoWidenEnabled = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1208)
			{
				int16_t val = ((int16_t *)cep)[8];
				if (val)
					bs2bEnabled = 2;
				else
					bs2bEnabled = 0;
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1205)
			{
				convolverEnabled = ((int16_t *)cep)[8];
				if (!convolverEnabled)
				{
					FreeConvolver();
					convolverReady = -1;
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1206)
			{
				int16_t oldVal = analogModelEnable;
				analogModelEnable = ((int16_t *)cep)[8];
				if (analogModelEnable && oldVal != analogModelEnable)
				{
					refreshTubeAmp();
#ifdef DEBUG
					LOGI("Tube amp enabled");
#endif
				}
				*replyData = 0;
				return 0;
			}
			/*			else if (cmd == 1207)
			{
			int16_t oldVal = wavechild670Enabled;
			int curStat = ((int16_t *)cep)[8];
			if (curStat)
			wavechild670Enabled = -1;
			else
			wavechild670Enabled = 0;
			if (!wavechild670Enabled)
			{
			if (compressor670)
			{
			free(compressor670);
			compressor670 = 0;
			}
			}
			*replyData = 0;
			return 0;
			}*/
			else if (cmd == 10003)
			{
				samplesInc = ((int16_t *)cep)[8];
				*replyData = 0;
				return 0;
			}
			else if (cmd == 10004)
			{
				size_t i, j, tempbufValue;
				if (finalImpulse)
				{
					for (i = 0; i < previousimpChannels; i++)
						free(finalImpulse[i]);
					free(finalImpulse);
					finalImpulse = 0;
				}
				if (!finalImpulse)
				{
					tempbufValue = impulseLengthActual * impChannels;
					size_t bufferSize = tempbufValue * sizeof(float);
					tempImpulseFloat = (float*)malloc(bufferSize);
					if (!tempImpulseFloat)
					{
						convolverReady = -1;
						convolverEnabled = !convolverEnabled;
					}
					memcpy(tempImpulseFloat, tempImpulseIncoming, bufferSize);
					free(tempImpulseIncoming);
					tempImpulseIncoming = 0;
					if (normalise < 0.99998f)
						normaliseToLevel(tempImpulseFloat, tempbufValue, normalise);
					finalImpulse = (float**)malloc(impChannels * sizeof(float*));
					for (i = 0; i < impChannels; i++)
					{
						float* channelbuf = (float*)malloc(impulseLengthActual * sizeof(float));
						if (!channelbuf)
						{
							convolverReady = -1;
							convolverEnabled = !convolverEnabled;
							free(finalImpulse);
							finalImpulse = 0;
						}
						float* p = tempImpulseFloat + i;
						for (j = 0; j < impulseLengthActual; j++)
							channelbuf[j] = p[j * impChannels];
						finalImpulse[i] = channelbuf;
					}
					if (!refreshConvolver(currentframeCountInit))
					{
						if (finalImpulse)
						{
							for (i = 0; i < impChannels; i++)
								free(finalImpulse[i]);
							free(finalImpulse);
							finalImpulse = 0;
						}
						if (tempImpulseFloat)
						{
							free(tempImpulseFloat);
							tempImpulseFloat = 0;
						}
						convolverReady = -1;
						convolverEnabled = !convolverEnabled;
					}
				}
				*replyData = 0;
				return 0;
			}
			else if (cmd == 1500)
			{
				finalGain = ((int16_t *)cep)[8] / 100.0f;
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 60)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 115)
			{
				float mBand[NUM_BANDS];
				for (int i = 0; i < NUM_BANDS; i++)
					mBand[i] = ((float*)cep)[4 + i];
				refreshEqBands(currentframeCountInit, mBand);
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1997)
			{
				if (isBenchData < 3)
				{
					for (uint32_t i = 0; i < 10; i++)
					{
						benchmarkValue[0][i] = (float)((float*)cep)[4 + i];
#ifdef DEBUG
//						LOGI("bench_c0: %lf", benchmarkValue[0][i]);
#endif
					}
					isBenchData++;
				}
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 40)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 1998)
			{
				if (isBenchData < 3)
				{
					for (uint32_t i = 0; i < 10; i++)
					{
						benchmarkValue[1][i] = (float)((float*)cep)[4 + i];
#ifdef DEBUG
//						LOGI("bench_c1: %lf", benchmarkValue[1][i]);
#endif
					}
					isBenchData++;
					if (convolverReady > 0)
					{
						if (!refreshConvolver(currentframeCountInit))
						{
							if (finalImpulse)
							{
								for (uint32_t i = 0; i < impChannels; i++)
									free(finalImpulse[i]);
								free(finalImpulse);
								finalImpulse = 0;
								free(tempImpulseFloat);
								tempImpulseFloat = 0;
							}
							convolverReady = -1;
							convolverEnabled = !convolverEnabled;
						}
					}
				}
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 9999)
			{
				previousimpChannels = impChannels;
				impChannels = ((int32_t *)cep)[5];
				impulseLengthActual = ((int32_t *)cep)[4] / impChannels;
				normalise = ((int32_t *)cep)[6] / 1000.0f;
				int numTime2Send = ((int32_t *)cep)[7];
				tempImpulseIncoming = (float*)calloc(4096 * impChannels * numTime2Send, sizeof(float));
				*replyData = 0;
				return 0;
			}
		}
		if (cep->psize == 4 && cep->vsize == 16384)
		{
			int32_t cmd = ((int32_t *)cep)[3];
			if (cmd == 12000)
			{
				memcpy(tempImpulseIncoming + (samplesInc * 4096), ((float*)cep) + 4, 4096 * sizeof(float));
				*replyData = 0;
				return 0;
			}
		}
		return -1;
	}
	return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}
void EffectDSPMain::refreshTubeAmp()
{
	if (!InitTube(&tubeP[0], 0, mSamplingRate, tubedrive, 6.5f, 5.6f, 4.5f, 4.8f, 6000, 0))
		analogModelEnable = 0;
	tubeP[1] = tubeP[0];
	rightparams2.tube = tubeP;
}
void EffectDSPMain::refreshBassLinearPhase(uint32_t actualframeCount, uint32_t tapsLPFIR, float bassBoostCentreFreq)
{
	float strength = bassBoostStrength / 100.0f;
	if (strength < 1.0f)
		strength = 1.0f;
	int filterLength = (int)tapsLPFIR;
	float transition = 80.0f;
	if (filterLength > 4096)
		transition = 40.0f;
	float freq[4] = { 0, (bassBoostCentreFreq * 2.0f) / mSamplingRate, (bassBoostCentreFreq * 2.0f + transition) / mSamplingRate , 1.0f };
	float amplitude[4] = { strength, strength, 0, 0 };
	float *freqSamplImp = fir2(&filterLength, freq, amplitude, 4);
	unsigned int i;
	if (!bassBoostLp)
	{
		bassBoostLp = (AutoConvolverMono**)malloc(sizeof(AutoConvolverMono*) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			bassBoostLp[i] = AllocateAutoConvolverMonoZeroLatency(freqSamplImp, filterLength, actualframeCount, threadResult);
		ramp = 0.4f;
	}
	else
	{
		for (i = 0; i < NUMCHANNEL; i++)
			UpdateAutoConvolverMonoZeroLatency(bassBoostLp[i], freqSamplImp, filterLength);
	}
	free(freqSamplImp);
#ifdef DEBUG
	LOGI("Linear phase bass boost allocate all done: total taps %d", filterLength);
#endif
	bassLpReady = 1;
}
int EffectDSPMain::refreshConvolver(uint32_t actualframeCount)
{
	if (!finalImpulse)
		return 0;
#ifdef DEBUG
	LOGI("refreshConvolver::IR channel count:%d, IR frame count:%d, Audio buffer size:%d", impChannels, impulseLengthActual, actualframeCount);
#endif
	unsigned int i;
	FreeConvolver();
	if (!convolver)
	{
		if (impChannels < 3)
		{
			convolver = (AutoConvolverMono**)malloc(sizeof(AutoConvolverMono*) * 2);
			if (!convolver)
				return 0;
			for (i = 0; i < 2; i++)
			{
				if (impChannels == 1)
					convolver[i] = InitAutoConvolverMono(finalImpulse[0], impulseLengthActual, actualframeCount, benchmarkValue, 12, threadResult);
				else
					convolver[i] = InitAutoConvolverMono(finalImpulse[i], impulseLengthActual, actualframeCount, benchmarkValue, 12, threadResult);
			}
			fullStconvparams.conv = convolver;
			fullStconvparams.out = outputBuffer;
			if (finalImpulse)
			{
				for (i = 0; i < impChannels; i++)
					free(finalImpulse[i]);
				free(finalImpulse);
				finalImpulse = 0;
				free(tempImpulseFloat);
				tempImpulseFloat = 0;
			}
			if (impulseLengthActual < 20000)
				convolverReady = 1;
			else
				convolverReady = 2;
#ifdef DEBUG
			LOGI("Convolver strategy used: %d", convolver[0]->methods);
#endif
		}
		else if (impChannels == 4)
		{
			fullStereoConvolver = (AutoConvolverMono**)malloc(sizeof(AutoConvolverMono*) * impChannels);
			if (!fullStereoConvolver)
				return 0;
			if (!fullStereoBuf)
			{
				fullStereoBuf = (float**)malloc(2 * sizeof(float*));
				fullStereoBuf[0] = (float*)malloc(actualframeCount * sizeof(float));
				fullStereoBuf[1] = (float*)malloc(actualframeCount * sizeof(float));
				fullStconvparams.out = fullStereoBuf;
				fullStconvparams1.out = fullStereoBuf;
			}
			for (i = 0; i < 4; i++)
				fullStereoConvolver[i] = InitAutoConvolverMono(finalImpulse[i], impulseLengthActual, actualframeCount, benchmarkValue, 12, threadResult);
			fullStconvparams.conv = fullStereoConvolver;
			fullStconvparams1.conv = fullStereoConvolver;
			if (finalImpulse)
			{
				for (i = 0; i < 4; i++)
					free(finalImpulse[i]);
				free(finalImpulse);
				finalImpulse = 0;
				free(tempImpulseFloat);
				tempImpulseFloat = 0;
			}
			if (impulseLengthActual < 8192)
				convolverReady = 3;
			else
				convolverReady = 4;
#ifdef DEBUG
			LOGI("Convolver strategy used: %d", fullStereoConvolver[0]->methods);
#endif
		}
		ramp = 0.4f;
#ifdef DEBUG
		LOGI("Convolver IR allocate complete");
#endif
	}
	return 1;
}
void EffectDSPMain::refreshStereoWiden(uint32_t parameter)
{
	switch (parameter)
	{
	case 0: // A Bit
		mMatrixMCoeff = 1.0f * 0.5f;
		mMatrixSCoeff = 1.2f * 0.5f;
		break;
	case 1: // Slight
		mMatrixMCoeff = 0.95f * 0.5f;
		mMatrixSCoeff = 1.4f * 0.5f;
		break;
	case 2: // Moderate
		mMatrixMCoeff = 0.9f * 0.5f;
		mMatrixSCoeff = 1.6f * 0.5f;
		break;
	case 3: // High
		mMatrixMCoeff = 0.85f * 0.5f;
		mMatrixSCoeff = 1.8f * 0.5f;
		break;
	case 4: // Super
		mMatrixMCoeff = 0.8f * 0.5f;
		mMatrixSCoeff = 2.0f * 0.5f;
		break;
	}
}
void EffectDSPMain::refreshCompressor()
{
	sf_advancecomp(&compressor, mSamplingRate, pregain, threshold, knee, ratio, attack, release, 0.003f, 0.09f, 0.16f, 0.42f, 0.98f, -(pregain / 1.4f));
	ramp = 0.3f;
}
void EffectDSPMain::refreshEqBands(uint32_t actualframeCount, float *bands)
{
	if (!arbEq || !xaxis || !yaxis)
		return;
#ifdef DEBUG
	LOGI("Allocating FIR Equalizer");
#endif
	const float interpFreq[NUM_BANDS] = { 25.0f, 40.0f, 63.0f, 100.0f, 160.0f, 250.0f, 400.0f, 630.0f, 1000.0f, 1600.0f, 2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f };
	float y2[NUM_BANDS];
	float workingBuf[NUM_BANDSM1]; // interpFreq or bands data length minus 1
	spline(&interpFreq[0], bands, NUM_BANDS, &y2[0], &workingBuf[0]);
	splint(&interpFreq[0], bands, &y2[0], NUM_BANDS, xaxis, yaxis, 1024, 1);
	int i;
	for (i = 0; i < 1024; i++)
		arbEq->nodes[i]->gain = yaxis[i];
	float *eqImpulseResponse = arbEq->GetFilter(arbEq, mSamplingRate);
	if (!FIREq)
	{
		FIREq = (AutoConvolverMono**)malloc(sizeof(AutoConvolverMono*) * NUMCHANNEL);
		for (i = 0; i < NUMCHANNEL; i++)
			FIREq[i] = AllocateAutoConvolverMonoZeroLatency(eqImpulseResponse, eqfilterLength, actualframeCount, threadResult);
	}
	else
	{
		for (i = 0; i < NUMCHANNEL; i++)
			UpdateAutoConvolverMonoZeroLatency(FIREq[i], eqImpulseResponse, eqfilterLength);
	}
#ifdef DEBUG
	LOGI("FIR Equalizer allocate all done: total taps %d", eqfilterLength);
#endif
	eqFIRReady = 1;
}
void EffectDSPMain::refreshReverb()
{
	if (mReverbMode == 1)
	{
		if (!verbL)
		{
			verbL = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, 50.0f, inBandwidth, earlyLv, tailLv);
			verbR = gverb_new(mSamplingRate, 3000.0f, roomSize, fxreTime, damping, 50.0f, inBandwidth, earlyLv, tailLv);
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		else
		{
			gverb_set_roomsize(verbL, roomSize);
			gverb_set_roomsize(verbR, roomSize);
			gverb_set_revtime(verbL, fxreTime);
			gverb_set_revtime(verbR, fxreTime);
			gverb_set_damping(verbL, damping);
			gverb_set_damping(verbR, damping);
			gverb_set_inputbandwidth(verbL, inBandwidth);
			gverb_set_inputbandwidth(verbR, inBandwidth);
			gverb_set_earlylevel(verbL, earlyLv);
			gverb_set_earlylevel(verbR, earlyLv);
			gverb_set_taillevel(verbL, tailLv);
			gverb_set_taillevel(verbR, tailLv);
		}
	}
	else
	{
		//Refresh reverb memory when mode changed
		if (verbL)
		{
			gverb_flush(verbL);
			gverb_flush(verbR);
		}
		if (mPreset < 0 || mPreset > 18)
			mPreset = 0;
		sf_presetreverb(&myreverb, mSamplingRate, (sf_reverb_preset)mPreset);
	}
}
void *EffectDSPMain::threadingConvF(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[0]->process(arguments->conv[0], arguments->in[0], arguments->out[0], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingConvF1(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[1]->process(arguments->conv[1], arguments->in[0], arguments->out[1], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingConvF2(void *args)
{
	ptrThreadParamsFullStConv *arguments = (ptrThreadParamsFullStConv*)args;
	arguments->conv[3]->process(arguments->conv[3], arguments->in[1], arguments->out[1], arguments->frameCount);
	return 0;
}
void *EffectDSPMain::threadingTube(void *args)
{
	ptrThreadParamsTube *arguments = (ptrThreadParamsTube*)args;
	processTube(&arguments->tube[1], arguments->in[1], arguments->in[1], arguments->frameCount);
	return 0;
}
int32_t EffectDSPMain::process(audio_buffer_t *in, audio_buffer_t *out)
{
	size_t i, j, frameCount = in->frameCount;
	float outLL, outLR, outRL, outRR;
	channel_split(in->s16, frameCount, inputBuffer);
	if (bassBoostEnabled)
	{
		if (bassLpReady > 0)
		{
			bassBoostLp[0]->process(bassBoostLp[0], inputBuffer[0], inputBuffer[0], frameCount);
			bassBoostLp[1]->process(bassBoostLp[1], inputBuffer[1], inputBuffer[1], frameCount);
		}
	}
	if (equalizerEnabled)
	{
		if (eqFIRReady == 1)
		{
			FIREq[0]->process(FIREq[0], inputBuffer[0], inputBuffer[0], frameCount);
			FIREq[1]->process(FIREq[1], inputBuffer[1], inputBuffer[1], frameCount);
		}
	}
	if (reverbEnabled)
	{
		if (mReverbMode == 1)
		{
			for (i = 0; i < frameCount; i++)
			{
				gverb_do(verbL, inputBuffer[0][i], &outLL, &outLR);
				gverb_do(verbR, inputBuffer[1][i], &outRL, &outRR);
				inputBuffer[0][i] = outLL + outRL;
				inputBuffer[1][i] = outLR + outRR;
			}
		}
		else
		{
			for (i = 0; i < frameCount; i++)
				sf_reverb_process(&myreverb, inputBuffer[0][i], inputBuffer[1][i], &inputBuffer[0][i], &inputBuffer[1][i]);
		}
	}
	if (convolverEnabled)
	{
		if (convolverReady == 1)
		{
			convolver[0]->process(convolver[0], inputBuffer[0], outputBuffer[0], frameCount);
			convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], frameCount);
			memcpy(inputBuffer[0], outputBuffer[0], memSize);
			memcpy(inputBuffer[1], outputBuffer[1], memSize);
		}
		else if (convolverReady == 2)
		{
			pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
			convolver[1]->process(convolver[1], inputBuffer[1], outputBuffer[1], frameCount);
			pthread_join(rightconv, 0);
			memcpy(inputBuffer[0], outputBuffer[0], memSize);
			memcpy(inputBuffer[1], outputBuffer[1], memSize);
		}
		else if (convolverReady == 3)
		{
			pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
			fullStereoConvolver[1]->process(fullStereoConvolver[1], inputBuffer[0], fullStereoBuf[1], frameCount);
			fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], frameCount);
			fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], frameCount);
			pthread_join(rightconv, 0);
			for (i = 0; i < frameCount; i++)
			{
				inputBuffer[0][i] = outputBuffer[0][i] + fullStereoBuf[0][i];
				inputBuffer[1][i] = outputBuffer[1][i] + fullStereoBuf[1][i];
			}
		}
		else if (convolverReady == 4)
		{
			pthread_create(&rightconv, 0, EffectDSPMain::threadingConvF, (void*)&fullStconvparams);
			pthread_create(&rightconv1, 0, EffectDSPMain::threadingConvF1, (void*)&fullStconvparams1);
			fullStereoConvolver[2]->process(fullStereoConvolver[2], inputBuffer[1], outputBuffer[0], frameCount);
			fullStereoConvolver[3]->process(fullStereoConvolver[3], inputBuffer[1], outputBuffer[1], frameCount);
			pthread_join(rightconv, 0);
			pthread_join(rightconv1, 0);
			for (i = 0; i < frameCount; i++)
			{
				inputBuffer[0][i] = outputBuffer[0][i] + fullStereoBuf[0][i];
				inputBuffer[1][i] = outputBuffer[1][i] + fullStereoBuf[1][i];
			}
		}
	}
	if (analogModelEnable)
	{
		pthread_create(&righttube, 0, EffectDSPMain::threadingTube, (void*)&rightparams2);
		processTube(&tubeP[0], inputBuffer[0], inputBuffer[0], frameCount);
		pthread_join(righttube, 0);
	}
	/*	if (wavechild670Enabled == 1)
	Wavechild670ProcessFloat(compressor670, inputBuffer[0], inputBuffer[1], inputBuffer[0], inputBuffer[1], frameCount);*/
	if (stereoWidenEnabled)
	{
		for (i = 0; i < frameCount; i++)
		{
			outLR = (inputBuffer[0][i] + inputBuffer[1][i]) * mMatrixMCoeff;
			outRL = (inputBuffer[0][i] - inputBuffer[1][i]) * mMatrixSCoeff;
			inputBuffer[0][i] = outLR + outRL;
			inputBuffer[1][i] = outLR - outRL;
		}
	}
	if (bs2bEnabled == 1)
	{
		double tmpL, tmpR;
		for (i = 0; i < frameCount; i++)
		{
			tmpL = (double)inputBuffer[0][i];
			tmpR = (double)inputBuffer[1][i];
			BS2BProcess(&bs2b, &tmpL, &tmpR);
			inputBuffer[0][i] = (float)tmpL;
			inputBuffer[1][i] = (float)tmpR;
		}
	}
	if (ramp < 1.0f)
		ramp += 0.05f;
	if (compressionEnabled)
	{
		sf_compressor_process(&compressor, frameCount, inputBuffer[0], inputBuffer[1], outputBuffer[0], outputBuffer[1]);
		channel_join(outputBuffer, out->s16, frameCount, ramp);
	}
	else
		channel_join(inputBuffer, out->s16, frameCount, ramp);
	return mEnable ? 0 : -ENODATA;
}
