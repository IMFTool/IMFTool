/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
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
#include "AS_02.h"
#include "Metadata.h"
#include <vector>
#include "ImfCommon.h"
#include <QString>
#include <QTextDocument>
#include <QTextOption>


class Metadata {

private:

public:
	enum eEssenceType {
		Unknown_Type = 0,
		Jpeg2000,
		Pcm,
		TimedText
	};

	enum eColorEncoding {
		Unknown_Color_Encoding = 0,
		CDCI,
		RGBA
	};

	// (k) - start
	enum eColorSpace {
		Unknown = 0,
		RGB709, // ["ITU-R.BT709 Color Primaries" in SMPTE RP 224]
		RGB_2020_PQ, // ["SMPTE ST 2084 Transfer Characteristic" in SMPTE RP 224]
		RGB_P3D65, // ["P3D65 Color Primaries" in SMPTE RP 224]
		YUV_709, // ["ITU-R.BT709 Color Primaries" in SMPTE RP 224]
		YUV_2020_LIN, // ["ITU-R.BT2020 Transfer Characteristic" in SMPTE RP 224]
		YUV_2020_PQ, // ["SMPTE ST 2084 Transfer Characteristic" in SMPTE RP 224]
		// ...
		COLOR_3, // UHD (8, 10 bit), 4K (8, 10 bit)
		COLOR_4, // UHD (8, 10 bit), xvYCC709 (BT.709)
		COLOR_5, // UHD (10, 12 bit), 4K (10, 12 bit), YCbCr (BT.2020), NON-CONST-Y
		COLOR_6, // 4K (10, 12, 16 bit), P3D65, CONST-Y
		COLOR_7, // UHD (10, 12, 16 bit), 4K (10, 12, 16 bit), YCbCr, NON-CONST-Y
	};
	// (k) - end

	Metadata(Metadata::eEssenceType type = Metadata::Unknown_Type);
	~Metadata() {}
	bool IsWellKnownType() { return type; }
	QString GetAsString();
	void GetAsTextDocument(QTextDocument &rDoc);

	Metadata::eEssenceType					type;
	EditRate								editRate; // EditRate to appear in the CPL
	ASDCP::Rational							aspectRatio;
	quint32									storedWidth;
	quint32									storedHeight;
	quint32									displayWidth;
	quint32									displayHeight;
	eColorEncoding							colorEncoding;
	eColorSpace								colorSpace; // (k)
	int										lutIndex; // (k)
	quint32									horizontalSubsampling;
	quint32									componentDepth;
	Duration								duration;
	quint32									audioChannelCount;
	quint32									audioQuantization;
	SoundfieldGroup							soundfieldGroup;
	QString									fileName;
	QString									filePath;
	QString									fileType;
	QString									profile;	//Timed Text Profile
	//WR
	QString									languageTag;
	QString									mcaTitle;
	QString									mcaTitleVersion;
	QString									mcaAudioContentKind;
	QString									mcaAudioElementKind;
	EditRate								effectiveFrameRate; // For TTML only: Effective Frame Rate of TTML1/IMSC1 file
	Duration								originalDuration; // For TTML only: Duration of TTML1/IMSC1 file expressed in effectiveFrameRate
	//WR
};


Q_DECLARE_METATYPE(ASDCP::Rational);
Q_DECLARE_METATYPE(Metadata);
