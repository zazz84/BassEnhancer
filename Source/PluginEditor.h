/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class BassEnhancerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    BassEnhancerAudioProcessorEditor (BassEnhancerAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~BassEnhancerAudioProcessorEditor() override;

	// GUI setup
	static const int N_SLIDERS_COUNT = 4;
	static const int SCALE = 70;
	static const int LABEL_OFFSET = 25;
	static const int SLIDER_WIDTH = 200;
	static const int BOTTOM_MENU_HEIGHT = 50;
	static const int HUE = 70;

	static const int TYPE_BUTTON_GROUP = 1;

    //==============================================================================
	void paint (juce::Graphics&) override;
    void resized() override;

	typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
	typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;

private:
    BassEnhancerAudioProcessor& audioProcessor;

	juce::AudioProcessorValueTreeState& valueTreeState;

	juce::Label m_labels[N_SLIDERS_COUNT] = {};
	juce::Slider m_sliders[N_SLIDERS_COUNT] = {};
	std::unique_ptr<SliderAttachment> m_sliderAttachment[N_SLIDERS_COUNT] = {};

	juce::Label automationTLabel;
	juce::Label smoothingTypeLabel;
	juce::Label detectionTypeLabel;

	juce::TextButton typeAButton{ "A" };
	juce::TextButton typeBButton{ "B" };
	juce::TextButton typeCButton{ "C" };
	juce::TextButton typeDButton{ "D" };

	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonBAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonCAttachment;
	std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonDAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassEnhancerAudioProcessorEditor)
};
