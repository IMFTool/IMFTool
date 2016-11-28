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
#include <QImage>
#include <QThreadPool>
#include <QRunnable>
#include <QStringList>
#include <QFileInfo>
#include <QVariant>
#include "openjpeg.h"
#include "Error.h"
#include <QTime>
#include <QDebug>
#include <QTimer>
#include <chrono>
#include "ImfPackage.h"
#include "JP2K_Player.h"

class FrameRequest;

class JP2K_Decoder : public QObject, public QRunnable{

	Q_OBJECT

public:
	JP2K_Decoder(QSharedPointer<DecodedFrames>&, QSharedPointer<FrameRequest>&);

	// change values with new asset:
	QSharedPointer<AS_02::JP2K::MXFReader> reader;
	Metadata::eColorEncoding ColorEncoding;
	Metadata::eColorSpace ColorSpace;
	int prec, prec_shift, max;
	float adjustYCbCr;
	int layer = 3; // default

private:

	typedef struct
	{
		OPJ_UINT8* pData; //Our data.
		OPJ_SIZE_T dataSize; //How big is our data.
		OPJ_SIZE_T offset; //Where are we currently in our data.
	}opj_memory_stream;

	QSharedPointer<DecodedFrames> decoded_shared;
	QSharedPointer<FrameRequest> request;

	int *decoded_image_counter;

	opj_memory_stream pMemoryStream;

	// DataToQImage
	bool convert_to_709 = false;
	bool convert_neccessary = false;

	unsigned char *img_buff;
	void DataToQImage(); // converts opj_image_t -> QImage
	int w, h, xpos, buff_pos, x, y, bytes_per_line;
	float Y, Cb, Cr, r, g, b, out_r, out_g, out_b, out_r8, out_g8, out_b8;

	// memory stream methods
	static OPJ_SIZE_T opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data);
	static OPJ_SIZE_T opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data);
	static OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data);
	static OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data);
	static void opj_memory_stream_do_nothing(void * p_user_data);
	opj_stream_t* opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream);

	ASDCP::JP2K::FrameBuffer *buff;

	void processFinished(QImage img);
	opj_image_t *psImage = NULL;
	opj_codec_t *pDecompressor = NULL;
	opj_dparameters_t params; // decoding parameters
	opj_stream_t *pStream;

	int error = 0; // 0 = no error

protected:
	void run();
signals:
	void finished();
public slots:
	void ended(){

		qDebug() << "thread terminated!!";
	}
};
