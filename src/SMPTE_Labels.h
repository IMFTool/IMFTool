// Copyright(C) Krispin Weiss
// Generated 28/12/2016 @ 19:17:30 by weon-PC!

#pragma once
#include <QObject>

namespace SMPTE {

	enum eColorPrimaries {
		ColorPrimaries,
		ColorPrimaries_SMPTE170M,
		ColorPrimaries_ITU470_PAL,
		ColorPrimaries_ITU709,
		ColorPrimaries_ITU2020,
		ColorPrimaries_SMPTE_DCDM,
		ColorPrimaries_P3D65,
	};


	static QMap<QString, eColorPrimaries> ColorPrimariesMap{
		{ "060E2B34040101060401010103000000", ColorPrimaries },
		{ "060E2B34040101060401010103010000", ColorPrimaries_SMPTE170M },
		{ "060E2B34040101060401010103020000", ColorPrimaries_ITU470_PAL },
		{ "060E2B34040101060401010103030000", ColorPrimaries_ITU709 },
		{ "060E2B340401010D0401010103040000", ColorPrimaries_ITU2020 },
		{ "060E2B340401010D0401010103050000", ColorPrimaries_SMPTE_DCDM },
		{ "060E2B340401010D0401010103060000", ColorPrimaries_P3D65 },
	};


	static QMap<eColorPrimaries, QString> vColorPrimaries{
		{ ColorPrimaries, "?" },
		{ ColorPrimaries_SMPTE170M, "SMPTE170M" },
		{ ColorPrimaries_ITU470_PAL, "ITU470PAL" },
		{ ColorPrimaries_ITU709, "ITU709" },
		{ ColorPrimaries_ITU2020, "ITU2020" },
		{ ColorPrimaries_SMPTE_DCDM, "SMPTEDCDM" },
		{ ColorPrimaries_P3D65, "P3D65" },
	};


	enum eTransferCharacteristic {
		TransferCharacteristic,
		TransferCharacteristic_ITU470_PAL,
		TransferCharacteristic_ITU709,
		TransferCharacteristic_SMPTE240M,
		TransferCharacteristic_274M_296M,
		TransferCharacteristic_ITU1361,
		TransferCharacteristic_linear,
		TransferCharacteristic_SMPTE_DCDM,
		TransferCharacteristic_IEC6196624_xvYCC,
		TransferCharacteristic_ITU2020,
		TransferCharacteristic_SMPTEST2084,
		TransferCharacteristic_HLG_OETF,
	};


	static QMap<QString, eTransferCharacteristic> TransferCharacteristicMap{
		{ "060E2B34040101010401010101000000", TransferCharacteristic },
		{ "060E2B34040101010401010101010000", TransferCharacteristic_ITU470_PAL },
		{ "060E2B34040101010401010101020000", TransferCharacteristic_ITU709 },
		{ "060E2B34040101010401010101030000", TransferCharacteristic_SMPTE240M },
		{ "060E2B34040101010401010101040000", TransferCharacteristic_274M_296M },
		{ "060E2B34040101060401010101050000", TransferCharacteristic_ITU1361 },
		{ "060E2B34040101060401010101060000", TransferCharacteristic_linear },
		{ "060E2B34040101080401010101070000", TransferCharacteristic_SMPTE_DCDM },
		{ "060E2B340401010D0401010101080000", TransferCharacteristic_IEC6196624_xvYCC },
		{ "060E2B340401010E0401010101090000", TransferCharacteristic_ITU2020 },
		{ "060E2B340401010D04010101010A0000", TransferCharacteristic_SMPTEST2084 },
		{ "060E2B340401010D04010101010B0000", TransferCharacteristic_HLG_OETF },
	};


	static QMap<eTransferCharacteristic, QString> vTransferCharacteristic{
		{ TransferCharacteristic, "?" },
		{ TransferCharacteristic_ITU470_PAL, "ITU470PAL" },
		{ TransferCharacteristic_ITU709, "ITU709" },
		{ TransferCharacteristic_SMPTE240M, "SMPTE240M" },
		{ TransferCharacteristic_274M_296M, "274M296M" },
		{ TransferCharacteristic_ITU1361, "ITU1361" },
		{ TransferCharacteristic_linear, "linear" },
		{ TransferCharacteristic_SMPTE_DCDM, "SMPTEDCDM" },
		{ TransferCharacteristic_IEC6196624_xvYCC, "IEC6196624xvYCC" },
		{ TransferCharacteristic_ITU2020, "ITU2020" },
		{ TransferCharacteristic_SMPTEST2084, "SMPTEST2084" },
		{ TransferCharacteristic_HLG_OETF, "HLGOETF" },
	};

static QMap<QString, QMap<QString, QString>> styles{
	{
		"white_8", {
				{ "font-size", "8px"},
				{ "color", "white"  }
			}
	}
};
};