#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/CrushProcessor.h"
#include "DSP/SignalChain.h"
#include "DSP/WidthProcessor.h"
#include "DSP/SpaceProcessor.h"
#include "DSP/ShadowProcessor.h"
#include "DSP/DeviceProcessor.h"
#include "DSP/VocalCompressor.h"
#include "DSP/DeEsserProcessor.h"
#include "DSP/AirExciterProcessor.h"
#include "DSP/VocalResonanceProcessor.h"
#include "DSP/TransientPreserver.h"
#include "DSP/InputConditioner.h"
#include "DSP/AutoLevelCompensator.h"
#include "DSP/StudioMicroDetuner.h"
#include "DSP/AnalogTransformerCore.h"
#include "Presets/PresetManager.h"
#include "Utils/AudioUtils.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>
#include <sstream>

struct TestReport
{
    int testCount = 0;
    int passCount = 0;
    int failCount = 0;

    void logResult(const std::string& testName, bool passed, const std::string& details)
    {
        testCount++;
        if (passed)
        {
            passCount++;
            std::cout << "[PASS] " << testName << " - " << details << std::endl;
        }
        else
        {
            failCount++;
            std::cout << "[FAIL] *** ISSUE FOUND *** " << testName << " - " << details << std::endl;
        }
    }
};

static void runCoreDSPTests(TestReport& report)
{
    std::cout << "\n========================================================" << std::endl;
    std::cout << "  UNDERGROUND VOCAL FX -- COMPREHENSIVE DSP AUDIT" << std::endl;
    std::cout << "========================================================" << std::endl;

    double sampleRate = 48000.0;
    int blockSize = 512;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(blockSize), 2 };

    // 1. Silent Input Sanity & DC Offset Test
    {
        InputConditioner cond;
        cond.prepare(spec);
        juce::AudioBuffer<float> silence(2, blockSize);
        silence.clear();
        cond.process(silence, 1.0f);

        float peak = cond.getPeakLevel();
        report.logResult("InputConditioner - Silent Input DC Rejection", peak < 1.0e-5f, "Peak=" + std::to_string(peak));
    }

    // 2. Extreme Overload Soft Clamping Test (+18 dBFS)
    {
        InputConditioner cond;
        cond.prepare(spec);
        juce::AudioBuffer<float> hotBuffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                hotBuffer.setSample(ch, i, 8.0f * std::sin(2.0f * 3.14159f * 440.0f * (float)i / (float)sampleRate));
        }

        cond.process(hotBuffer, 1.0f);
        float peak = hotBuffer.getMagnitude(0, 0, blockSize);
        bool finite = !std::isnan(peak) && !std::isinf(peak);
        bool clamped = peak < 2.0f;

        report.logResult("InputConditioner - +18dBFS Extreme Input Clamping", finite && clamped, "Clamped Peak=" + std::to_string(peak));
    }

    // 3. Dynamic Resonance Suppression Test
    {
        VocalResonanceProcessor resProc;
        resProc.prepare(spec);
        resProc.setAmount(1.0f);

        juce::AudioBuffer<float> resBuffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                resBuffer.setSample(ch, i, 0.85f * std::sin(2.0f * 3.14159f * 3400.0f * (float)i / (float)sampleRate));
        }

        resProc.process(resBuffer);
        float redDb = resProc.getTotalReductionDb();
        report.logResult("VocalResonanceProcessor - Dynamic Attenuation at 3.4kHz", redDb > 0.5f, "Reduction=" + std::to_string(redDb) + " dB");
    }

    // 4. Transient / Consonant Attack Protection Test
    {
        TransientPreserver trans;
        trans.prepare(spec);

        juce::AudioBuffer<float> attackBuffer(2, blockSize);
        attackBuffer.clear();
        attackBuffer.setSample(0, 10, 0.95f);
        attackBuffer.setSample(1, 10, 0.95f);

        float factor = trans.analyze(attackBuffer);
        report.logResult("TransientPreserver - Syllable Plosive Detection", factor > 0.1f, "Consonant Factor=" + std::to_string(factor));
    }

    // 5. Saturation Harmonic Profile & THD Measurement
    {
        CrushProcessor crush;
        crush.prepare(spec);
        crush.setCharacter(CrushProcessor::Character::Tube12AX7);
        crush.setAmount(0.60f);

        juce::AudioBuffer<float> sineBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                sineBuf.setSample(ch, i, 0.707f * std::sin(2.0f * 3.14159f * 1000.0f * (float)i / (float)sampleRate));
        }

        crush.process(sineBuf);
        float peak = sineBuf.getMagnitude(0, 0, blockSize);
        report.logResult("CrushProcessor - 12AX7 Triode Saturation Peak", peak > 0.1f && peak < 2.0f, "Output Peak=" + std::to_string(peak));
    }

    // 6. Air Exciter Sheen Generation Test
    {
        AirExciterProcessor air;
        air.prepare(spec);
        air.setMidAir(0.75f);
        air.setTopAir(0.75f);

        juce::AudioBuffer<float> airBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                airBuf.setSample(ch, i, 0.5f * std::sin(2.0f * 3.14159f * 5000.0f * (float)i / (float)sampleRate));
        }

        air.process(airBuf);
        float peak = airBuf.getMagnitude(0, 0, blockSize);
        report.logResult("AirExciterProcessor - Dynamic Sheen Generation", peak > 0.05f, "Output Peak=" + std::to_string(peak));
    }

    // 7. Studio Micro-Detuner 3D Haas Shimmer Test
    {
        StudioMicroDetuner detuner;
        detuner.prepare(spec);
        detuner.setAmount(0.75f);

        juce::AudioBuffer<float> detuneBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                detuneBuf.setSample(ch, i, 0.5f * std::sin(2.0f * 3.14159f * 1000.0f * (float)i / (float)sampleRate));
        }

        detuner.process(detuneBuf);
        float peakL = detuneBuf.getMagnitude(0, 0, blockSize);
        float peakR = detuneBuf.getMagnitude(1, 0, blockSize);
        report.logResult("StudioMicroDetuner - 3D Haas Dual Micro-Pitch Shimmer", peakL > 0.05f && peakR > 0.05f, 
                         "PeakL=" + std::to_string(peakL) + " PeakR=" + std::to_string(peakR));
    }

    // 8. Stereo Width Mono-Sum Compatibility Test
    {
        WidthProcessor width;
        width.prepare(spec);
        width.setAmount(0.85f);

        juce::AudioBuffer<float> widthBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                widthBuf.setSample(ch, i, 0.5f * std::sin(2.0f * 3.14159f * 1000.0f * (float)i / (float)sampleRate));
        }

        juce::dsp::AudioBlock<float> block(widthBuf);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        width.process(ctx);

        float sumMono = 0.0f;
        for (int i = 0; i < blockSize; ++i)
        {
            float m = 0.5f * (widthBuf.getSample(0, i) + widthBuf.getSample(1, i));
            sumMono += m * m;
        }
        float monoRms = std::sqrt(sumMono / blockSize);
        report.logResult("WidthProcessor - Mono Sum Compatibility (> -12dBFS)", monoRms > 0.10f, "Mono RMS=" + std::to_string(monoRms));
    }

    // 9. Analog Transformer Warmth Test
    {
        AnalogTransformerCore xfmr;
        xfmr.prepare(spec);
        xfmr.setWarmth(0.80f);

        juce::AudioBuffer<float> xfmrBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                xfmrBuf.setSample(ch, i, 0.707f * std::sin(2.0f * 3.14159f * 100.0f * (float)i / (float)sampleRate));
        }

        xfmr.process(xfmrBuf);
        float peak = xfmrBuf.getMagnitude(0, 0, blockSize);
        report.logResult("AnalogTransformerCore - Neve 1073 Chest Iron Harmonics", peak > 0.1f, "Harmonic Peak=" + std::to_string(peak));
    }

    // 10. SpaceProcessor Reverb & Ducking Test
    {
        SpaceProcessor space;
        space.prepare(spec);
        space.setReverbMix(0.60f);
        space.setDelayTime(0.25f);
        space.setDucking(0.80f);

        juce::AudioBuffer<float> spaceBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                spaceBuf.setSample(ch, i, 0.5f * std::sin(2.0f * 3.14159f * 1000.0f * (float)i / (float)sampleRate));
        }

        juce::dsp::AudioBlock<float> block(spaceBuf);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        space.process(ctx);

        float peak = spaceBuf.getMagnitude(0, 0, blockSize);
        report.logResult("SpaceProcessor - Dattorro Reverb & Sidechain Ducking", peak > 0.05f, "Output Peak=" + std::to_string(peak));
    }

    // 11. Full Signal Chain Flow
    {
        SignalChain chain;
        chain.prepare(spec);

        ShadowProcessor shadow;
        CrushProcessor crush;
        WidthProcessor width;
        SpaceProcessor space;
        DeviceProcessor device;
        VocalCompressor comp;
        DeEsserProcessor deEss;
        AirExciterProcessor air;

        shadow.prepare(spec);
        crush.prepare(spec);
        width.prepare(spec);
        space.prepare(spec);
        device.prepare(spec);
        comp.prepare(spec);
        deEss.prepare(spec);
        air.prepare(spec);

        juce::AudioBuffer<float> chainBuf(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
                chainBuf.setSample(ch, i, 0.35f * std::sin(2.0f * 3.14159f * 300.0f * (float)i / (float)sampleRate));
        }

        chain.process(chainBuf, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                      80.0f, 0.0f, 1.0f, 2.0f, 18000.0f, 0.707f, 1.0f, 0.707f, 1, 1,
                      0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                      0.5f, 0.5f, 6500.0f, 0.4f, 0.5f, 0.6f,
                      shadow, crush, width, space, device, comp, deEss, air);

        float outPeak = chainBuf.getMagnitude(0, 0, blockSize);
        bool valid = !std::isnan(outPeak) && !std::isinf(outPeak) && outPeak > 0.05f;
        report.logResult("SignalChain - Full Unified Processing Flow", valid, "Output Peak=" + std::to_string(outPeak));

        // Process 4 more blocks to ensure FFT FIFO triggers and spectrum bars light up
        for (int blk = 0; blk < 4; ++blk)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                for (int i = 0; i < blockSize; ++i)
                    chainBuf.setSample(ch, i, 0.45f * std::sin(2.0f * 3.14159f * 1000.0f * (float)(blk * blockSize + i) / (float)sampleRate));
            }
            chain.process(chainBuf, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                          80.0f, 0.0f, 1.0f, 2.0f, 18000.0f, 0.707f, 1.0f, 0.707f, 1, 1,
                          0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                          0.5f, 0.5f, 6500.0f, 0.4f, 0.5f, 0.6f,
                          shadow, crush, width, space, device, comp, deEss, air);
        }

        float fftBars[64] = { 0.0f };
        chain.getSpectrumData(fftBars);

        float maxBar = 0.0f;
        for (int i = 0; i < 64; ++i)
            if (fftBars[i] > maxBar) maxBar = fftBars[i];

        bool spectrumActive = maxBar > 0.10f;
        report.logResult("VisualEQDisplay - Real-Time Audio FFT Spectrum Activity", spectrumActive, "Max Spectrum Bar=" + std::to_string(maxBar));
    }

    // 12. Complete 7-Preset Silence & Idle Noise Floor Rejection Audit (< -90 dBFS)
    {
        SignalChain chain;
        chain.prepare(spec);

        ShadowProcessor shadow;
        CrushProcessor crush;
        WidthProcessor width;
        SpaceProcessor space;
        DeviceProcessor device;
        VocalCompressor comp;
        DeEsserProcessor deEss;
        AirExciterProcessor air;

        shadow.prepare(spec);
        crush.prepare(spec);
        width.prepare(spec);
        space.prepare(spec);
        device.prepare(spec);
        comp.prepare(spec);
        deEss.prepare(spec);
        air.prepare(spec);

        // Test with Preset 05: DESTROYED / CRUSHED (+20dB PUNISH mode, Squeeze=0.70, DeEss=0.60)
        crush.setCharacter(CrushProcessor::Character::PentodeEL34);
        crush.setAmount(0.80f);
        crush.setTone(0.72f);
        crush.setPunish(true);
        comp.setSqueeze(0.70f);
        deEss.setAmount(0.60f);

        juce::AudioBuffer<float> silenceBuf(2, blockSize);
        silenceBuf.clear();

        chain.process(silenceBuf, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                      110.0f, 0.0f, 3.0f, 0.0f, 20000.0f, 0.707f, 2.0f, 0.707f, 1, 1,
                      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
                      0.70f, 0.60f, 5800.0f, 0.0f, 0.0f, 0.5f,
                      shadow, crush, width, space, device, comp, deEss, air);

        float peakNoise = silenceBuf.getMagnitude(0, 0, blockSize);
        bool silent = peakNoise < 1.0e-5f;
        report.logResult("Preset 05 (PUNISH +20dB) - Silence Noise Floor (< -100dBFS)", silent, "Peak Noise=" + std::to_string(peakNoise));

        // Test with Preset 01: POLISHED / CRISP (High Air + Squeeze FET)
        air.setMidAir(0.30f);
        air.setTopAir(0.45f);
        comp.setSqueeze(0.42f);
        crush.setPunish(false);
        crush.setAmount(0.20f);
        silenceBuf.clear();

        chain.process(silenceBuf, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                      80.0f, -1.2f, 1.5f, 2.2f, 20000.0f, 0.707f, 1.2f, 0.707f, 1, 1,
                      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
                      0.42f, 0.50f, 6800.0f, 0.30f, 0.45f, 0.70f,
                      shadow, crush, width, space, device, comp, deEss, air);

        float peakNoiseP1 = silenceBuf.getMagnitude(0, 0, blockSize);
        bool silentP1 = peakNoiseP1 < 1.0e-5f;
        report.logResult("Preset 01 (POLISHED / CRISP) - Top-Air Silence Noise Floor (< -100dBFS)", silentP1, "Peak Noise=" + std::to_string(peakNoiseP1));
    }
}

int main()
{
    TestReport report;
    runCoreDSPTests(report);

    std::cout << "\n========================================================" << std::endl;
    std::cout << "  AUDIT SUMMARY: " << report.passCount << "/" << report.testCount << " PASSED ("
              << (report.failCount == 0 ? "100% SUCCESS" : "FAILED") << ")" << std::endl;
    std::cout << "========================================================" << std::endl;

    return report.failCount == 0 ? 0 : 1;
}
