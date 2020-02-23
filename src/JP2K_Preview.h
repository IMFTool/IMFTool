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
#include "openjpeg.h"
#include "ImfPackage.h"
#include "PreviewCommon.h"

class AssetMxfTrack;

typedef struct
{
	OPJ_UINT8* pData; //Our data.
	OPJ_SIZE_T dataSize; //How big is our data.
	OPJ_SIZE_T offset; //Where are we currently in our data.
}opj_memory_stream;

class JP2: public PreviewCommon {

public:

protected:

	AS_02::JP2K::MXFReader *reader;
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF1; // current frame offset
	ASDCP::MXF::IndexTableSegment::IndexEntry IndexF2; // next frame offset
	int default_buffer_size = 30000000; // byte

	OPENJPEG_H::opj_image_t *psImage;
	OPENJPEG_H::opj_codec_t *pDecompressor;
	OPENJPEG_H::opj_stream_t *pStream;
	opj_memory_stream pMemoryStream;

	// data to qimage
	unsigned char *img_buff;
	int w, h, xpos, buff_pos, x, y, bytes_per_line;
	float Y, Cb, Cr, r, g, b, out_r, out_g, out_b, out_r8, out_g8, out_b8;
	QImage DataToQImage(); // converts opj_image_t -> QImage

	// memory stream methods
	static OPJ_SIZE_T opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data);
	static OPJ_SIZE_T opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data);
	static OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data);
	static OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data);
	static void opj_memory_stream_do_nothing(void * p_user_data);
	OPENJPEG_H::opj_stream_t* opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream);

	// info methods
	static void info_callback(const char *msg, void *data);
	static void warning_callback(const char *msg, void *data);
	static void error_callback(const char *msg, void *data);

	bool err = false; // error in the decoding process?
	ASDCP::JP2K::FrameBuffer *buff;
};

class JP2K_Preview : public QObject, public JP2  {
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
	JP2K_Preview();
	~JP2K_Preview();

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


