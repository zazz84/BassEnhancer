/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class SecondOrderAllPass
{
public:
	SecondOrderAllPass();

	void init(int sampleRate);
	void setCoef(float frequency, float Q);
	float process(float in);

protected:
	float m_sampleRate;
	float m_a0 = 1.0f;
	float m_a1 = 0.0f;
	float m_a2 = 0.0f;
	float m_b0 = 0.0f;
	float m_b1 = 0.0f;
	float m_b2 = 1.0f;

	float m_x1 = 0.0f;
	float m_x2 = 0.0f;
	float m_y1 = 0.0f;
	float m_y2 = 0.0f;
};

//==============================================================================
class LowPassFilter
{
public:
	LowPassFilter() {};

	void init(int sampleRate) { m_SampleRate = sampleRate; }
	void setCoef(float frequency)
	{
		float warp = tan((frequency * 3.141593f) / m_SampleRate);
		m_OutLastCoef = (1 - warp) / (1 + warp);
		m_InCoef = warp / (1 + warp);
	}
	float process(float in)
	{ 
		m_OutLast = m_InCoef * (in + m_InLast) + m_OutLastCoef * m_OutLast;
		m_InLast = in;
		return m_OutLast;
	}

protected:
	int   m_SampleRate = 48000;
	float m_InCoef = 1.0f;
	float m_OutLastCoef = 0.0f;

	float m_OutLast = 0.0f;
	float m_InLast = 0.0f;
};

//==============================================================================
class LadderFilter
{
public:
	void init(int sampleRate)
	{
		m_SampleRate = sampleRate;
		m_lowPassFilter[0].init(sampleRate);
		m_lowPassFilter[1].init(sampleRate);
		m_lowPassFilter[2].init(sampleRate);
		m_lowPassFilter[3].init(sampleRate);
	}
	void setCoef(float frequency)
	{
		m_lowPassFilter[0].setCoef(frequency);
		m_lowPassFilter[1].setCoef(frequency);
		m_lowPassFilter[2].setCoef(frequency);
		m_lowPassFilter[3].setCoef(frequency);
	}
	void setResonance(float resonance)
	{
		m_resonance = resonance;
	}
	float process(float in)
	{
		float lowPass = in - m_resonance * m_OutLast;

		lowPass = m_lowPassFilter[0].process(lowPass);
		lowPass = m_lowPassFilter[1].process(lowPass);
		lowPass = m_lowPassFilter[2].process(lowPass);
		lowPass = m_lowPassFilter[3].process(lowPass);

		m_OutLast = lowPass;
		return lowPass;
	}

protected:
	LowPassFilter m_lowPassFilter[4] = {};
	int   m_SampleRate = 48000;

	float m_OutLast = 0.0f;
	float m_resonance = 0.0f;
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
	std::atomic<float>* gainParameter = nullptr;
	std::atomic<float>* mixParameter = nullptr;
	std::atomic<float>* volumeParameter = nullptr;

	juce::AudioParameterBool* buttonAParameter = nullptr;
	juce::AudioParameterBool* buttonBParameter = nullptr;
	juce::AudioParameterBool* buttonCParameter = nullptr;
	juce::AudioParameterBool* buttonDParameter = nullptr;

	LowPassFilter12dB  m_lowPassFilter[2] = {};
	LadderFilter       m_ladderFilter[2] = {};
	SecondOrderAllPass m_secondOrderAllPass[2] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassEnhancerAudioProcessor)
};
