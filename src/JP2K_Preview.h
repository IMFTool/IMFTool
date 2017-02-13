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

class AssetMxfTrack;

typedef struct
{
	OPJ_UINT8* pData; //Our data.
	OPJ_SIZE_T dataSize; //How big is our data.
	OPJ_SIZE_T offset; //Where are we currently in our data.
}opj_memory_stream;

class JP2 {

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
	float *oetf_709;
	float *eotf_709;
	float *oetf_2020;
	float *eotf_2020;
	float *oetf_PQ;
	float *eotf_PQ;

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

class JP2K_Preview : public QObject, public JP2 {
	Q_OBJECT
private:

	void cleanUp();
	void setUp();
	bool decodeImage();
	void setAsset();
	bool extractFrame(qint64 frameNr);
	void save2File(); // save JP2K bytestream to file
	
	int cpus = 0; // nr of threads used for decoding
	QTime decode_time; // time (ms) needed to decode/convert the image
	QString msg; // error message
	QString mxf_path; // path to current asset

public:
	JP2K_Preview();
	~JP2K_Preview();

	qint64 frameNr;
	qint64 first_proxy;
	qint64 second_proxy;
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


