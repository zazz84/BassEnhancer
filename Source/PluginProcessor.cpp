/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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

	// Mics constants
	const float gain = juce::Decibels::decibelsToGain(gainNormalized * 48.0f);
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

		preFilter.setCoef(frequency);
		postFilter.setCoef(frequency);
		ladderFilter.setCoef(frequency);

		if (buttonA)
		{
			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];

				// Pre filter
				const float inPreFilter = ladderFilter.process(in, 3.0f) * gain;

				//Distort
				const float inPreFilterAbs = fabs(inPreFilter);
				const float sign = inPreFilter / inPreFilterAbs;
				const float inDistort = (inPreFilterAbs > 0.25f) ? sign : 0.0f;

				// Post filter
				const float inPostFilter = postFilter.process(inDistort);

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPostFilter + mixInverse * in);
			}
		}
		else if (buttonB)
		{
			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];

				// Pre filter
				const float inPreFilter = ladderFilter.process(in, 3.6f) * gain;

				//Distort
				const float inPreFilterAbs = fabs(inPreFilter);
				const float inDistort = inPreFilter / (1.0f + inPreFilterAbs);

				// Post filter
				const float inPostFilter = postFilter.process(inDistort);

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPostFilter + mixInverse * in);
			}
		}
		else
		{
			for (int sample = 0; sample < samples; ++sample)
			{
				// Get input
				const float in = channelBuffer[sample];

				// Pre filter
				const float inPreFilter = ladderFilter.process(in, 3.9f * gainNormalized);

				// Apply volume, mix and send to output
				channelBuffer[sample] = volume * (mix * inPreFilter + mixInverse * in);
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

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassEnhancerAudioProcessor();
}
