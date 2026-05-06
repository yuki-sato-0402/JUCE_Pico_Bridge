#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"

CustomAudioProcessor::CustomAudioProcessor() 
: AudioProcessor (BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::createLCR(), true)),
    myVisualiser(1),
    parameters(*this, nullptr, juce::Identifier("PARAMETERS"),
    juce::AudioProcessorValueTreeState::ParameterLayout {
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "terms",  1}, "terms",
        juce::NormalisableRange<float>(1, 40, 1, 1), 1),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "filterOnOff",  1}, "filterOnOff",
        juce::NormalisableRange<float>(0, 1, 1, 1), 0),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "cutoffOvertone",  1}, "cutoffOvertone",
        juce::NormalisableRange<float>(2, 40, 1, 1), 2),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attenuation",  1}, "attenuation",
        juce::NormalisableRange<float>(1.f, 100.f, 0.01f, 1.f),1.f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "oscillator",  1}, "oscillator",
        juce::NormalisableRange<float>(1, 3, 1, 1), 1),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "attack",  1}, "attack",
        juce::NormalisableRange<float>(1.f, 1000.f, 0.01f, 1.f), 10.f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "decay",  1}, "decay",
        juce::NormalisableRange<float>(1.f, 1000.f, 0.01f, 1.f), 100.f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "sustain",  1}, "sustain",
        juce::NormalisableRange<float>(0.f, 100.f, 0.01f, 1.f), 80.f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "release",  1}, "release",
        juce::NormalisableRange<float>(0.f, 5000.f, 0.01f, 1.f),1000.f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "amp",  1}, "amp",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f),0.5f),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "PosNeg",  1}, "PosNeg",
        juce::NormalisableRange<float>(0, 1, 1), 1), 
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "PosNegSync",  1}, "PosNegSync",
        juce::NormalisableRange<float>(0, 1, 1), 0),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "cycleCountToAdd",  1}, "cycleCountToAdd",
        juce::NormalisableRange<float>(0, 20, 1), 1), 
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "cycleCountToSubtract",  1}, "cycleCountToSubtract",
        juce::NormalisableRange<float>(0, 20, 1), 1),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "termsToAddPerCount",  1}, "termsToAddPerCount",
        juce::NormalisableRange<float>(1, 20, 1), 1),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "harmonicSeriesMode",  1}, "harmonicSeriesMode",
        juce::NormalisableRange<float>(0, 1, 1), 0),
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "harmonicRatio",  1}, "harmonicRatio",
        juce::NormalisableRange<float>(0.1f, 2.f, 0.01f), 1.f),
    }
  )
{
 
 
  for (RNBO::ParameterIndex i = 0; i < rnboObject.getNumParameters(); ++i){
    RNBO::ParameterInfo info;
    rnboObject.getParameterInfo (i, &info);

    if (info.visible){
      auto paramID = juce::String (rnboObject.getParameterId (i));
      std::cout << "Parameter Index: " << i << std::endl;
      std::cout << "Min Value: " << info.min << std::endl;
      std::cout << "Max Value: " << info.max << std::endl;

      // Each apvts parameter id and range must be the same as the rnbo param object's.
      // If you hit this assertion then you need to fix the incorrect id in ParamIDs.h.
      jassert (parameters.getParameter (paramID) != nullptr);

      // If you hit these assertions then you need to fix the incorrect apvts
      // parameter range in createParameterLayout().
      jassert (std::abs (info.min - parameters.getParameterRange (paramID).start) < 1e-5f);
      jassert (std::abs (info.max - parameters.getParameterRange (paramID).end) < 1e-5f);
      apvtsParamIdToRnboParamIndex[paramID] = i;
    

    parameters.addParameterListener(paramID, this);
    rnboObject.setParameterValue(i, parameters.getRawParameterValue(paramID)->load());  // RNBO に適用
      
    } 
  }

  myVisualiser.setBufferSize(512);
  myVisualiser.setRepaintRate(30);

  setupMidiCCMappings();
}

void CustomAudioProcessor::setupMidiCCMappings()
{
    // 自作MIDIコントローラーに合わせてここを調整してください
    ccToParameterID[20] = "harmonicSeriesMode";
    ccToParameterID[21] = "harmonicRatio";
    ccToParameterID[22] = "oscillator";
    ccToParameterID[23] = "terms";
    ccToParameterID[24] = "filterOnOff";
    ccToParameterID[25] = "cutoffOvertone";
    ccToParameterID[26] = "attenuation";
    ccToParameterID[27]  = "PosNegSync"; 
    ccToParameterID[28]  = "PosNeg"; 
    ccToParameterID[29]  = "cycleCountToAdd"; 
    ccToParameterID[30]  = "cycleCountToSubtract";
    ccToParameterID[70]  = "termsToAddPerCount";
    ccToParameterID[71]  = "amp";
    ccToParameterID[72]  = "attack";
    ccToParameterID[73]  = "decay";
    ccToParameterID[74]  = "sustain";
    ccToParameterID[75]  = "release";
}

juce::String CustomAudioProcessor::getParameterDisplayValue(const juce::String& parameterID, float value)
{
    if (parameterID == "harmonicSeriesMode") {
        return (value == 0) ? "Linear Mode" : "Exponential";
    } else if (parameterID == "oscillator") {
        if (value == 1) return "Square";
        if (value == 2) return "Triangle";
        if (value == 3) return "Sawtooth";
    } else if (parameterID == "filterOnOff") {
        return (value == 0) ? "Off" : "On";
    } else if (parameterID == "PosNeg") {
        return (value == 0) ? "Positive" : "Negative";
    } else if (parameterID == "PosNegSync") {
        return (value == 0) ? "Off" : "On";
    } else if (parameterID == "terms" || parameterID == "cutoffOvertone" || parameterID == "cycleCountToAdd" || parameterID == "cycleCountToSubtract" || parameterID == "termsToAddPerCount") {
        return juce::String((int)value);
    }
    
    return juce::String(value, 2);
}

void CustomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rnboObject.prepareToProcess (sampleRate, static_cast<size_t> (samplesPerBlock));
    myVisualiser.clear();

    auto devices = juce::MidiOutput::getAvailableDevices();
    for (const auto& device : devices) {
        std::cout << "MIDI Output Device: " << device.name << " (ID: " << device.identifier << ")" << std::endl;
    }
    if (!devices.isEmpty()) {
        auto deviceInfo = devices[2]; // 最初のMIDI出力デバイスを選択
        midiOutputDevice = juce::MidiOutput::openDevice (deviceInfo.identifier);
        if (midiOutputDevice) {
            std::cout << "Opened MIDI Output Device: " << deviceInfo.name << std::endl;

            for (RNBO::ParameterIndex i = 0; i < rnboObject.getNumParameters(); ++i) {
                RNBO::ParameterInfo info;
                rnboObject.getParameterInfo (i, &info);
                if (info.visible) {
                    auto paramID = juce::String(rnboObject.getParameterId(i));

                    float initialValue = parameters.getRawParameterValue(paramID)->load();
                    
                    // 数値ではなく表示用の文字列を取得
                    juce::String initialValueStr = getParameterDisplayValue(paramID, initialValue);
                    
                    juce::String rangeMsg = paramID + "," + initialValueStr + "," + juce::String(int(info.max) - int(info.min) + 1);
                    createMessage(rangeMsg);
                }
            }
        } else {
            std::cout << "Failed to open MIDI Output Device: " << deviceInfo.name << std::endl;
        }
    } else {
        std::cout << "No MIDI Output Devices Available" << std::endl;
    }


}
 
void CustomAudioProcessor::releaseResources()
{
}
 

void CustomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto tc = preProcess(midiMessages);

    //std::cout << "Num Channels: " << buffer.getNumChannels() << std::endl;

    rnboObject.process(
        buffer.getArrayOfWritePointers(), static_cast<RNBO::Index>(buffer.getNumChannels()),
        buffer.getArrayOfWritePointers(), static_cast<RNBO::Index>(buffer.getNumChannels()),
        static_cast<RNBO::Index> (buffer.getNumSamples()),
        &_midiInput, &_midiOutput
    );

    // Copy Channel 3 (Index 2) for the visualizer, then delete it.
    const int visChannelIndex = 2;
    const int numSamples = buffer.getNumSamples();

   
    if (buffer.getNumChannels() > visChannelIndex) {
        // Create a single-channel buffer for the visualizer
        juce::AudioBuffer<float> visualBuf(1, numSamples);
        float* visWrite = visualBuf.getWritePointer(0);
        const float* src = buffer.getReadPointer(visChannelIndex);

        // Copy data from the original buffer to the visualizer buffer
        std::memcpy(visWrite, src, sizeof(float) * (size_t)numSamples);

        buffer.clear(visChannelIndex, 0, numSamples);

        const float* channelsForVis[1] = { visualBuf.getReadPointer(0) };
        myVisualiser.pushBuffer(channelsForVis, 1, numSamples);
    }

    postProcess(tc, midiMessages);
}

void CustomAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //std::cout << "Parameter Changed: " << parameterID << " New Value: " << newValue << std::endl;
    rnboObject.setParameterValue (apvtsParamIdToRnboParamIndex[parameterID], newValue);
    createMessage(getParameterDisplayValue(parameterID, newValue));
}

void CustomAudioProcessor::createMessage(juce::String newValue)
{
    juce::MemoryBlock data;
    uint8_t manufacturerId = 0x7D;
    data.append (&manufacturerId, 1);
    data.append (newValue.toRawUTF8(), newValue.getNumBytesAsUTF8());

    auto msg = juce::MidiMessage::createSysExMessage (data.getData(), (int) data.getSize());

    std::cout << "Sending MIDI SysEx Message (Value Only): " << newValue << std::endl;
    if (midiOutputDevice != nullptr) {
        midiOutputDevice->sendMessageNow (msg);
    }
}



RNBO::TimeConverter CustomAudioProcessor::preProcess(juce::MidiBuffer& midiMessages) {
	RNBO::MillisecondTime time = rnboObject.getCurrentTime();
    RNBO::TimeConverter timeConverter(rnboObject.getSampleRate(), time);

	// fill midi input 
	_midiInput.clear();  // make sure midi input starts clear
	for (auto meta: midiMessages)
	{
        auto msg = juce::MidiMessage (meta.data, meta.numBytes, meta.samplePosition);
        
        if (msg.isController())
        {
            int ccNumber = msg.getControllerNumber();
            if (ccToParameterID.count(ccNumber) > 0)
            {
                juce::String paramID = ccToParameterID[ccNumber];
                float newValue = msg.getControllerValue() / 127.0f;
                
                if (auto* param = parameters.getParameter(paramID))
                {
                    // パラメータを更新。これによりUIとRNBOエンジン両方に通知されます。
                    param->setValueNotifyingHost(newValue);
                }
            }
        }

        RNBO::MillisecondTime t = timeConverter.convertSampleOffsetToMilliseconds(meta.samplePosition);
		_midiInput.addEvent(RNBO::MidiEvent(t, 0, meta.data, (RNBO::Index)meta.numBytes));
	}
	midiMessages.clear();		// clear the input that we consumed above so juce doesn't get confused
	return timeConverter;
}

void CustomAudioProcessor::postProcess(RNBO::TimeConverter& timeConverter, juce::MidiBuffer& midiMessages) {
	// consume midi output
	if (!_midiOutput.empty()) {
		for (const auto& ev: _midiOutput) {
			int sampleNumber = static_cast<int>(timeConverter.convertMillisecondsToSampleOffset(ev.getTime()));
			auto midiMessage = MidiMessage(ev.getData(), (int)ev.getLength());
			midiMessages.addEvent(midiMessage, sampleNumber);
		}
		_midiOutput.clear();
	}
}

juce::AudioProcessorEditor* CustomAudioProcessor::createEditor()
{
   return new CustomAudioEditor (*this,  parameters);
}

bool CustomAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}


const juce::String CustomAudioProcessor::getName() const{
    return "RNBO_FourierSeriesExpansionSynthesizer";
}

bool CustomAudioProcessor::acceptsMidi() const
{
    return true;
}

bool CustomAudioProcessor::producesMidi() const 
{
    return false;
}   
 
bool CustomAudioProcessor::isMidiEffect() const
{
    return false;
}

double CustomAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}
 
int CustomAudioProcessor::getNumPrograms()
{
    return 1;   
}
 
int CustomAudioProcessor::getCurrentProgram()
{
    return 0;
}
 
void CustomAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}
 
const juce::String CustomAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}
 
void CustomAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}


void CustomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CustomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}


