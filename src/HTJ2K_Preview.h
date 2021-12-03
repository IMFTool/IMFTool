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
#include <QThreadPool>
#include "Error.h"
#include "ImfPackage.h"
#include "PreviewCommon.h"
#include "ojph_mem.h"
#include "ojph_file.h"
#include "ojph_codestream.h"
#include "ojph_params.h"
#include "ojph_img_io.h"

class AssetMxfTrack;

class HTJ2K: public PreviewCommon {

public:
	bool extractFrame(qint64 frameNr);
	bool decodeImage();
	quint8 cp_reduce; // layers to be skipped for decoding and reconstruction
	QString mMsg; // error message

protected:

	AS_02::JP2K::MXFReader *reader;
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF1; // current frame offset
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF2; // next frame offset
	int default_buffer_size = 40000000; // byte

    ojph::codestream* mpCodestream;
    ojph::mem_infile* mpMemBuf = NULL;
    ojph::ui16* mpOutBuf[3]; // Holding X,Y,Z of decoded image
	// data to QImage
	unsigned char *img_buff;

	QImage DataToQImage(); // converts mpOutBuf[3] -> QImage

	bool err = false; // error in the decoding process?
	ASDCP::JP2K::FrameBuffer *buff;
};

class HTJ2K_Preview : public QObject, public HTJ2K  {
	Q_OBJECT
private:

	void cleanUp();
	void setUp();
	void setAsset();
	
	QTime mDecode_time; // time (ms) needed to decode/convert the image
	QString mMxf_path; // path to current asset

public:
	HTJ2K_Preview();
	~HTJ2K_Preview();

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
};


