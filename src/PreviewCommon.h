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
#include <QObject>
#include "Error.h"
#include "ImfPackage.h"

class PreviewCommon {

public:
	//WR
	quint32	ComponentMinRef = 0;
	quint32	ComponentMaxRef = 0;
	//WR

	// change values with new asset:
	//QSharedPointer<AS_02::JP2K::MXFReader> reader_shared;
	int src_bitdepth = 0, prec_shift = 0, max = 0, layer = 3, RGBrange = 0, RGBmaxcv = 0;

	// enable conversion? (much slower!!)
	bool convert_to_709 = true; // default

	opj_dparameters_t params; // decoding parameters

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
	int max_f; // (float)pow(2, bitdepth)
	float max_f_; // max_f - 1;
	float *oetf_709;
	float *eotf_2020;
	float *eotf_PQ;
	float *eoft_HLG;
	float *eoft_sRGB;

};
