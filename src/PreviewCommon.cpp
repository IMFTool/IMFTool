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

	float alpha = 1.09929682680944f;
	float beta = 0.018053968510807f;

	float m1 = 0.1593017578125f;
	float m2 = 78.84375f;
	float c1 = 0.8359375f;
	float c2 = 18.8515625f;
	float c3 = 18.6875f;

	float a = 0.17883277f;
	float b = 1 - 4*a;
	float c = 0.5 - a * log(4*a);

	eotf_PQ = new quint16[max_f];
	eotf_2020 = new quint16[max_f];
	eotf_HLG = new quint32[max_f];
	eotf_DCDM = new quint16[max_f];

	oetf_709 = new quint8[max_f];


	for (int i = 0; i < max_f; i++) {

		float input = (float)(i / max_f_); // convert input to value between 0...1

		// BT.709 - OETF (Inverse of BT.1886 EOTF)
		oetf_709[i] = (quint8)(255.0f * pow(input, 1.0f / 2.4f) + 0.5);

		// BT.2020 - EOTF
		if (input < (4.5 * beta)) {
			eotf_2020[i] = input / 4.5 * max_f_;
		}
		else {
			eotf_2020[i] = pow(((input + (alpha - 1)) / alpha), 1.0 / 0.45) * max_f_;
		}

		// PQ Integer
		eotf_PQ[i] = (quint16)(pow(((pow(input, (1.0f / m2)) - c1)) / (c2 - c3 *pow(input, (1.0f / m2))), 1.0f / m1) * max_f_+ 0.5f);

		//HLG OEFT^-1 including the inverted gamma correction and scaling per Figure 42 of BT.2390-7, max value for input = 1.0: 257403,7778
		if (input <= 0.5) {
			eotf_HLG[i] = (quint32)(pow(input * input / 3.0, 1.03) / 0.2546 * max_f_ + 0.5);
		}
		else {
			eotf_HLG[i] = (quint32)(pow((exp((input - c) / a) + b)/12.0f, 1.03f) / 0.2546f * max_f_ + 0.5f);
		}
		// DCDM
		eotf_DCDM[i] = (quint16)(pow(input, 2.6f) * 65535.0f);

	}

	eotf_PQ[0] = 0;
}

bool PreviewCommon::linearize() {
	// linearize XYZ data
	bool success = true;
	switch (transferCharacteristics) {

	case SMPTE::TransferCharacteristic_CinemaMezzanineDCDM:
	case SMPTE::TransferCharacteristic_CinemaMezzanineDCDM_Wrong:
		cv_comp1 = eotf_DCDM[cv_comp1]; // convert to 16 bit linear
		cv_comp2 = eotf_DCDM[cv_comp2]; // convert to 16 bit linear
		cv_comp3 = eotf_DCDM[cv_comp3]; // convert to 16 bit linear
		break;

	case SMPTE::TransferCharacteristic_CinemaMezzanineLinear:
	case SMPTE::TransferCharacteristic_linear:
		break;

	case SMPTE::TransferCharacteristic_SMPTEST2084:
		cv_comp1 = eotf_PQ[cv_comp1]; // convert to 16 bit linear, scale by 100 such that 100 nits correspond to 65535
		cv_comp2 = eotf_PQ[cv_comp2]; // convert to 16 bit linear
		cv_comp3 = eotf_PQ[cv_comp3]; // convert to 16 bit linear
		cv_comp1 *= 100;
		cv_comp2 *= 100;
		cv_comp3 *= 100;
		break;

	case SMPTE::TransferCharacteristic_ITU2020:
		cv_comp1 = eotf_2020[cv_comp1]; // convert to 16 bit linear
		cv_comp2 = eotf_2020[cv_comp2]; // convert to 16 bit linear
		cv_comp3 = eotf_2020[cv_comp3]; // convert to 16 bit linear
		break;

	case SMPTE::TransferCharacteristic_HLG_OETF:
		cv_comp1 = eotf_HLG[cv_comp1]; // convert to 16 bit linear
		cv_comp2 = eotf_HLG[cv_comp2]; // convert to 16 bit linear
		cv_comp3 = eotf_HLG[cv_comp3]; // convert to 16 bit linear
		break;

	case SMPTE::TransferCharacteristic_ITU709:
	case SMPTE::TransferCharacteristic_IEC6196624_xvYCC:
		// convert to 8 bit
		cv_comp1 = cv_comp1 >> (bitdepth -8);
		cv_comp2 = cv_comp2 >> (bitdepth -8);
		cv_comp3 = cv_comp3 >> (bitdepth -8);
		break;

	default:
		success = false; // Unknown / unhandled transfer characteristics
	}
	return success;
}

bool PreviewCommon::colorTransform() {

	switch (colorPrimaries) {
	case SMPTE::ColorPrimaries_CinemaMezzanine:
		// convert from XYZ -> BT.709, matrix coefficients are scaled by 1024 to allow for integer processing
		out_ri = cv_comp1*3319 + cv_comp2*-1574 + cv_comp3*-511;
		out_gi = cv_comp1*-993 + cv_comp2*1921 + cv_comp3*43;
		out_bi = cv_comp1*57  + cv_comp2*-209 + cv_comp3*1082;
		break;

	case SMPTE::ColorPrimaries_ITU2020:
		// convert from BT.2020 -> BT.709, matrix coefficients are scaled by 1024 to allow for integer processing (CV 655 = 100 nits, CV 65535 = 10,000 nits)
		out_ri = cv_comp1*1700 + cv_comp2*-602 + cv_comp3*-75;
		out_gi = cv_comp1*-128 + cv_comp2*1160 + cv_comp3*-9;
		out_bi = cv_comp1*-19  + cv_comp2*-103 + cv_comp3*1146;
		break;

	case SMPTE::ColorPrimaries_P3D65:
		// convert from P3D65 -> BT.709, matrix coefficients are scaled by 1024 to allow for integer processing
		out_ri = cv_comp1*1234 + cv_comp2*-210;
		out_gi = cv_comp1*-42  + cv_comp2*1066;
		out_bi = cv_comp1*-20  + cv_comp2*-125 + cv_comp3*1168;
		break;

	default:
		return false; // Unknown / unhandled color primaries
	}

	if (out_ri < 0) out_ri = 0;
	if (out_gi < 0) out_gi = 0;
	if (out_bi < 0) out_bi = 0;

	// convert back to a 16 bit representation
	out_ri = out_ri >> 10;
	out_gi = out_gi >> 10;
	out_bi = out_bi >> 10;

	// clamp values between 0...65535
	if (out_ri >= max_f) out_ri = max_f - 1;
	if (out_gi >= max_f) out_gi = max_f - 1;
	if (out_bi >= max_f) out_bi = max_f - 1;

	return true;
}

bool PreviewCommon::setCodingParameters() {
	prec_shift = src_bitdepth - 8;
	max = (1 << src_bitdepth) - 1;

	offset_y = 16 << (src_bitdepth - 8);
	range_y = 219 << (src_bitdepth - 8);
	range_c = (max + 1 - 2 * offset_y);

	switch (colorPrimaries) {
	case SMPTE::ColorPrimaries_ITU709:
		// set YCbCr -> RGB conversion parameters
		Kr = 218;
		Kg = 732;
		Kb = 74;
		Kbg = 192;
		Krg = 479;
		break;
	case SMPTE::ColorPrimaries_ITU2020:
		// set YCbCr -> RGB conversion parameters
		Kr = 269;
		Kg = 694;
		Kb = 61;
		Kbg = 169;
		Krg = 585;
		break;
	case SMPTE::ColorPrimaries_SMPTE170M:
	case SMPTE::ColorPrimaries_ITU470_PAL:
		// set YCbCr -> RGB conversion parameters
		Kr = 306;
		Kg = 601;
		Kb = 117;
		Kbg = 352;
		Krg = 731;
		break;
	case SMPTE::ColorPrimaries_P3D65:
	case SMPTE::ColorPrimaries_CinemaMezzanine:
		//P365 and DCDM is 4:4:4 only
		break;
	default:
		return false;
		break; // abort!
	}
	return true;
}

void PreviewCommon::info_callback(const char *mMsg, void *client_data) {
	qDebug() << "INFO" << mMsg;
}

void PreviewCommon::warning_callback(const char *mMsg, void *client_data) {
	qDebug() << "WARNING" << mMsg;
}

void PreviewCommon::error_callback(const char *mMsg, void *client_data) {
	qDebug() << "ERROR" << mMsg;
}


PreviewCommon::~PreviewCommon() {
	delete[] oetf_709;
	delete[] eotf_2020;
	delete[] eotf_PQ;
	delete[] eotf_HLG;
	delete[] eotf_DCDM;
}
