/* Copyright(C) 2020 Wolfgang Ruppel
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.If not, see <http://www.gnu.org/licenses/>.
*/


#include "PreviewCommon.h"

PreviewCommon::PreviewCommon() {
	max_f = 1 << bitdepth;
	max_f_ = (float)(max_f)-1.0;

	oetf_709 = new float[max_f];

	float alpha = 1.09929682680944;
	float beta = 0.018053968510807;
	eotf_2020 = new float[max_f];

	float m1 = 0.1593017578125;
	float m2 = 78.84375;
	float c1 = 0.8359375;
	float c2 = 18.8515625;
	float c3 = 18.6875;
	eotf_PQ = new float[max_f];

	float a = 0.17883277;
	float b = 1 - 4*a;
	float c = 0.5 - a * log(4*a);
	eoft_HLG= new float[max_f];
	eoft_sRGB= new float[max_f];

	for (int i = 0; i < max_f; i++) {

		float input = (float)(i / max_f_); // convert input to value between 0...1

		// BT.709 - OETF (Inverse of BT.1886 EOTF)
		oetf_709[i] = pow(input, 1.0f / 2.4f);

		// BT.2020 - EOTF
		if (input < (4.5 * beta)) {
			eotf_2020[i] = input / 4.5;
		}
		else {
			eotf_2020[i] = pow(((input + (alpha - 1)) / alpha), 1.0 / 0.45);
		}

		// PQ
		eotf_PQ[i] = pow(((pow(input, (1.0 / m2)) - c1)) / (c2 - c3 *pow(input, (1.0 / m2))), 1.0 / m1) * 10000;

		//HLG OEFT^-1 including the inverted gamma correction and scaling per Figure 42 of BT.2390-7
		if (input <= 0.5) {
			eoft_HLG[i] = pow(input * input / 3.0, 1.03) / 0.2546;
		}
		else {
			eoft_HLG[i] = pow((exp((input - c) / a) + b)/12.0, 1.03) / 0.2546;
		}
		//sRGB
		if (input <= 0.03928) {
			eoft_sRGB[i] = input / 12.92;
		} else {
			eoft_sRGB[i] = pow((input + 0.055)/1.055, 2.4f);
		}
	}

	eotf_PQ[0] = 0;
}

PreviewCommon::~PreviewCommon() {
	delete oetf_709;
	delete eotf_2020;
	delete eotf_PQ;
	delete eoft_HLG;
}
