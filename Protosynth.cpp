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
static CrossFade phasMix;
static CrossFade filterMixCross;
static Svf filter1;
static AdEnv ad1;
static MidiUsbHandler midi;
static EchoSmpl echo;
static Phaser phas;
static MapUI mapUI[2];
static Overdrive overd;


//pick input pins
//Osc Sec
int detPin1 = 16;
int detPin2 = 17;
int oscMixPin = 18;
int pwKnob1 = 19;
int pwKnob2 = 20;
//Filt Knobs
int filtcut = 21;
int filtres = 22;
int filterMix = 23;

//ADSR
int attPin = 24;
int decPin = 25;
int adToFilt = 28;

//LFO
int lfoSpeedPin = 0;
int lfotoosc = 1;
int lfoToFilt = 2;

//Fx
int echoDur = 3;
int echoFeed = 4;
int phasePin = 5;
int overdKnob = 6;

// osc waveform selector
int osc1state = 1;
int osc2state = 1;
int sin1 =1;
int saw1 =2;
int squ1 =3;
int tri1 =4;
int sin2 =5;
int saw2 =6;
int squ2 =7;
int tri2 =8;
int subState = 1;

static Led led1;
static Led lfoLed;
static Led sin1Led, sin2Led, saw1Led, saw2Led, squ1Led, squ2Led, tri1Led, tri2Led;

bool Gate;

float lastFreq, env_out, lfo1Out, filLow, filHigh, filtMix;



static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig1,sig2, oscmix;
    for(size_t i = 0; i < size; i += 2)
    {

        //lfo process
        lfo1.SetFreq(hardware.adc.GetMux(0, 0));
        lfo1.SetAmp(1.0);
        lfo1Out = (lfo1.Process());
        lfoLed.Set(lfo1Out);
        lfoLed.Update();

        //osc proccess
        float detuneAmount = -50 + (hardware.adc.GetFloat(1) * 100);
        float detuneAmount2 = -50 + (hardware.adc.GetFloat(2) * 100);
        osc.SetFreq(lastFreq + detuneAmount + (lfo1Out * hardware.adc.GetMux(0,1)));
        osc2.SetFreq((lastFreq / subState) + detuneAmount2 + (lfo1Out * hardware.adc.GetMux(0,1)));
        sig1 = osc.Process();
        sig2 = osc2.Process();
        osccross.SetPos(0.5);
        oscmix = osccross.Process(sig1, sig2);

        //adsr
        ad1.SetTime(ADENV_SEG_ATTACK, hardware.adc.GetFloat(9));
        ad1.SetTime(ADENV_SEG_DECAY, hardware.adc.GetFloat(10));
        env_out = ad1.Process();
        osc.SetAmp(env_out);
        osc2.SetAmp(env_out);
        led1.Set(env_out);
        led1.Update();

        //filter high low
        filter1.SetFreq((hardware.adc.GetFloat(6)*5000) + ((env_out * 5000) * hardware.adc.GetFloat(11)) + ((lfo1Out * 5000)* hardware.adc.GetMux(0,2)));
        filter1.SetRes(hardware.adc.GetFloat(7));
        filter1.Process(oscmix);
        filLow = filter1.Low();
        filHigh = filter1.High();
        filterMixCross.SetPos(hardware.adc.GetFloat(8));
        filterMix = filterMixCross.Process(filLow, filHigh);
        
        //echo code
	    float knob1,knob2;
	    knob1 =  hardware.adc.GetMux(0, 3) * 200.0;
   	    knob2 =  hardware.adc.GetMux(0, 4);
	    mapUI[0].setParamValue("Duration", knob1);
        mapUI[0].setParamValue("Feedback", knob2); 
        float filter_out = filter1.Low();
	    float finalout = echo.Process(filter_out);
    
        //phaser
        float PhasAmount = hardware.adc.GetMux(0, 5);
        phas.SetLfoDepth(PhasAmount);
        float phaseOut = phas.Process(finalout);

        //overdrive
        overd.SetDrive(hardware.adc.GetMux(0,6));
        float drive_out = overd.Process(phaseOut);

        // left out
        out[i] = drive_out * 0.5;

        // right out
        out[i + 1] = drive_out * 0.5;
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
    phas.Init(sample_rate);
    phas.SetLfoDepth(1.f);
    phas.SetFreq(2000.f);
    overd.Init();

    //setup envelope
    ad1.SetTime(ADENV_SEG_ATTACK, 0.15);
    ad1.SetTime(ADENV_SEG_DECAY, 0.35);
    ad1.SetMin(0.0);
    ad1.SetMax(2.0);
    ad1.SetCurve(0); // linear

    //setup led outputs
    led1.Init(hardware.GetPin(9), false);
    lfoLed.Init(hardware.GetPin(10), false);

    sin1Led.Init(hardware.GetPin(1), false);
    saw1Led.Init(hardware.GetPin(2), false);
    squ1Led.Init(hardware.GetPin(3), false);
    tri1Led.Init(hardware.GetPin(4), false);
    sin2Led.Init(hardware.GetPin(5), false);
    saw2Led.Init(hardware.GetPin(6), false);
    squ2Led.Init(hardware.GetPin(7), false);
    tri2Led.Init(hardware.GetPin(8), false);

    AdcChannelConfig adcConfig[12];
    adcConfig[0].InitMux(seed::A0, 8,  seed::D11, seed::D12, seed::D13);
    // oscPins
    adcConfig[1].InitSingle (hardware.GetPin(detPin1));
    adcConfig[2].InitSingle (hardware.GetPin(detPin2));
    adcConfig[3].InitSingle (hardware.GetPin(oscMixPin));
    adcConfig[4].InitSingle (hardware.GetPin(pwKnob1));
    adcConfig[5].InitSingle (hardware.GetPin(pwKnob2));
    //filter pins
    adcConfig[6].InitSingle (hardware.GetPin(filtcut));
    adcConfig[7].InitSingle (hardware.GetPin(filtres));
    adcConfig[8].InitSingle (hardware.GetPin(filterMix));
    //adsr pins
    adcConfig[9].InitSingle (hardware.GetPin(attPin));
    adcConfig[10].InitSingle (hardware.GetPin(decPin));
    adcConfig[11].InitSingle (hardware.GetPin(adToFilt));
    hardware.adc.Init(adcConfig, 12);
    hardware.adc.Start();


    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0.0);
    osc2.SetWaveform(osc.WAVE_SIN);
    osc2.SetFreq(440);
    osc2.SetAmp(0.0);
    lfo1.SetWaveform(osc.WAVE_TRI);
    
    //setup filter
    filter1.Init(sample_rate);
    filter1.SetFreq(500.0);
    filter1.SetRes(0.85);
    filter1.SetDrive(0.8);

    //osc selection switches
    Switch oscwave1;
    Switch oscwave2;
    oscwave1.Init(hardware.GetPin(26), 1000);
    oscwave2.Init(hardware.GetPin(27), 1000);
    Switch subState1;
    subState1.Init(hardware.GetPin(29), 1000);

    //set init wave leds
    sin1Led.Set(1);
    sin2Led.Set(1);
    sin1Led.Update();
    sin2Led.Update();

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);    

    // start callback
    hw.StartAudio(AudioCallback);

    

    while(1) {
        oscwave1.Debounce();
        oscwave2.Debounce();
        subState1.Debounce();
        if(subState1.RisingEdge()){
            switch(subState){
                case 1:
                    subState = 2;
                    break;
                
                case 2:
                    subState = 3;
                    break;
                
                case 3:
                    subState = 1;
                    break;
            }
        }
        
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
                    sin1Led.Set(1);
                    saw1Led.Set(0);
                    squ1Led.Set(0);
                    tri1Led.Set(0);
                    sin1Led.Update();
                    saw1Led.Update();
                    squ1Led.Update();
                    tri1Led.Update();
                    break;
                
                case 2:
                    osc.SetWaveform(osc.WAVE_SAW);
                    sin1Led.Set(0);
                    saw1Led.Set(1);
                    squ1Led.Set(0);
                    tri1Led.Set(0);
                    sin1Led.Update();
                    saw1Led.Update();
                    squ1Led.Update();
                    tri1Led.Update();
                    break;
                
                case 3:
                    osc.SetWaveform(osc.WAVE_SQUARE);
                    sin1Led.Set(0);
                    saw1Led.Set(0);
                    squ1Led.Set(1);
                    tri1Led.Set(0);
                    sin1Led.Update();
                    saw1Led.Update();
                    squ1Led.Update();
                    tri1Led.Update();
                    break;
                
                case 4:
                    osc.SetWaveform(osc.WAVE_TRI);
                    sin1Led.Set(0);
                    saw1Led.Set(0);
                    squ1Led.Set(0);
                    tri1Led.Set(1);
                    sin1Led.Update();
                    saw1Led.Update();
                    squ1Led.Update();
                    tri1Led.Update();
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
                    sin2Led.Set(1);
                    saw2Led.Set(0);
                    squ2Led.Set(0);
                    tri2Led.Set(0);
                    sin2Led.Update();
                    saw2Led.Update();
                    squ2Led.Update();
                    tri2Led.Update();
                    break;
                
                case 2:
                    osc2.SetWaveform(osc.WAVE_SAW);
                    sin2Led.Set(0);
                    saw2Led.Set(1);
                    squ2Led.Set(0);
                    tri2Led.Set(0);
                    sin2Led.Update();
                    saw2Led.Update();
                    squ2Led.Update();
                    tri2Led.Update();
                    break;
                
                case 3:
                    osc2.SetWaveform(osc.WAVE_SQUARE);
                    sin2Led.Set(0);
                    saw2Led.Set(0);
                    squ2Led.Set(1);
                    tri2Led.Set(0);
                    sin2Led.Update();
                    saw2Led.Update();
                    squ2Led.Update();
                    tri2Led.Update();
                    break;
                
                case 4:
                    osc2.SetWaveform(osc.WAVE_TRI);
                    sin2Led.Set(0);
                    saw2Led.Set(0);
                    squ2Led.Set(0);
                    tri2Led.Set(1);
                    sin2Led.Update();
                    saw2Led.Update();
                    squ2Led.Update();
                    tri2Led.Update();
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
                        lfo1.Reset();
                        Gate = true;
                    }
                    break;
                }
                case NoteOff:
                {
                    Gate = false;
                    break;
                }
                default: break;
            }
        }
    }
}