#include "daisysp.h"
#include "daisy_seed.h"
#include "UI.h"
#include "Echosmpl.h"
#include "MapUI.h"

using namespace daisysp;
using namespace daisy;
//#define DSY_ADC_MAX_CHANNELS = 18;
DaisySeed hardware;

static DaisySeed  hw;
static Oscillator osc, osc2, lfo1;
static CrossFade osccross;
static Svf filter1;
static MidiUsbHandler midi;
static EchoSmpl echo;
static Phaser phas;
static MapUI mapUI[2];
static Overdrive overd;
static Adsr env;
static SampleRateReducer sred;

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
int subState = 1;

static Led led1, lfoLed, sin1Led, sin2Led, saw1Led, saw2Led, squ1Led, squ2Led, tri1Led, tri2Led;

bool Gate;

float lastFreq, env_out, lfo1Out, filLow, pitchBendVal;

float lfoSpeedVAL, lfotoFiltVAL, lfoToOscVAL, fx1VAL, fx2VAL, fx3VAL, fx4VAL, envtofiltVAL, attackVAL, RelVal, SusVal, det1VAL, det2VAL, pw1VAL, pw2VAL, oscmixVAL, cutoffVAL, resdriveVAL, filtresVAL;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig1,sig2, oscmix;
    for(size_t i = 0; i < size; i += 2)
    {
        //adsr
        env.SetAttackTime(attackVAL);
        env.SetReleaseTime(RelVal);
        env.SetSustainLevel(SusVal);
        env_out = env.Process(Gate);
        led1.Set(env_out);
        led1.Update();

        //lfo 
        lfo1.SetFreq(lfoSpeedVAL * 20);
        lfo1.SetAmp(1.0);
        lfo1Out = (lfo1.Process());

        //osc proccess
        float detuneAmount = pitchBendVal + (-50 + det1VAL);
        float detuneAmount2 = pitchBendVal + (-50 + det2VAL);
        osc.SetFreq(lastFreq + detuneAmount  +(lfo1Out * (lfoToOscVAL*50)));
        osc2.SetFreq(((lastFreq / subState) + detuneAmount2)+(lfo1Out * (lfoToOscVAL*50))); 
        osc.SetPw(pw1VAL);
        osc2.SetPw(pw2VAL);
        sig1 = osc.Process();
        sig2 = osc2.Process();
        osccross.SetPos(oscmixVAL);
        oscmix = osccross.Process(sig1, sig2);
        osc.SetAmp(env_out);
        osc2.SetAmp(env_out);

        //filter
        float lfoModAmount = ((lfo1Out * 5000) * lfotoFiltVAL) + ((env_out * envtofiltVAL) * 4000.0);
        filter1.SetFreq(cutoffVAL + lfoModAmount);
        filter1.SetRes(filtresVAL);
        filter1.Process(oscmix);
        filter1.SetDrive(resdriveVAL);
        filLow = filter1.Low();
        
        //echo
	    float knob1,knob2;
	    knob1 =  fx1VAL * 200.0;
   	    knob2 =  fx2VAL;
	    mapUI[0].setParamValue("Duration", knob1);
        mapUI[0].setParamValue("Feedback", knob2); 
	    float echoOut = echo.Process(filLow);
    
        //phaser
        float PhasAmount = fx3VAL;
        phas.SetLfoDepth(PhasAmount);
        float phaseOut = phas.Process(echoOut);

        //sample red
        sred.SetFreq(fx4VAL);
        float sampleRedOut = sred.Process(phaseOut);

        // left out
        out[i] = sampleRedOut * 0.5;

        // right out
        out[i + 1] = sampleRedOut* 0.5;
    }
}

void getAnalogInputs(){
    //multiplexer inputs
    lfoSpeedVAL = hardware.adc.GetMuxFloat(0,0);
    lfotoFiltVAL = hardware.adc.GetMuxFloat(0,1);
    lfoToOscVAL = hardware.adc.GetMuxFloat(0,2);
    fx1VAL = hardware.adc.GetMuxFloat(0,3);
    fx2VAL = hardware.adc.GetMuxFloat(0,4);
    fx3VAL = hardware.adc.GetMuxFloat(0,5);
    fx4VAL = hardware.adc.GetMuxFloat(0,6);
    envtofiltVAL = hardware.adc.GetMuxFloat(0,7);
    //adc inputs
    det1VAL = (hardware.adc.GetFloat(1) * 100);
    det2VAL = (hardware.adc.GetFloat(2) * 100);
    oscmixVAL = hardware.adc.GetFloat(3);
    pw1VAL =  hardware.adc.GetFloat(4);
    pw2VAL =  hardware.adc.GetFloat(5);
    cutoffVAL = (hardware.adc.GetFloat(6)*5000);
    filtresVAL = hardware.adc.GetFloat(7);
    resdriveVAL = hardware.adc.GetFloat(8);
    attackVAL = hardware.adc.GetFloat(9) * 5;
    SusVal = hardware.adc.GetFloat(10);
    RelVal = hardware.adc.GetFloat(11) * 2;
}

void bootDFU(){
    System::ResetToBootloader();
}

int main(void)
{
    
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();

    //adsr setup
    env.Init(sample_rate);
    env.SetTime(ADSR_SEG_ATTACK, .1);
    env.SetTime(ADSR_SEG_DECAY, 0.0);
    env.SetTime(ADSR_SEG_RELEASE, .01);
    env.SetSustainLevel(1);
    
    osc.Init(sample_rate);
    osc2.Init(sample_rate);
    lfo1.Init(sample_rate);
    echo.Init(hw.AudioSampleRate());
	echo.buildUserInterface(&mapUI[0]);
    phas.Init(sample_rate);
    phas.SetLfoDepth(1.f);
    phas.SetFreq(2000.f);
    sred.Init();

    //setup led outputs
    led1.Init(hardware.GetPin(9), false);
    lfoLed.Init(hardware.GetPin(10), false);
    sin1Led.Init(seed::D1, false);
    saw1Led.Init(seed::D2, false);
    squ1Led.Init(seed::D3, false);
    tri1Led.Init(seed::D4, false);
    sin2Led.Init(seed::D5, false);
    saw2Led.Init(seed::D6, false);
    squ2Led.Init(seed::D7, false);
    tri2Led.Init(seed::D8, false);

    AdcChannelConfig adcConfig[12];
    adcConfig[0].InitMux(seed::A0, 8,  seed::D11, seed::D12, seed::D13);
    adcConfig[1].InitSingle (seed::A1); //osc 1 detune
    adcConfig[2].InitSingle (seed::A2); //osc 2 detune
    adcConfig[3].InitSingle (seed::A3); //osc mix
    adcConfig[4].InitSingle (seed::A4); //pw1
    adcConfig[5].InitSingle (seed::A5); //pw2
    adcConfig[6].InitSingle (seed::A6); //filter cutoff
    adcConfig[7].InitSingle (seed::A7); //filter res
    adcConfig[8].InitSingle (seed::A8); //filter drive 
    adcConfig[9].InitSingle (seed::A9); //attack
    adcConfig[10].InitSingle (seed::A10); //sustain
    adcConfig[11].InitSingle (seed::A11); //release
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
    Switch subState1;
    oscwave1.Init(hardware.GetPin(26), 1000);
    oscwave2.Init(hardware.GetPin(27), 1000);
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
        getAnalogInputs();
        
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

        //holding down all buttons boots into dfu mode
        if(oscwave1.RisingEdge()){
            if(oscwave2.RisingEdge()){
                if(subState1.RisingEdge()){
                    bootDFU();
                }
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
        while(midi.HasEvents())
        {
            auto msg = midi.PopEvent();
            switch(msg.type)
            {
                case NoteOn:
                {
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
                case PitchBend:
                {
                    auto pitch_msg = msg.AsPitchBend();
                    pitchBendVal = pitch_msg.value;
                    break;
                }
                default: break;
            }
        }
    }
}