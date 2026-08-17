#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ShadowProcessor.h"
#include "CrushProcessor.h"
#include "WidthProcessor.h"
#include "SpaceProcessor.h"
#include "DeviceProcessor.h"
#include "EQEngine.h"
#include "VocalCompressor.h"
#include "DeEsserProcessor.h"
#include "AirExciterProcessor.h"
#include "VocalResonanceProcessor.h"
#include "TransientPreserver.h"
#include "InputConditioner.h"
#include "AutoLevelCompensator.h"
#include "StudioMicroDetuner.h"
#include "AnalogTransformerCore.h"

struct AudioDynamicNode
{
    float freqHz { 1000.0f };
    float gainDb { 0.0f };
    float qFactor { 0.707f };
    int filterType { 0 };
    int stereoMode { 0 };
    bool active { true };
};

class SignalChain
{
public:
    SignalChain();
    ~SignalChain();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void updateDynamicNodes(const std::vector<AudioDynamicNode>& nodes)
    {
        juce::ScopedLock sl(nodeLock);
        activeNodes = nodes;
    }

    void process(juce::AudioBuffer<float>& buffer, 
                 float inputGainDb,
                 float outputGainDb,
                 float globalMix,
                 float degenerateMacro,
                 float subEnable,
                 float gritEnable,
                 float modEnable,
                 float delayEnable,
                 float reverbEnable,
                 float eqLowCutFreq,
                 float eqLowGainDb,
                 float eqMidGainDb,
                 float eqHighGainDb,
                 float eqHighCutFreq,
                 float eqLowQ,
                 float eqMidQ,
                 float eqHighQ,
                 int eqLowCutSlope,
                 int eqHighCutSlope,
                 float macroDepth,
                 float macroDark,
                 float macroMotion,
                 float macroChaos,
                 float macroAge,
                 float macroGhost,
                 float macroTone,
                 float compSqueeze,
                 float deEssAmount,
                 float deEssFreq,
                 float airMid,
                 float airTop,
                 float spaceDucking,
                 ShadowProcessor& shadowModule,
                 CrushProcessor& crushModule,
                 WidthProcessor& widthModule,
                 SpaceProcessor& spaceModule,
                 DeviceProcessor& deviceModule,
                 VocalCompressor& compModule,
                 DeEsserProcessor& deEssModule,
                 AirExciterProcessor& airModule);

    float getInputLevel() const noexcept { return inputLevelPeak.load(); }
    float getOutputLevel() const noexcept { return outputLevelPeak.load(); }

    VocalResonanceProcessor& getResonanceProcessor() noexcept { return vocalResonanceProcessor; }
    AutoLevelCompensator& getAutoLevelCompensator() noexcept { return autoLevelCompensator; }
    TransientPreserver& getTransientPreserver() noexcept { return transientPreserver; }
    StudioMicroDetuner& getMicroDetuner() noexcept { return studioMicroDetuner; }
    AnalogTransformerCore& getTransformerCore() noexcept { return analogTransformerCore; }

    void getSpectrumData(float* dest64Bars) const noexcept;

private:
    double sampleRate { 44100.0 };

    juce::CriticalSection nodeLock;
    std::vector<AudioDynamicNode> activeNodes;

    std::atomic<float> inputLevelPeak { 0.0f };
    std::atomic<float> outputLevelPeak { 0.0f };

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> parallelSatBuffer;
    juce::AudioBuffer<float> parallelShadowBuffer;
    juce::AudioBuffer<float> parallelWidthBuffer;
    juce::AudioBuffer<float> parallelSpaceBuffer;

    // Advanced $200 Boutique Vocal Processing Engines
    InputConditioner inputConditioner;

    AnalogTransformerCore analogTransformerCore;
    VocalResonanceProcessor vocalResonanceProcessor;
    TransientPreserver transientPreserver;
    StudioMicroDetuner studioMicroDetuner;
    AutoLevelCompensator autoLevelCompensator;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> globalMixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> degenerateSmoother;

    EQEngine masterEQEngine;
    juce::dsp::Limiter<float> outputLimiter;

    static constexpr int maxDynamicNodes = 10;

    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;

    juce::dsp::FFT fftEngine { fftOrder };
    juce::dsp::WindowingFunction<float> windowEngine { fftSize, juce::dsp::WindowingFunction<float>::hann };

    float fifoBuffer[fftSize] { 0.0f };
    float fftData[fftSize * 2] { 0.0f };
    int fifoIndex { 0 };

    mutable float spectrumBars[64] { 0.0f };
    mutable std::atomic<bool> nextFFTBlockReady { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SignalChain)
};
