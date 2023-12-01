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
#ifdef CODEC_HTJ2K
		HTJ2K,
#endif
		Jpeg2000,
		Pcm,
		TimedText,
		IAB,
		ISXD,
		ProRes,
		SADM,
		ADM,
	};

enum eEssenceSubType {
		Unknown_SubType = 0,
		HTJ2K,
	};


	enum eColorEncoding {
		Unknown_Color_Encoding = 0,
		CDCI,
		RGBA
	};

struct MGASoundfieldGroup {
		SoundfieldGroup						soundfieldGroup;
		QString								mcaTagSymbol;
		QString								mcaTagName;
		QString								mcaSpokenLanguage;
		QString								mcaContent;
		QString								mcaUseClass;
		QString								mgaMetadataSectionLinkId;
		QString								mcaTitle;
		QString								mcaTitleVersion;
		QString								admAudioProgrammeID;
	};

typedef MGASoundfieldGroup ADMSoundfieldGroup;

	Metadata(Metadata::eEssenceType rType = Metadata::Unknown_Type, Metadata::eEssenceSubType rSubType = Metadata::Unknown_SubType);
	~Metadata() {}
	bool IsWellKnownType() { return type; }
	QString GetAsString();
	void GetAsTextDocument(QTextDocument &rDoc);

	Metadata::eEssenceType					type;
	Metadata::eEssenceSubType				subType;
	EditRate								editRate; // EditRate to appear in the CPL
	ASDCP::Rational							aspectRatio;
	quint32									storedWidth;
	quint32									storedHeight;
	quint32									displayWidth;
	quint32									displayHeight;
	eColorEncoding							colorEncoding;
	SMPTE::eColorPrimaries                  colorPrimaries = SMPTE::ColorPrimaries_UNKNOWN; // (k)
	SMPTE::eTransferCharacteristic        	transferCharcteristics = SMPTE::TransferCharacteristic_UNKNOWN; // (k)
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
	bool									tt_profile_is_text = true;
	//WR
	QString									languageTag;
	QString									mcaTitle;
	QString									mcaTitleVersion;
	QString									mcaAudioContentKind;
	QString									mcaAudioElementKind;
	QString									mcaTagName; // IAB only
	QString									mcaTagSymbol; // IAB only
	QString									essenceContainer; // IAB only
	QString									essenceCoding; // IAB only
	EditRate								referenceImageEditRate; // IAB only
	EditRate								audioSamplingRate; // IAB only
	EditRate								effectiveFrameRate = EditRate(0,0); // For TTML only: Effective Frame Rate of TTML1/IMSC1 file
	Duration								originalDuration; // For TTML only: Duration of TTML1/IMSC1 file expressed in effectiveFrameRate
	quint32									componentMinRef;  // J2K RGBA only
	quint32									componentMaxRef;  // J2K RGBA only
	QUuid									assetId;
	QString									pictureEssenceCoding; // J2K only
	QString									pixelLayout; // J2K RGB only
	QString									namespaceURI; // ISXD only
	bool									isPHDR = false; // For J2K with PHDRMetadataTrackSubDescriptor
	QList<MGASoundfieldGroup>				mgaSoundFieldGroupList;
	QList<ADMSoundfieldGroup>				admSoundFieldGroupList;
	qint32									mgaAverageBytesPerSecond;
	//WR
#ifdef APP5_ACES
	AS_02::ACES::ResourceList_t AncillaryResources;
#endif
};


Q_DECLARE_METATYPE(ASDCP::Rational);
Q_DECLARE_METATYPE(Metadata);
