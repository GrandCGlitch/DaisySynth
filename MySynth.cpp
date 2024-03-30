#include "daisysp.h"
#include "daisy_seed.h"
#include "UI.h"
#include "Echosmpl.h"
#include "MapUI.h"

using namespace daisysp;
using namespace daisy;
DaisySeed hardware;

static DaisySeed  hw;
static Oscillator osc, osc2, lfo1;
static CrossFade osccross;
static Svf filter1;
static AdEnv ad1;
static MidiUsbHandler midi;
MapUI mapUI[2];
static EchoSmpl echo;

//pick input pins
int detunePin1 = 18;
int FreqMod = 19;
int filtcut = 20;
int filtres = 21;
int attPin = 22;
int decPin = 23;
int durKnob = 24;
int feedKnob = 25;
int lfoSpeed = 15;
int lfoDepth = 16;

int osc1state = 1;
int osc2state = 1;

static Led led1;
static Led lfoLed;

float lastFreq;

float env_out;
float sig2;
float lfo1Out;
static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig1, oscmix;
    for(size_t i = 0; i < size; i += 2)
    {
        float vibroAmount = hardware.adc.GetFloat(1);
        float detuneAmount = hardware.adc.GetFloat(0) * 440;
        osc.SetFreq(lastFreq  + detuneAmount + ((lfo1Out * 100)* vibroAmount));
        osc2.SetFreq(lastFreq + detuneAmount + ((lfo1Out * 100) * vibroAmount));
        sig1 = osc.Process();
        sig2 = osc2.Process();

        osccross.SetPos(0.5);
        oscmix = osccross.Process(sig1, sig2);

        ad1.SetTime(ADENV_SEG_ATTACK, hardware.adc.GetFloat(4));
        ad1.SetTime(ADENV_SEG_DECAY, hardware.adc.GetFloat(5));

        lfo1.SetFreq(hardware.adc.GetFloat(8) * 20);
        lfo1.SetAmp(1.0);
        lfo1Out = (lfo1.Process());
        lfoLed.Set(lfo1Out);
        lfoLed.Update();
        filter1.SetFreq((hardware.adc.GetFloat(2)*5000) + (lfo1Out* (hardware.adc.GetFloat(9)*5000)));
        filter1.SetRes(hardware.adc.GetFloat(3));
        filter1.Process(oscmix);

        env_out = ad1.Process();
        osc.SetAmp(env_out);
        osc2.SetAmp(env_out);
        led1.Set(env_out);
        led1.Update();

	    float knob1,knob2;
	    knob1 =  hardware.adc.GetFloat(6) * 200.0;
   	    knob2 =  hardware.adc.GetFloat(7);
	    mapUI[0].setParamValue("Duration", knob1);
        mapUI[0].setParamValue("Feedback", knob2); 
        float filter_out = filter1.Low();
	    float finalout = echo.Process(filter_out);
        
        // left out
        out[i] = finalout * 0.5;

        // right out
        //out[i + 1] = filter1.Low();
    }
}

int main(void)
{
    

    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(64);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);
    osc2.Init(sample_rate);
    lfo1.Init(sample_rate);
    ad1.Init(sample_rate);

    echo.Init(hw.AudioSampleRate());
	echo.buildUserInterface(&mapUI[0]);

    //setup envelope
    ad1.SetTime(ADENV_SEG_ATTACK, 0.15);
    ad1.SetTime(ADENV_SEG_DECAY, 0.35);
    ad1.SetMin(0.0);
    ad1.SetMax(1.0);
    ad1.SetCurve(0); // linear

    
    led1.Init(hardware.GetPin(28), false);
    led1.Set(0.0);
    led1.Update();

    lfoLed.Init(hardware.GetPin(0), false);



    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);    

    AdcChannelConfig adcConfig[10];
    adcConfig[0].InitSingle (hardware.GetPin(detunePin1));
    adcConfig[1].InitSingle (hardware.GetPin(FreqMod));
    adcConfig[2].InitSingle (hardware.GetPin(filtcut));
    adcConfig[3].InitSingle (hardware.GetPin(filtres));
    adcConfig[4].InitSingle (hardware.GetPin(attPin));
    adcConfig[5].InitSingle (hardware.GetPin(decPin));
    adcConfig[6].InitSingle (hardware.GetPin(durKnob));
    adcConfig[7].InitSingle (hardware.GetPin(feedKnob));
    adcConfig[8].InitSingle (hardware.GetPin(lfoSpeed));
    adcConfig[9].InitSingle (hardware.GetPin(lfoDepth));
    hardware.adc.Init(adcConfig, 10);
    hardware.adc.Start();

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SAW);
    osc.SetFreq(440);
    osc.SetAmp(0.0);

    osc2.SetWaveform(osc.WAVE_SAW);
    osc2.SetFreq(440);
    osc2.SetAmp(0.0);

    lfo1.SetWaveform(osc.WAVE_TRI);
    

    //setup filter
    filter1.Init(sample_rate);
    filter1.SetFreq(500.0);
    filter1.SetRes(0.85);
    filter1.SetDrive(0.8);

    Switch oscwave1;
    oscwave1.Init(hardware.GetPin(29), 1000);

    Switch oscwave2;
    oscwave2.Init(hardware.GetPin(30), 1000);


    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {
        oscwave1.Debounce();
        oscwave2.Debounce();
        if(oscwave1.RisingEdge()){
            if(osc1state < 4){
            osc1state ++;
            }else{
                osc1state = 1;
            }
            switch(osc1state)
            {
                case 1:
                    osc.SetWaveform(osc.WAVE_SIN);
                    break;
                
                case 2:
                    osc.SetWaveform(osc.WAVE_SAW);
                    break;
                
                case 3:
                    osc.SetWaveform(osc.WAVE_SQUARE);
                    break;
                
                case 4:
                    osc.SetWaveform(osc.WAVE_TRI);
                    break;

            }
            System::Delay(1);
        }

        if(oscwave2.RisingEdge()){
            if(osc2state < 4){
            osc2state ++;
            }else{
                osc2state = 1;
            }
            switch(osc2state)
            {
                case 1:
                    osc2.SetWaveform(osc.WAVE_SIN);
                    break;
                
                case 2:
                    osc2.SetWaveform(osc.WAVE_SAW);
                    break;
                
                case 3:
                    osc2.SetWaveform(osc.WAVE_SQUARE);
                    break;
                
                case 4:
                    osc2.SetWaveform(osc.WAVE_TRI);
                    break;

            }
            System::Delay(1);
        }

        midi.Listen();

        /** When there are messages waiting in the queue... */
        while(midi.HasEvents())
        {
            /** Pull the oldest one from the list... */
            auto msg = midi.PopEvent();
            switch(msg.type)
            {
                case NoteOn:
                {
                    ad1.Trigger();
                    auto note_msg = msg.AsNoteOn();
                    if(note_msg.velocity != 0){
                        lastFreq = mtof(note_msg.note);
                        
                    }
                    if(note_msg.velocity == 0){
                        
                    }
                }
                break;
                default: break;
            }
        }
    }
}
