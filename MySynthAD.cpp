#include "daisysp.h"
#include "daisy_seed.h"
#include "moogladder.h"

using namespace daisysp;
using namespace daisy;
DaisySeed hardware;

static DaisySeed  hw;
static Oscillator osc, osc2;
static CrossFade osccross;
static Svf filter1;
static AdEnv ad1;
static MidiUsbHandler midi;

float envout;
//pick input pins
int detunePin1 = 18;
int detunePin2 = 19;
int filtcut = 20;
int filtres = 21;

float env_out;

bool gate;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig1, sig2, oscmix;
    for(size_t i = 0; i < size; i += 2)
    {
        sig1 = osc.Process();
        sig2 = osc2.Process();

        osccross.SetPos(0.5);
        oscmix = osccross.Process(sig1, sig2);

        env_out = ad1.Process();
        osc.SetAmp(env_out);
        osc2.SetAmp(env_out);

        filter1.SetFreq((hardware.adc.GetFloat(2)*5000));
;       filter1.SetRes(hardware.adc.GetFloat(3));
        filter1.Process(oscmix);
        // left out
        out[i] = filter1.Low();

        // right out
        out[i + 1] = filter1.Low();
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
    osc2.Init(sample_rate);
    ad1.Init(sample_rate);

    //setup envelope
    ad1.SetTime(ADENV_SEG_ATTACK, 0.15);
    ad1.SetTime(ADENV_SEG_DECAY, 0.35);
    ad1.SetMin(0.0);
    ad1.SetMax(1.0);
    ad1.SetCurve(0); // linear

    Led led1;
    led1.Init(hardware.GetPin(28), false);
    led1.Set(0.0);
    led1.Update();


    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);    

    AdcChannelConfig adcConfig[4];
    adcConfig[0].InitSingle (hardware.GetPin(detunePin1));
    adcConfig[1].InitSingle (hardware.GetPin(detunePin2));
    adcConfig[2].InitSingle (hardware.GetPin(filtcut));
    adcConfig[3].InitSingle (hardware.GetPin(filtres));

    hardware.adc.Init(adcConfig, 4);
    hardware.adc.Start();

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SAW);
    osc.SetFreq(440);
    osc.SetAmp(0.0);

    osc2.SetWaveform(osc.WAVE_SAW);
    osc2.SetFreq(440);
    osc2.SetAmp(0.0);

    //setup filter
    filter1.Init(sample_rate);
    filter1.SetFreq(500.0);
    filter1.SetRes(0.85);
    filter1.SetDrive(0.8);


    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {
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
                        osc.SetFreq((mtof(note_msg.note)) + 440*(hardware.adc.GetFloat(0)));
                        osc2.SetFreq((mtof(note_msg.note)) + 440*(hardware.adc.GetFloat(1)));
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
