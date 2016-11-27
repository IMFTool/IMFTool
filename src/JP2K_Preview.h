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
#include "ImfPackage.h"


class AssetMxfTrack;

typedef struct
{
	OPJ_UINT8* pData; //Our data.
	OPJ_SIZE_T dataSize; //How big is our data.
	OPJ_SIZE_T offset; //Where are we currently in our data.
}opj_memory_stream;

class JP2K_Preview : public QObject
{
	Q_OBJECT
private:

	void cleanUp();
	void setUp();

	// luts
	float *oetf;
	float *eotf;

	// data to qimage
	unsigned char *img_buff;
	int w, h, prec, prec_shift, xpos, buff_pos, x, y, bytes_per_line;
	float Y, Cb, Cr, r, g, b, out_r, out_g, out_b, out_r8, out_g8, out_b8;
	QImage DataToQImage(); // converts opj_image_t -> QImage

	// info methods
	static void info_callback(const char *msg, void *data);
	static void warning_callback(const char *msg, void *data);
	static void error_callback(const char *msg, void *data);

	// extract frame
	Error error;
	ASDCP::MXF::IndexTableSegment::IndexEntry f1; // current frame offset
	ASDCP::MXF::IndexTableSegment::IndexEntry f2; // next frame offset
	AS_02::JP2K::MXFReader *reader;
	ASDCP::JP2K::FrameBuffer *buff;
	QString mxf_path;
	int cpus = 0;

	// memory stream methods
	static OPJ_SIZE_T opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data);
	static OPJ_SIZE_T opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data);
	static OPJ_OFF_T opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data);
	static OPJ_BOOL opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data);
	static void opj_memory_stream_do_nothing(void * p_user_data);
	OPENJPEG_H::opj_stream_t* opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream);
	OPENJPEG_H::opj_image_t *psImage;
	OPENJPEG_H::opj_codec_t *pDecompressor;
	OPENJPEG_H::opj_stream_t *pStream;
	bool decodeImage();
	opj_memory_stream pMemoryStream;

	QTime decode_time;
	QTime dataToQImageTime; // TEMP
	QString msg;
	bool err = false; // error in the decoding process?

	// asset
	QSharedPointer<AssetMxfTrack> current_asset;
	void setAsset();

public:
	JP2K_Preview();
	~JP2K_Preview();

	QString path;
	qint64 frameNr;
	qint64 first_proxy;
	qint64 second_proxy;

	opj_dparameters_t params; // decoding parameters
	QSharedPointer<AssetMxfTrack> asset;
	Metadata::eColorEncoding ColorEncoding;
	Metadata::eColorSpace ColorSpace;

	bool convert_to_709 = true;
	bool convert_neccessary = false;
	bool extractFrame(qint64 frameNr);
	void save2File(); // save JP2K bytestream to file
	
signals:
	void proxyFinished(const QImage&, const QImage&); // finished generating proxy
	void ShowFrame(const QImage&);
	void decodingStatus(qint64, QString);
	void finished(); // everything, including cleanup is done...
public slots:
	void getProxy();
	void decode(); // decode J2K -> JPEG
	void setLayer(int);
};


