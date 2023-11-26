/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class LowPassFilter
{
public:
	LowPassFilter() {};

	void init(int sampleRate) { m_SampleRate = sampleRate; }
	void setCoef(float frequency)
	{
		m_InCoef = frequency * 3.14f / m_SampleRate;
		m_OutLastCoef = 1.0f - m_InCoef;
	}
	float process(float in) { return m_OutLast = m_InCoef * in + m_OutLastCoef * m_OutLast; }

protected:
	int   m_SampleRate  = 48000;
	float m_InCoef      = 1.0f;
	float m_OutLastCoef = 0.0f;

	float m_OutLast = 0.0f;
};

//==============================================================================

class LowPassFilter12dB : public LowPassFilter
{
public:
	float process(float in)
	{
		m_OutLast  = m_InCoef * in + m_OutLastCoef * m_OutLast;
		return m_OutLast2 = m_InCoef * m_OutLast + m_OutLastCoef * m_OutLast2;
	}

protected:
	float m_OutLast2 = 0.0f;
};

//==============================================================================
class BassEnhancerAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    BassEnhancerAudioProcessor();
    ~BassEnhancerAudioProcessor() override;

	static const std::string paramsNames[];

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	using APVTS = juce::AudioProcessorValueTreeState;
	static APVTS::ParameterLayout createParameterLayout();

	APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:	
	//==============================================================================
	std::atomic<float>* frequencyParameter = nullptr;
	std::atomic<float>* thresholdParameter = nullptr;
	std::atomic<float>* mixParameter = nullptr;
	std::atomic<float>* volumeParameter = nullptr;

	LowPassFilter12dB m_preFilter[2] = {};
	LowPassFilter12dB m_postFilter[2] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassEnhancerAudioProcessor)
};
