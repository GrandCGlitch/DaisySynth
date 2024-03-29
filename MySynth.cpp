#include "daisysp.h"
#include "daisy_seed.h"
#include "moogladder.h"

using namespace daisysp;
using namespace daisy;

DaisySeed hardware;

static DaisySeed  hw;
static Oscillator osc1, osc2;
static MoogLadder flt;
static CrossFade osccross;

float  saw1, saw2 , freq, res;

//12 bit max
float potMax = 65535.0;

//pick input pins
int detunePin1 = 18;
int detunePin2 = 19;
int filtfreqPin = 20;
int filtResPin = 21;


static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig, oscmix;
    for(size_t i = 0; i < size; i += 2)
    {
        //setup filter
        freq = ((5000.0 * (hardware.adc.GetFloat(2) / potMax)) + 5000.0);
        res = (hardware.adc.GetFloat(3) / potMax);
        flt.SetFreq(freq);
        flt.SetRes(res);

        //setup osc detuning
        osc1.SetFreq((440.0 * (hardware.adc.GetFloat(0) / potMax)) + 440.0);
        osc2.SetFreq((440.0 * (hardware.adc.GetFloat(1) / potMax)) + 440.0);

        //run oscillators and mix between them 
        saw1 = osc1.Process();
        saw2 = osc2.Process();
        osccross.SetPos(0.5);
        oscmix = osccross.Process(saw1, saw2);

        //run mixed oscillators through filter
        sig = flt.Process(oscmix);

        //output sound
        out[i] = sig;
        out[i + 1] = sig;
    }
}

int main(void)
{

    //setup hardware
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc1.Init(sample_rate);
    osc2.Init(sample_rate);
    flt.Init(sample_rate);

    flt.SetRes(0.7);

    //setup pot inputs
    AdcChannelConfig adcConfig[4];
    adcConfig[0].InitSingle(hardware.GetPin(detunePin1));
    adcConfig[1].InitSingle(hardware.GetPin(detunePin2));
    adcConfig[2].InitSingle(hardware.GetPin(filtfreqPin));
    adcConfig[3].InitSingle(hardware.GetPin(filtResPin));
    hardware.adc.Init(adcConfig, 4);
    hardware.adc.Start();

    hardware.Configure();
    hardware.Init();

    //setup osc 1
    osc1.SetWaveform(osc1.WAVE_SAW);
    osc1.SetFreq((440.0 * (hardware.adc.GetFloat(0) / potMax)) + 440.0);
    osc1.SetAmp(0.5);

    //setup osc 2
    osc2.SetWaveform(osc2.WAVE_SAW);
    osc2.SetFreq((440.0 * (hardware.adc.GetFloat(1) / potMax)) + 440.0);
    osc2.SetAmp(0.5);

    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {
    }
}
