/* ------------------------------------------------------------
name: "Echo"
Code generated with Faust 2.43.1 (https://faust.grame.fr)
Compilation options: -lang cpp -cn Echo -scn  -es 1 -mcd 16 -single -ftz 0

in main cpp file:
#include "MapUI.h"
#include "Echo.h"
Echo* echo;
MapUI mapUI;

//use in callback : echo->compute(size,(float**) in, out);

//init: 	Echo.Init(hw.AudioSampleRate());
//		    Echo.buildUserInterface(&mapUI);

// Echo file made from Fause Kadenze talk

------------------------------------------------------------ */

#ifndef  __Echo_H__
#define  __Echo_H__

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>
#include "UI.h"

#ifndef FAUSTCLASS 
#define FAUSTCLASS Echo
#endif

#define RESTRICT __restrict__

float DSY_SDRAM_BSS fRec0[32768];
float DSY_SDRAM_BSS fRec3[32768];

class Echo {
	
 private:
	
	float fHslider0;
	int IOTA0;
	int fSampleRate;
	float fConst0;
	float fConst1;
	float fHslider1;
	float fConst2;
	float fRec2[2];
	float fVec0[2];
	float fVec1[2];
	float fRec1[2];
	
	float fVec2[2];
	float fVec3[2];
	float fRec4[2];
	
	
 public:

	int getNumInputs() {
		return 2;
	}
	int getNumOutputs() {
		return 2;
	}
	
	static void classInit(int sample_rate) {
	}
	
	void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(192000.0f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = 0.0441000015f / fConst0;
		fConst2 = 1.0f - 44.0999985f / fConst0;
	}
	
	void instanceResetUserInterface() {
		fHslider0 = float(0.5f);
		fHslider1 = float(200.0f);
	}
	
	void instanceClear() {
		IOTA0 = 0;
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec2[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fVec0[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fVec1[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec1[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 32768; l4 = l4 + 1) {
			fRec0[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fVec2[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fVec3[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec4[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 32768; l8 = l8 + 1) {
			fRec3[l8] = 0.0f;
		}
	}
	
	void Init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	Echo* clone() {
		return new Echo();
	}
	
	int getSampleRate() {
		return fSampleRate;
	}
	
	void buildUserInterface(UI* ui_interface) {
		ui_interface->declare(0, "0", "");
		ui_interface->openHorizontalBox("Echof");
		ui_interface->declare(&fHslider1, "knob", "7");
		ui_interface->addHorizontalSlider("Duration", &fHslider1, float(200.0f), float(1.0f), float(200.0f), float(1.0f));
		ui_interface->declare(&fHslider0, "knob", "8");
		ui_interface->addHorizontalSlider("Feedback", &fHslider0, float(0.5f), float(0.0f), float(1.0f), float(0.00999999978f));
		ui_interface->closeBox();
	}
	
	void compute(int count, float** RESTRICT inputs, float** RESTRICT outputs) {
		float* input0 = inputs[0];
		float* input1 = inputs[1];
		float* output0 = outputs[0];
		float* output1 = outputs[1];
		float fSlow0 = float(fHslider0);
		float fSlow1 = fConst1 * float(fHslider1);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec2[0] = fSlow1 + fConst2 * fRec2[1];
			float fTemp0 = fConst0 * fRec2[0];
			float fTemp1 = fTemp0 + -2.49999499f;
			int iTemp2 = int(fTemp1);
			int iTemp3 = std::min<int>(24000, std::max<int>(0, iTemp2)) + 1;
			float fTemp4 = std::floor(fTemp1);
			float fTemp5 = fTemp0 + -2.0f - fTemp4;
			float fTemp6 = 0.0f - fTemp5;
			float fTemp7 = fTemp0 + -3.0f - fTemp4;
			float fTemp8 = 0.0f - 0.5f * fTemp7;
			float fTemp9 = fTemp0 + -4.0f - fTemp4;
			float fTemp10 = 0.0f - 0.333333343f * fTemp9;
			float fTemp11 = fTemp0 + -5.0f - fTemp4;
			float fTemp12 = 0.0f - 0.25f * fTemp11;
			float fTemp13 = fTemp0 + -1.0f - fTemp4;
			int iTemp14 = std::min<int>(24000, std::max<int>(0, iTemp2 + 1)) + 1;
			float fTemp15 = 0.0f - fTemp7;
			float fTemp16 = 0.0f - 0.5f * fTemp9;
			float fTemp17 = 0.0f - 0.333333343f * fTemp11;
			int iTemp18 = std::min<int>(24000, std::max<int>(0, iTemp2 + 2)) + 1;
			float fTemp19 = 0.0f - fTemp9;
			float fTemp20 = 0.0f - 0.5f * fTemp11;
			float fTemp21 = fTemp5 * fTemp7;
			int iTemp22 = std::min<int>(24000, std::max<int>(0, iTemp2 + 3)) + 1;
			float fTemp23 = 0.0f - fTemp11;
			float fTemp24 = fTemp21 * fTemp9;
			int iTemp25 = std::min<int>(24000, std::max<int>(0, iTemp2 + 4)) + 1;
			float fTemp26 = fRec0[(IOTA0 - iTemp3) & 32767] * fTemp6 * fTemp8 * fTemp10 * fTemp12 + fTemp13 * (fRec0[(IOTA0 - iTemp14) & 32767] * fTemp15 * fTemp16 * fTemp17 + 0.5f * fTemp5 * fRec0[(IOTA0 - iTemp18) & 32767] * fTemp19 * fTemp20 + 0.166666672f * fTemp21 * fRec0[(IOTA0 - iTemp22) & 32767] * fTemp23 + 0.0416666679f * fTemp24 * fRec0[(IOTA0 - iTemp25) & 32767]);
			fVec0[0] = fTemp26;
			float fTemp27 = fTemp26 + fVec0[1];
			fVec1[0] = fTemp27;
			fRec1[0] = 0.995000005f * fRec1[1] - 0.5f * (fVec1[1] - fTemp27);
			fRec0[IOTA0 & 32767] = float(input0[i0]) + fSlow0 * fRec1[0];
			output0[i0] = float(fRec0[IOTA0 & 32767]);
			float fTemp28 = fTemp13 * (fTemp15 * fTemp16 * fTemp17 * fRec3[(IOTA0 - iTemp14) & 32767] + 0.5f * fTemp5 * fTemp19 * fTemp20 * fRec3[(IOTA0 - iTemp18) & 32767] + 0.166666672f * fTemp21 * fTemp23 * fRec3[(IOTA0 - iTemp22) & 32767] + 0.0416666679f * fTemp24 * fRec3[(IOTA0 - iTemp25) & 32767]);
			float fTemp29 = fTemp6 * fTemp8 * fTemp10 * fTemp12 * fRec3[(IOTA0 - iTemp3) & 32767];
			fVec2[0] = fTemp29 + fTemp28;
			float fTemp30 = fTemp28 + fTemp29 + fVec2[1];
			fVec3[0] = fTemp30;
			fRec4[0] = 0.995000005f * fRec4[1] + 0.5f * (fTemp30 - fVec3[1]);
			fRec3[IOTA0 & 32767] = float(input1[i0]) + fSlow0 * fRec4[0];
			output1[i0] = float(fRec3[IOTA0 & 32767]);
			IOTA0 = IOTA0 + 1;
			fRec2[1] = fRec2[0];
			fVec0[1] = fVec0[0];
			fVec1[1] = fVec1[0];
			fRec1[1] = fRec1[0];
			fVec2[1] = fVec2[0];
			fVec3[1] = fVec3[0];
			fRec4[1] = fRec4[0];
		}
	}

};

#endif