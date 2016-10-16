/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel
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

	Metadata(Metadata::eEssenceType type = Metadata::Unknown_Type);
	~Metadata() {}
	bool IsWellKnownType() { return type; }
	QString GetAsString();
	void GetAsTextDocument(QTextDocument &rDoc);

	Metadata::eEssenceType					type;
	EditRate								editRate;
	ASDCP::Rational							aspectRatio;
	quint32									storedWidth;
	quint32									storedHeight;
	quint32									displayWidth;
	quint32									displayHeight;
	eColorEncoding							colorEncoding;
	quint32									horizontalSubsampling;
	quint32									componentDepth;
	Duration								duration;
	quint32									audioChannelCount;
	quint32									audioQuantization;
	SoundfieldGroup							soundfieldGroup;
	QString									fileName;
	QString									filePath;
	QString									fileType;
	EditRate								infoEditRate; //TT Edit Rate is based od milliseconds in this application. infoEdirRate is used for the real ER of the tt
	QString									profile;	//Timed Text Profile
	//WR
	QString									languageTag;
	QString									mcaTitle;
	QString									mcaTitleVersion;
	QString									mcaAudioContentKind;
	QString									mcaAudioElementKind;
	//WR
};


Q_DECLARE_METATYPE(ASDCP::Rational);
Q_DECLARE_METATYPE(Metadata);
