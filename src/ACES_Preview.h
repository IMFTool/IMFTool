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

#include "ACES.h"
#include "AS_02_ACES.h"
#include "ImfRgbaFile.h"
#include "ImfFrameBuffer.h"
#include "ImfStandardAttributes.h"
#include "ImfChromaticities.h"
#include "ImfRgba.h"
#include "ImfArray.h"
#include "ImathMatrix.h"
#include "ImfRgbaFile.h"
#include "As02AcesIStream.h"

using namespace Imf;
using namespace Imath;

class AssetMxfTrack;

typedef struct
{
	uint8_t* pData; //Our data.
	size_t dataSize; //How big is our data.
	size_t offset; //Where are we currently in our data.
}aces_memory_stream;

class ACES {

public:

	//WR
	quint32	ComponentMinRef;
	quint32	ComponentMaxRef;
	//WR

	// change values with new asset:
	//QSharedPointer<AS_02::JP2K::MXFReader> reader_shared;
	int src_bitdepth, prec_shift, max, layer = 3, RGBrange, RGBmaxcv;

	// enable conversion? (much slower!!)
	bool convert_to_709 = true; // default

	opj_dparameters_t params; // decoding parameters

	Metadata::eColorEncoding ColorEncoding; // YCbCr or RGB
	SMPTE::eColorPrimaries colorPrimaries; // BT.709 / BT.2020 / DCI-P3
	SMPTE::eTransferCharacteristic transferCharactersitics; // BT.709 / BT.2020 / PQ
	float Kb, Kr, Kg; // YCbCr -> RGB (depending on BT.709 or BT.2020)

	QSharedPointer<AssetMxfTrack> current_asset; // pointer to current asset

protected:

	// luts
	static const int bitdepth = 16; // lookup table size (default: 16 bit)
	int max_f; // (float)pow(2, bitdepth)
	float max_f_; // max_f - 1;
	float *eotf_2020;
	float *eotf_PQ;

	//AS_02::JP2K::MXFReader *reader;
	AS_02::ACES::MXFReader *reader;
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF1; // current frame offset
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF2; // next frame offset
	int default_buffer_size = 30000000; // byte

	
	//::opj_image_t  *psImage;
	//AS_02_aces_h__::opj_codec_t *pDecompressor;
	//AS_02_aces_h__::opj_stream_t *pStream;
	//aces_memory_stream pMemoryStream;
	As02AcesIStream* pAcesIStream;

	// data to qimage
	int w, h, xpos, buff_pos, x, y, bytes_per_line;
	float Y, Cb, Cr, r, g, b, out_r, out_g, out_b, out_r8, out_g8, out_b8;
	QImage DataToQImage(); // converts opj_image_t -> QImage


	// info methods
	static void info_callback(const char *msg, void *data);
	static void warning_callback(const char *msg, void *data);
	static void error_callback(const char *msg, void *data);

	bool err = false; // error in the decoding process?
	//ASDCP::JP2K::FrameBuffer *buff;
	AS_02::ACES::FrameBuffer *buff;

	static Imf::Chromaticities aces_chromaticities;
	static Imath::M44f A0Rec709TransformMatrix;
	float *oetf_709;
};

class ACES_Preview : public QObject, public ACES {
	Q_OBJECT
private:

	void cleanUp();
	void setUp();
	bool decodeImage();
	void setAsset();
	bool extractFrame(qint64 frameNr);
	
	int mCpus = 0; // nr of threads used for decoding
	QTime mDecode_time; // time (ms) needed to decode/convert the image
	QString mMsg; // error message
	QString mMxf_path; // path to current asset


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
};


