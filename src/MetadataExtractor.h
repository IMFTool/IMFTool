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
#include "MetadataExtractorCommon.h"
#include "ImfCommon.h"
#include "Error.h"
#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QByteArray>
#include "ImfPackageCommon.h"

class AbstractWorker;

class MetadataExtractor : public QObject {

	Q_OBJECT

public:
	MetadataExtractor(QObject *pParent = NULL);
	virtual ~MetadataExtractor() {}
	//! Reads MXF descriptors and writes the properties into rMetadata.
	Error ReadMetadata(Metadata &rMetadata, const QString &rSourceFile);
	Error ReadAncillaryResource(QByteArray &rRawData, const byte_t ResourceID[], const QString &rSourceFile);
	void SetCplEditRate(EditRate rCplEditRate) {mCplEditRate = rCplEditRate;};
private:

	Q_DISABLE_COPY(MetadataExtractor);
	Error ReadJP2KMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile);
	Error ReadPcmMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile);
	Error ReadWavHeader(Metadata &rMetadata, const QFileInfo &rSourceFile);
	Error ReadTimedTextMetadata(Metadata &rMetadata, const QFileInfo &rSourceFile);
    Error ReadTimedTextMxfDescriptor(Metadata &rMetadata, const QFileInfo &rSourceFile);

	float DurationExtractor(xercesc::DOMDocument *dom_doc, float fr, int tr);
	float GetElementDuration(xercesc::DOMElement* eleDom, float fr, int tr);
	float ConvertTimingQStringtoDouble(QString string_time, float fr, int tr);
	//WR
	EditRate mCplEditRate;  // for creating TT files
	//WR
};

struct RateInfo
{
	ASDCP::UL ul;
	double bitrate;
	std::string label;

	RateInfo(const ASDCP::UL& u, const double& b, const std::string& l) {
		ul = u; bitrate = b; label = l;
	}

};
