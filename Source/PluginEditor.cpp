/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::Colour BassEnhancerAudioProcessorEditor::veryLight  = juce::Colour::fromHSV(0.7f, 0.5f, 0.7f, 1.0f);
const juce::Colour BassEnhancerAudioProcessorEditor::light      = juce::Colour::fromHSV(0.7f, 0.5f, 0.6f, 1.0f);
const juce::Colour BassEnhancerAudioProcessorEditor::medium     = juce::Colour::fromHSV(0.7f, 0.5f, 0.5f, 1.0f);
const juce::Colour BassEnhancerAudioProcessorEditor::dark       = juce::Colour::fromHSV(0.7f, 0.5f, 0.4f, 1.0f);

//==============================================================================
BassEnhancerAudioProcessorEditor::BassEnhancerAudioProcessorEditor (BassEnhancerAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState(vts)
{
	getLookAndFeel().setColour(juce::Slider::thumbColourId, dark);
	getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, medium);
	getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId, light);

	for (int i = 0; i < N_SLIDERS_COUNT; i++)
	{
		auto& label = m_labels[i];
		auto& slider = m_sliders[i];

		//Lable
		label.setText(BassEnhancerAudioProcessor::paramsNames[i], juce::dontSendNotification);
		label.setFont(juce::Font(24.0f * 0.01f * SCALE, juce::Font::bold));
		label.setJustificationType(juce::Justification::centred);
		addAndMakeVisible(label);

		//Slider
		slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
		slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
		addAndMakeVisible(slider);
		m_sliderAttachment[i].reset(new SliderAttachment(valueTreeState, BassEnhancerAudioProcessor::paramsNames[i], slider));
	}

	// Buttons
	addAndMakeVisible(typeAButton);
	addAndMakeVisible(typeBButton);
	addAndMakeVisible(typeCButton);
	addAndMakeVisible(typeDButton);

	typeAButton.setRadioGroupId(TYPE_BUTTON_GROUP);
	typeBButton.setRadioGroupId(TYPE_BUTTON_GROUP);
	typeCButton.setRadioGroupId(TYPE_BUTTON_GROUP);
	typeDButton.setRadioGroupId(TYPE_BUTTON_GROUP);

	typeAButton.setClickingTogglesState(true);
	typeBButton.setClickingTogglesState(true);
	typeCButton.setClickingTogglesState(true);
	typeDButton.setClickingTogglesState(true);

	buttonAAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(valueTreeState, "ButtonA", typeAButton));
	buttonBAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(valueTreeState, "ButtonB", typeBButton));
	buttonCAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(valueTreeState, "ButtonC", typeCButton));
	buttonDAttachment.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(valueTreeState, "ButtonD", typeDButton));

	typeAButton.setColour(juce::TextButton::buttonColourId, light);
	typeBButton.setColour(juce::TextButton::buttonColourId, light);
	typeCButton.setColour(juce::TextButton::buttonColourId, light);
	typeDButton.setColour(juce::TextButton::buttonColourId, light);

	typeAButton.setColour(juce::TextButton::buttonOnColourId, dark);
	typeBButton.setColour(juce::TextButton::buttonOnColourId, dark);
	typeCButton.setColour(juce::TextButton::buttonOnColourId, dark);
	typeDButton.setColour(juce::TextButton::buttonOnColourId, dark);

	setSize((int)(SLIDER_WIDTH * 0.01f * SCALE * N_SLIDERS_COUNT), (int)((SLIDER_WIDTH + BOTTOM_MENU_HEIGHT) * 0.01f * SCALE));
}

BassEnhancerAudioProcessorEditor::~BassEnhancerAudioProcessorEditor()
{
}

void BassEnhancerAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(veryLight);
}

void BassEnhancerAudioProcessorEditor::resized()
{
	// Sliders + Menus
	int width = getWidth() / N_SLIDERS_COUNT;
	int height = SLIDER_WIDTH * 0.01f * SCALE;
	juce::Rectangle<int> rectangles[N_SLIDERS_COUNT];

	for (int i = 0; i < N_SLIDERS_COUNT; ++i)
	{
		rectangles[i].setSize(width, height);
		rectangles[i].setPosition(i * width, 0);
		m_sliders[i].setBounds(rectangles[i]);

		rectangles[i].removeFromBottom((int)(LABEL_OFFSET * 0.01f * SCALE));
		m_labels[i].setBounds(rectangles[i]);
	}

	// Buttons
	const int posY = height + (int)(BOTTOM_MENU_HEIGHT * 0.01f * SCALE * 0.25f);
	const int buttonHeight = (int)(BOTTOM_MENU_HEIGHT * 0.01f * SCALE * 0.5f);
	const int center = (int)(getWidth() * 0.5f);

	typeAButton.setBounds((int)(center - buttonHeight * 2.4f), posY, buttonHeight, buttonHeight);
	typeBButton.setBounds((int)(center - buttonHeight * 1.2f), posY, buttonHeight, buttonHeight);
	typeCButton.setBounds((int)(center + buttonHeight * 0.0f), posY, buttonHeight, buttonHeight);
	typeDButton.setBounds((int)(center + buttonHeight * 1.2f), posY, buttonHeight, buttonHeight);
}