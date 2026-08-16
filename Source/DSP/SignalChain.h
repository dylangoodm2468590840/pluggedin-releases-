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



struct AudioDynamicNode
{
    float freqHz { 1000.0f };
    float gainDb { 0.0f };
    float qFactor { 0.707f };
    int filterType { 0 };  // 0: Bell, 1: Low Cut, 2: High Cut, 3: Low Shelf, 4: High Shelf, 5: Notch
    int stereoMode { 0 };  // 0: Stereo (L+R), 1: Mid (M), 2: Side (S), 3: Left, 4: Right
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

    // Lock-free spectrum getter for VisualEQDisplay
    void getSpectrumData(float* dest64Bars) const noexcept;

private:
    double sampleRate { 44100.0 };

    juce::CriticalSection nodeLock;
    std::vector<AudioDynamicNode> activeNodes;

    std::atomic<float> inputLevelPeak { 0.0f };
    std::atomic<float> outputLevelPeak { 0.0f };

    // Pre-allocated dry buffer to eliminate audio thread heap allocations
    juce::AudioBuffer<float> dryBuffer;

    // Smoothed parameters for zipper-free control
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> globalMixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> degenerateSmoother;

    // 64-Bit Transposed Direct Form II Master EQ Engine with Nyquist De-cramping
    EQEngine masterEQEngine;

    // Output protection brickwall limiter
    juce::dsp::Limiter<float> outputLimiter;

    // 5-Band Parametric Audio Biquad Filters (Cascaded for steep slopes)
    juce::dsp::IIR::Filter<float> eqLowCutFilter1;
    juce::dsp::IIR::Filter<float> eqLowCutFilter2;
    juce::dsp::IIR::Filter<float> eqLowBellFilter;
    juce::dsp::IIR::Filter<float> eqMidBellFilter;
    juce::dsp::IIR::Filter<float> eqHighShelfFilter;
    juce::dsp::IIR::Filter<float> eqHighCutFilter1;
    juce::dsp::IIR::Filter<float> eqHighCutFilter2;

    // Dynamic Biquad Audio Filter Cascade
    static constexpr int maxDynamicNodes = 8;
    juce::dsp::IIR::Filter<float> dynamicBiquads[maxDynamicNodes];

    // Real-Time 2048-Point FFT Spectrum Analyzer Engine
    static constexpr int fftOrder = 11; // 2048 points
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
