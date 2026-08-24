#include "KeyDetector.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

// Krumhansl-Schmuckler Key Profiles (12 pitch classes)
static const float sMajorProfile[12] = {
    6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
};

static const float sMinorProfile[12] = {
    6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f
};

static const char* sNoteNames[12] = {
    "C", "C# / Db", "D", "D# / Eb", "E", "F", "F# / Gb", "G", "G# / Ab", "A", "A# / Bb", "B"
};

static const char* sShortNoteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

juce::String KeyDetector::getNoteName(int note0to11)
{
    int n = ((note0to11 % 12) + 12) % 12;
    return sNoteNames[n];
}

KeyDetector::KeyDetector()
    : juce::Thread("PlugTuneKeyDetectorThread")
{
    mFormatManager.registerBasicFormats();
}

KeyDetector::~KeyDetector()
{
    cancel();
}

void KeyDetector::cancel()
{
    signalThreadShouldExit();
    stopThread(2000);
}

void KeyDetector::analyzeFileAsync(const juce::File& file, std::function<void(KeyDetectionOutcome)> onComplete)
{
    cancel();
    mTargetFile = file;
    mCallback = onComplete;
    mIsAnalyzing = true;
    startThread(juce::Thread::Priority::normal);
}

void KeyDetector::run()
{
    std::unique_ptr<juce::AudioFormatReader> reader(mFormatManager.createReaderFor(mTargetFile));
    KeyDetectionOutcome outcome;

    if (reader != nullptr && reader->lengthInSamples > 0 && !threadShouldExit())
    {
        outcome = analyzeAudio(reader.get());
    }
    else
    {
        outcome.success = false;
        outcome.keyName = "Unable to read audio file";
    }

    mIsAnalyzing = false;

    if (!threadShouldExit() && mCallback)
    {
        juce::MessageManager::callAsync([cb = mCallback, res = outcome]() {
            cb(res);
        });
    }
}

KeyDetectionOutcome KeyDetector::analyzeAudio(juce::AudioFormatReader* reader)
{
    KeyDetectionOutcome outcome;
    double sampleRate = reader->sampleRate;
    int64_t totalSamples = reader->lengthInSamples;

    if (sampleRate < 8000.0 || totalSamples < 1000)
        return outcome;

    // Read up to 60 seconds of audio from the track
    int maxSamplesToRead = static_cast<int>(std::min(totalSamples, static_cast<int64_t>(sampleRate * 60.0)));
    juce::AudioBuffer<float> tempBuffer(1, maxSamplesToRead);

    reader->read(&tempBuffer, 0, maxSamplesToRead, 0, true, false);

    const float* audioData = tempBuffer.getReadPointer(0);

    // Compute 12-bin Pitch Class Profile (Chroma)
    std::array<float, 12> chroma;
    chroma.fill(0.0f);

    int windowSize = 2048;
    int hop = 1024;

    for (int pos = 0; pos + windowSize < maxSamplesToRead; pos += hop)
    {
        if (threadShouldExit()) return outcome;

        for (int i = 0; i < windowSize; i += 8)
        {
            float val = std::abs(audioData[pos + i]);
            if (val > 0.01f)
            {
                // Map approximate frequency to pitch chroma
                // Simple fast time-domain autocorrelation estimate per block
                int bestLag = 0;
                float bestCorr = 0.0f;
                for (int lag = 20; lag < 300; lag += 2)
                {
                    if (pos + i + lag < maxSamplesToRead)
                    {
                        float c = audioData[pos + i] * audioData[pos + i + lag];
                        if (c > bestCorr)
                        {
                            bestCorr = c;
                            bestLag = lag;
                        }
                    }
                }

                if (bestLag > 0)
                {
                    float freq = static_cast<float>(sampleRate) / static_cast<float>(bestLag);
                    if (freq >= 55.0f && freq <= 1500.0f)
                    {
                        int midi = static_cast<int>(std::round(69.0f + 12.0f * std::log2(freq / 440.0f)));
                        int cBin = ((midi % 12) + 12) % 12;
                        chroma[cBin] += bestCorr;
                    }
                }
            }
        }
    }

    // Normalize Chroma Vector
    float maxChroma = *std::max_element(chroma.begin(), chroma.end());
    if (maxChroma > 0.0f)
    {
        for (auto& v : chroma) v /= maxChroma;
    }

    // Correlate against 24 Key Profiles (12 Major, 12 Minor)
    float bestScore = -1.0f;
    int bestKey = 0;
    bool bestIsMinor = true;

    for (int k = 0; k < 12; ++k)
    {
        // Major correlation
        float majCorr = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            majCorr += chroma[(k + i) % 12] * sMajorProfile[i];
        }
        if (majCorr > bestScore)
        {
            bestScore = majCorr;
            bestKey = k;
            bestIsMinor = false;
        }

        // Minor correlation
        float minCorr = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            minCorr += chroma[(k + i) % 12] * sMinorProfile[i];
        }
        if (minCorr > bestScore)
        {
            bestScore = minCorr;
            bestKey = k;
            bestIsMinor = true;
        }
    }

    outcome.rootKey = bestKey;
    outcome.isMinor = bestIsMinor;
    outcome.confidence = std::clamp(bestScore / 35.0f, 0.70f, 0.99f);
    outcome.keyName = juce::String(sNoteNames[bestKey]) + (bestIsMinor ? " Minor (" + juce::String(sShortNoteNames[bestKey]) + "m)" 
                                                                       : " Major (" + juce::String(sShortNoteNames[bestKey]) + "M)");
    outcome.success = true;

    return outcome;
}

} // namespace PlugTuneDSP
