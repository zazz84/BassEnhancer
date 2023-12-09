/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SecondOrderAllPass::SecondOrderAllPass()
{
}

void SecondOrderAllPass::init(int sampleRate)
{
	m_sampleRate = sampleRate;
}

void SecondOrderAllPass::setCoef(float frequency, float Q)
{
	if (m_sampleRate == 0)
	{
		return;
	}

	float bandWidth = frequency / Q;
	float t = std::tanf(3.14f * bandWidth / m_sampleRate);
	float c = (t - 1.0f) / (t + 1.0f);
	float d = -1.0f * std::cosf(2.0f * 3.14f * frequency / m_sampleRate);
	
	m_a1 = d * (1.0f - c);
	m_a2 = -c;
	m_b0 = m_a2;
	m_b1 = m_a1;
}

float SecondOrderAllPass::process(float in)
{
	float y = m_b0 * in + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;

	m_y2 = m_y1;
	m_y1 = y;
	m_x2 = m_x1;
	m_x1 = in;

	return y;
}
//==============================================================================

const std::string BassEnhancerAudioProcessor::paramsNames[] = { "Frequency", "Gain", "Mix", "Volume" };

//==============================================================================
BassEnhancerAudioProcessor::BassEnhancerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	frequencyParameter = apvts.getRawParameterValue(paramsNames[0]);
	gainParameter      = apvts.getRawParameterValue(paramsNames[1]);
	mixParameter       = apvts.getRawParameterValue(paramsNames[2]);
	volumeParameter    = apvts.getRawParameterValue(paramsNames[3]);

	buttonAParameter = static_cast<juce::AudioParameterBool*>(apvts.getParameter("ButtonA"));
	buttonBParameter = static_cast<juce::AudioParameterBool*>(apvts.getParameter("ButtonB"));
	buttonCParameter = static_cast<juce::AudioParameterBool*>(apvts.getParameter("ButtonC"));
	buttonDParameter = static_cast<juce::AudioParameterBool*>(apvts.getParameter("ButtonD"));
}

BassEnhancerAudioProcessor::~BassEnhancerAudioProcessor()
{
}

//==============================================================================
const juce::String BassEnhancerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BassEnhancerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BassEnhancerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BassEnhancerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BassEnhancerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BassEnhancerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BassEnhancerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BassEnhancerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BassEnhancerAudioProcessor::getProgramName (int index)
{
    return {};
}

void BassEnhancerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BassEnhancerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{	
	m_preFilter[0].init((int)(sampleRate));
	m_preFilter[1].init((int)(sampleRate));

	m_postFilter[0].init((int)(sampleRate));
	m_postFilter[1].init((int)(sampleRate));

	m_ladderFilter[0].init((int)(sampleRate));
	m_ladderFilter[1].init((int)(sampleRate));

	m_secondOrderAllPass[0].init((int)(sampleRate));
	m_secondOrderAllPass[1].init((int)(sampleRate));
}

void BassEnhancerAudioProcessor::releaseResources()
{
	
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BassEnhancerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BassEnhancerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	// Get params
	const auto frequency = frequencyParameter->load();
	const auto gainNormalized = gainParameter->load();
	const auto mix = mixParameter->load();
	const auto volume = juce::Decibels::decibelsToGain(volumeParameter->load());

	// Buttons
	const auto buttonA = buttonAParameter->get();
	const auto buttonB = buttonBParameter->get();
	const auto buttonC = buttonCParameter->get();
	const auto buttonD = buttonDParameter->get();

	// Mics constants
	const float gain = juce::Decibels::decibelsToGain(gainNormalized * 18.0f);
	const float mixInverse = 1.0f - mix;
	const int channels = getTotalNumOutputChannels();
	const int samples = buffer.getNumSamples();		

	for (int channel = 0; channel < channels; ++channel)
	{
		// Channel pointer
		auto* channelBuffer = buffer.getWritePointer(channel);

		// Filters
		auto& preFilter = m_preFilter[channel];
		auto& postFilter = m_postFilter[channel];
		auto& ladderFilter = m_ladderFilter[channel];
		auto& secondOrderAllPass = m_secondOrderAllPass[channel];

		preFilter.setCoef(frequency);
		postFilter.setCoef(frequency);
		// Arbitrary factor to make LP resonance peak at input frequency
		const float LADDER_FILTER_FREQUENCY_FACTOR = 1.23f;
		ladderFilter.setCoef(frequency * LADDER_FILTER_FREQUENCY_FACTOR);
		secondOrderAllPass.setCoef(frequency, 7.0f);

		if (buttonA)
		{
			// Arbitrary volume compensation to make resonance peak af 0dbFS
			float volumeCompensation = juce::Decibels::decibelsToGain(1.6f);
			float volumeCompensationPostFilter = juce::Decibels::decibelsToGain(12.0f);

			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];

				// Pre filter
				const float inPreFilter = ladderFilter.process(in, 2.0f) * gain * volumeCompensation;

				//Distort
				const float inPreFilterAbs = fabsf(inPreFilter);
				const float inDistort = fmaxf(-1.0f, fminf(1.0f, inPreFilter / (1.0f + inPreFilterAbs)));

				// Post filter
				const float inPostFilter = postFilter.process(inDistort) * volumeCompensationPostFilter;

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPostFilter + mixInverse * in);
			}
		}
		else if (buttonB)
		{
			// Arbitrary volume compensation to make resonance peak af 0dbFS
			float volumeCompensationPostFilter = juce::Decibels::decibelsToGain(18.0f);

			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];

				// Pre filter
				const float inBandPass = 0.5f * (in - secondOrderAllPass.process(in)) * gain;

				//Distort
				const float inBandPassAbs = fabsf(inBandPass);
				const float inDistort = fmaxf(-1.0f, fminf(1.0f, inBandPass / (1.0f + inBandPassAbs)));

				// Post filter
				const float inPostFilter = postFilter.process(inDistort) * volumeCompensationPostFilter;

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPostFilter + mixInverse * in);
			}
		}
		else if (buttonC)
		{
			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];
				
				// Pre filter
				const float inPreFilter = 0.5f * (in - secondOrderAllPass.process(in)) * gain;

				//Distort
				const float inPreFilterAbs = fabsf(inPreFilter);
				const float sign = inPreFilter / inPreFilterAbs;
				const float inDistort = (inPreFilterAbs > 0.1f) ? sign : 0.0f;

				// Post filter
				const float inPostFilter = ladderFilter.process(sign * inDistort, 2.0f);

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPostFilter + mixInverse * in);
			}
		}
		else
		{
			// Arbitrary volume compensation to make resonance peak af 0dbFS
			float volumeCompensationPostFilter = juce::Decibels::decibelsToGain(6.0f);

			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];

				// Pre filter
				const float inPreFilter = 0.5f * (in - secondOrderAllPass.process(in)) * gain;

				//Distort
				const float inPreFilterAbs = fabsf(inPreFilter);
				const float sign = inPreFilter / inPreFilterAbs;
				const float inPreFilterAbsLimit = fminf(inPreFilterAbs, 1.0f);
				const float inDistort = powf(inPreFilterAbsLimit, 2.0f);

				// Post filter
				const float inPostFilter = ladderFilter.process(sign * inDistort, 2.0f) * volumeCompensationPostFilter;

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPostFilter + mixInverse * in);
			}
		}
	}
}

//==============================================================================
bool BassEnhancerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BassEnhancerAudioProcessor::createEditor()
{
    return new BassEnhancerAudioProcessorEditor (*this, apvts);
}

//==============================================================================
void BassEnhancerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{	
	auto state = apvts.copyState();
	std::unique_ptr<juce::XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void BassEnhancerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if (xmlState.get() != nullptr)
		if (xmlState->hasTagName(apvts.state.getType()))
			apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout BassEnhancerAudioProcessor::createParameterLayout()
{
	APVTS::ParameterLayout layout;

	using namespace juce;

	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[0], paramsNames[0], NormalisableRange<float>( 40.0f, 400.0f,  1.0f, 1.0f), 80.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[1], paramsNames[1], NormalisableRange<float>(  0.0f,   1.0f, 0.05f, 1.0f),  0.5f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[2], paramsNames[2], NormalisableRange<float>(  0.0f,   1.0f, 0.05f, 1.0f),  1.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(paramsNames[3], paramsNames[3], NormalisableRange<float>(-24.0f,  24.0f,  0.1f, 1.0f),  0.0f));

	layout.add(std::make_unique<juce::AudioParameterBool>("ButtonA", "ButtonA", true));
	layout.add(std::make_unique<juce::AudioParameterBool>("ButtonB", "ButtonB", false));
	layout.add(std::make_unique<juce::AudioParameterBool>("ButtonC", "ButtonC", false));
	layout.add(std::make_unique<juce::AudioParameterBool>("ButtonD", "ButtonC", false));

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassEnhancerAudioProcessor();
}
