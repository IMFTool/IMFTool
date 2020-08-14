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
#pragma once
#include "Error.h"
#include "ImfPackage.h"

class PreviewCommon {

public:
	//WR
	quint32	ComponentMinRef = 0;
	quint32	ComponentMaxRef = 0;
	//WR

	// Values are set when opening a new asset:
	int src_bitdepth = 0, prec_shift = 0, max = 0, layer = 3;

	// enable conversion? (much slower!!)
	bool convert_to_709 = true; // default

	Metadata::eColorEncoding ColorEncoding = Metadata::eColorEncoding::Unknown_Color_Encoding; // YCbCr or RGB
	SMPTE::eColorPrimaries colorPrimaries = SMPTE::eColorPrimaries::ColorPrimaries_UNKNOWN; // BT.709 / BT.2020 / DCI-P3
	SMPTE::eTransferCharacteristic transferCharacteristics = SMPTE::eTransferCharacteristic::TransferCharacteristic_UNKNOWN; // BT.709 / BT.2020 / PQ
	float Kb = 0, Kr = 0, Kg = 0; // YCbCr -> RGB (depending on BT.709 or BT.2020)

	QSharedPointer<AssetMxfTrack> current_asset; // pointer to current asset

	PreviewCommon();
	~PreviewCommon();

protected:
	// luts
	static const int bitdepth = 16; // lookup table size (default: 16 bit)
	static const int bitdepth_dcdm = 12;
	int max_f; // (float)pow(2, bitdepth)
	float max_f_; // max_f - 1;
	float *oetf_709;
	float *eotf_2020;
	float *eotf_PQ;
	float *eotf_HLG;
	float *eotf_sRGB;
	quint16 *eotf_DCDM;
	quint16 *eotf_PQi;
	quint16 *eotf_2020i;
	quint16 *eotf_HLGi;
	quint8 *oetf_709i;

	// data to qimage
	int w=0, h=0, xpos=0, buff_pos=0, x=0, y=0, bytes_per_line=0;
	qint32 out_ri=0, out_gi=0, out_bi=0, cv_comp1=0, cv_comp2=0, cv_comp3=0;
	float Y=0, Cb=0, Cr=0, r=0, g=0, b=0, out_r=0, out_g=0, out_b=0, out_r8=0, out_g8=0, out_b8=0;

	// info methods
	static void info_callback(const char *msg, void *data);
	static void warning_callback(const char *msg, void *data);
	static void error_callback(const char *msg, void *data);

	bool err = false; // error in the decoding process?

};
