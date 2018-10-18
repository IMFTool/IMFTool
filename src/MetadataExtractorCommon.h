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
#ifdef APP5_ACES
#include "AS_02_ACES.h"
#else
#include "AS_02.h"
#include "Metadata.h"
#include <vector>
#endif
#include "ImfCommon.h"
#include <QString>
#include <QTextDocument>
#include <QTextOption>
#include <QUuid>
#include "SMPTE_Labels.h" // (k)

class Metadata {

private:

public:
	enum eEssenceType {
		Unknown_Type = 0,
#ifdef APP5_ACES
		Aces,
#endif
		Jpeg2000,
		Pcm,
		TimedText
	};

	enum eColorEncoding {
		Unknown_Color_Encoding = 0,
		CDCI,
		RGBA
	};

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
	SMPTE::eColorPrimaries                  colorPrimaries; // (k)
	SMPTE::eTransferCharacteristic        	transferCharcteristics; // (k)
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
	quint32									componentMinRef;  // J2K RGBA only
	quint32									componentMaxRef;  // J2K RGBA only
	QUuid									assetId;
	QString									pictureEssenceCoding; // J2K only
	QString									pixelLayout; // J2K RGB only
	//WR
#ifdef APP5_ACES
	AS_02::ACES::ResourceList_t AncillaryResources;
#endif
};


Q_DECLARE_METATYPE(ASDCP::Rational);
Q_DECLARE_METATYPE(Metadata);
