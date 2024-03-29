#include "daisysp.h"
#include "daisy_seed.h"
#include "moogladder.h"

using namespace daisysp;
using namespace daisy;

DaisySeed hardware;

static DaisySeed  hw;
static Oscillator osc;
static MoogLadder flt;

float  saw, freq, res;
float potMax = 65535.0;

int detunePin = 19;
int filtfreqPin = 20;
int filtResPin = 21;


static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;
    for(size_t i = 0; i < size; i += 2)
    {
        freq = ((5000.0 * (hardware.adc.GetFloat(1) / potMax)) + 5000.0);
        res = (hardware.adc.GetFloat(2) / potMax);
        flt.SetFreq(freq);
        flt.SetRes(res);

        osc.SetFreq((440.0 * (hardware.adc.GetFloat(0) / potMax)) + 440.0);
        
        saw = osc.Process();
        sig = flt.Process(saw);

        out[i] = sig;
        out[i + 1] = sig;
    }
}

int main(void)
{

    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);
    flt.Init(sample_rate);
    flt.SetRes(0.7);


    AdcChannelConfig adcConfig[3];
    adcConfig[0].InitSingle(hardware.GetPin(detunePin));
    adcConfig[1].InitSingle(hardware.GetPin(filtfreqPin));
    adcConfig[2].InitSingle(hardware.GetPin(filtResPin));
    hardware.adc.Init(adcConfig, 3);
    hardware.adc.Start();

    hardware.Configure();
    hardware.Init();


    osc.SetWaveform(osc.WAVE_SAW);
    osc.SetFreq((440.0 * (hardware.adc.GetFloat(0) / potMax)) + 440.0);
    osc.SetAmp(0.5);

    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {
    }
}
