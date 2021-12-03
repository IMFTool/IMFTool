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
	int offset_y = 0, range_y = 0, range_c = 0;

	// enable conversion? (much slower!!)
	bool convert_to_709 = true; // default

	Metadata::eColorEncoding ColorEncoding = Metadata::eColorEncoding::Unknown_Color_Encoding; // YCbCr or RGB
	SMPTE::eColorPrimaries colorPrimaries = SMPTE::eColorPrimaries::ColorPrimaries_UNKNOWN; // BT.709 / BT.2020 / DCI-P3
	SMPTE::eTransferCharacteristic transferCharacteristics = SMPTE::eTransferCharacteristic::TransferCharacteristic_UNKNOWN; // BT.709 / BT.2020 / PQ
	qint16 Kb = 0, Kr = 0, Kg = 0, Kbg = 0, Krg = 0; // YCbCr -> RGB parameters (depending on BT.709 or BT.2020)

	QSharedPointer<AssetMxfTrack> current_asset; // pointer to current asset

	PreviewCommon();
	~PreviewCommon();

protected:
	// luts
	static const int bitdepth = 16; // lookup table size (default: 16 bit)
	int max_f; // (float)pow(2, bitdepth)
	float max_f_; // max_f - 1;

	quint16 *eotf_DCDM;
	quint16 *eotf_PQ;
	quint16 *eotf_2020;
	quint32 *eotf_HLG;
	quint8 *oetf_709;

	// data to qimage
	int w=0, h=0, xpos=0, buff_pos=0, x=0, y=0, bytes_per_line=0, index_422=0;
	qint32 out_ri=0, out_gi=0, out_bi=0;
	qint32 cv_comp1=0, cv_comp2=0, cv_comp3=0;
	qint32 cv_compY=0, cv_compCb=0, cv_compCr=0;

	bool linearize();
	bool colorTransform();
	bool setCodingParameters();

	// info methods
	static void info_callback(const char *msg, void *data);
	static void warning_callback(const char *msg, void *data);
	static void error_callback(const char *msg, void *data);

	bool err = false; // error in the decoding process?
	Kumu::FileReaderFactory defaultFactory;

};
