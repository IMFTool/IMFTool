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
#include <QObject>
#include <QThreadPool>
#include "Error.h"
#include "ImfPackage.h"

#if defined WIN32
//Caused by a persisting linking error in Imath: https://github.com/AcademySoftwareFoundation/Imath/issues/212
#define IMATH_HALF_NO_LOOKUP_TABLE
#endif
#include "ACES.h"
#include "AS_02_ACES.h"
#include "ImfRgbaFile.h"
#include "ImfChromaticities.h"
#include "ImfArray.h"
#include "As02AcesIStream.h"
#include "PreviewCommon.h"

using namespace Imf;
using namespace Imath;

class AssetMxfTrack;

typedef struct
{
	uint8_t* pData; //Our data.
	size_t dataSize; //How big is our data.
	size_t offset; //Where are we currently in our data.
}aces_memory_stream;

class ACES : public PreviewCommon {

public:

protected:

	//AS_02::JP2K::MXFReader *reader;
	AS_02::ACES::MXFReader *reader;
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF1; // current frame offset
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF2; // next frame offset
	int default_buffer_size = 200000000; // byte

	
	As02AcesIStream* pAcesIStream;

	QImage DataToQImage(quint8 rScale = 0, bool rShowActiveArea = false); // converts opj_image_t -> QImage


	bool err = false; // error in the decoding process?
	AS_02::ACES::FrameBuffer *buff;

	static Imf::Chromaticities aces_chromaticities;
	static Imath::M44f A0Rec709TransformMatrix;
};

class ACES_Preview : public QObject, public ACES {
	Q_OBJECT
private:

	void cleanUp();
	void setUp();
	bool decodeImage();
	void setAsset();
	bool extractFrame(qint64 frameNr);
	
	QTime mDecode_time; // time (ms) needed to decode/convert the image
	QString mMsg; // error message
	QString mMxf_path; // path to current asset
	quint8 mScale; // scale factor for images
	bool mShowActiveArea; // Show Active Area (true) or Show Native Resolution (false)


public:
	ACES_Preview();
	~ACES_Preview();

	qint64 mFrameNr;
	qint64 mFirst_proxy;
	qint64 mSecond_proxy;
	QSharedPointer<AssetMxfTrack> asset;
signals:
	void proxyFinished(const QImage&, const QImage&); // finished generating proxy
	void ShowFrame(const QImage&);
	void decodingStatus(qint64, QString);
	void finished(); // everything, including cleanup is done...
public slots:
	void getProxy();
	void decode();
	void setLayer(int);
	// Show Active Area (true) or Show Native Resolution (false)
	void showActiveArea(bool);
};


