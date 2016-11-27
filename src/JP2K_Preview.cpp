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
#include "JP2K_Preview.h"
#include <QThread>
#include <QTime>
#include "openjpeg.h"
#include "AS_DCP_internal.h"

// timeline preview constructor
JP2K_Preview::JP2K_Preview() {
	setUp();
}

JP2K_Preview::~JP2K_Preview()
{
	reader->Close();
	reader->~MXFReader();
	buff->~FrameBuffer();
	delete eotf;
	delete oetf;
}

void JP2K_Preview::setUp() {

	decode_time.start();
	dataToQImageTime.start();

	params.cp_reduce = 3; // (default)
	buff = new ASDCP::JP2K::FrameBuffer();
	cpus = opj_get_num_cpus();

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K);
	opj_codec_set_threads(pDecompressor, cpus);

	// Setup the decoder (first time), using user parameters
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		msg = "Error setting up decoder!"; // ERROR
		opj_destroy_codec(pDecompressor);
	}

	// create lookup tables
	float alpha = 1.099;
	float beta = 0.018;
	eotf = new float[4096];
	oetf = new float[4096];

	for (int i = 0; i < 4096; i++) {

		float input = (float) i / 4095; // convert input to value between 0...1

		// OETF -> linearize
		if (input < (4.5 * beta)) { // linear part!
			oetf[i] = input / 4.5;
		}
		else { // exp. part!
			oetf[i] = pow(((input + (alpha - 1)) / alpha), 2.222); // 0.45 th root of x: pow(val, 1.0 / 0.45) = pow(val, 2.222~)
		}

		// EOTF -> delinearize
		if (input < beta) { // linear part!
			eotf[i] = input * 4.5;
		}
		else { // exp. part!
			eotf[i] = alpha * pow(input, 0.45) - (alpha - 1);
		}
	}
}

void JP2K_Preview::getProxy() {

	params.cp_reduce = 4; // (default for proxy)
	QImage p1 = QImage(":/proxy_unknown.png");
	QImage p2 = QImage(":/proxy_unknown.png");

	setAsset(); // initialize reader

	// FIRST PROXY
	if (extractFrame(first_proxy)) { // frame extraction was successfull -> decode frame
		if (decodeImage()) { // try to decode image
			p1 = DataToQImage();
			cleanUp();
		}
	}

	// SECOND PROXY
	if (extractFrame(second_proxy)) { // frame extraction was successfull -> decode frame
		if (decodeImage()) { // try to decode image
			p2 = DataToQImage();
			cleanUp();
		}
	}

	emit proxyFinished(p1, p2);
}

// set decoding layer
void JP2K_Preview::setLayer(int index) {

	params.cp_reduce = index;

	// Setup the decoder (again), using user parameters
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		qDebug() << "Error setting up decoder";
		opj_destroy_codec(pDecompressor);
	}
}

void JP2K_Preview::setAsset() {

	if (asset && !asset.isNull()) {

		current_asset = asset;

		// get color space
		ColorEncoding = asset->GetMetadata().colorEncoding;
		ColorSpace = asset->GetMetadata().colorSpace;

		// check if color space conversion is neccesary
		if (ColorSpace == Metadata::eColorSpace::RGB709 || ColorSpace == Metadata::eColorSpace::YUV_709) {
			convert_neccessary = false;
		}
		else {
			convert_neccessary = true;
		}

		if (mxf_path != "") { // close old asset/reader
			reader->Close();
			reader->~MXFReader();
		}

		mxf_path = asset->GetPath().absoluteFilePath(); // get new path

														 // create new reader
		reader = new AS_02::JP2K::MXFReader();

		Result_t result_o = reader->OpenRead(mxf_path.toStdString()); // open file for reading
		if (!ASDCP_SUCCESS(result_o)) {
			msg = QString("Failed to init. reader: %1").arg(result_o.Message()); // ERROR
			err = true;
		}
	}
	else {
		msg = "Asset is invalid!"; // ERROR
		err = true;
	}
}

void JP2K_Preview::decode() {

	err = false; // reset
	decode_time.restart(); // start calculating decode time

	if (operator!=(asset, current_asset)) { // asset changed!!
		setAsset();
	}

	if (extractFrame(frameNr) && !err) { // frame extraction was successfull -> decode frame

		// try to decode image
		if (decodeImage() && !err) {
			emit ShowFrame(DataToQImage());

			if(!err) msg = QString("Decoded frame %1 in %2 ms").arg(frameNr).arg(decode_time.elapsed()); // no error

			emit decodingStatus(frameNr, msg);
			QApplication::processEvents();
			cleanUp();
		}
		else { // error decoding image
			emit ShowFrame(QImage(":/proxy_unknown.png"));
			emit decodingStatus(frameNr, msg);
			emit finished();
		}
	}
	else {
		emit ShowFrame(QImage(":/proxy_unknown.png"));
		emit decodingStatus(frameNr, msg);
		emit finished();
	}
}

bool JP2K_Preview::extractFrame(qint64 frameNr) {

	if (!asset) { // no asset was set -> abort
		err = "Asset not found!";
		err = true;
		return false;
	}

	// calculate neccessary buffer size
	Result_t result_f1 = reader->AS02IndexReader().Lookup(frameNr, f1); // current frame
	if (ASDCP_SUCCESS(result_f1)) {
		if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup((frameNr + 1), f2))) { // next frame
			buff->Capacity((f2.StreamOffset - f1.StreamOffset) - 20); // set buffer size
		}
		else {
			buff->Capacity(3000000); // default
		}

		// try reading requested frame number
		Result_t result = reader->ReadFrame(frameNr, *buff, NULL, NULL);
		if (ASDCP_SUCCESS(result)) {
			//&rRawData.append((const char*)buff->Data(), buff->Size()); // append data and return
			pMemoryStream.pData = (unsigned char*)buff->Data();
			pMemoryStream.dataSize = buff->Size();
			return true;
		}
		else {
			error = Error(result); // error reading frame
			qDebug() << "error reading frame" << frameNr;
			msg = "Error reading frame!"; // ERROR
			err = true;
			return false;
		}
	}
	else {
		qDebug() << "requested frame does not exist:" << frameNr;
		msg = QString("Frame does not exist: %1").arg(frameNr); // ERROR
		err = true;
		buff->Capacity(0);
		return false;
	}

	return false; // default
}

bool JP2K_Preview::decodeImage() {

	pMemoryStream.offset = 0;

	pStream = opj_stream_create_default_memory_stream(&pMemoryStream, OPJ_TRUE);

	//register callbacks (for debugging)
	//OPENJPEG_H::opj_set_info_handler(pDecompressor, info_callback, 0);
	//OPENJPEG_H::opj_set_warning_handler(pDecompressor, warning_callback);
	//OPENJPEG_H::opj_set_error_handler(pDecompressor, error_callback);

	// try reading header
	if (!OPENJPEG_H::opj_read_header(pStream, pDecompressor, &psImage))
	{
		OPENJPEG_H::opj_stream_destroy(pStream);
		OPENJPEG_H::opj_destroy_codec(pDecompressor);
		OPENJPEG_H::opj_image_destroy(psImage);
		msg = "Failed to read image header!"; // ERROR
		err = true;
		psImage = NULL; // reset decoded output stream

		pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser
	}
	else { // try decoding image
		if (!(OPENJPEG_H::opj_decode(pDecompressor, pStream, psImage)))
		{
			OPENJPEG_H::opj_stream_destroy(pStream);
			OPENJPEG_H::opj_destroy_codec(pDecompressor);
			OPENJPEG_H::opj_image_destroy(psImage);

			psImage = NULL; // reset decoded output stream
			pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser
			msg = "Failed to decode image!"; // ERROR
			err = true;
			return false;
		}
		else {
			return true;
		}
	}

	msg = "There was an error decoding the image";
	err = true;
	return false;
}

void JP2K_Preview::cleanUp() {
	OPENJPEG_H::opj_stream_destroy(pStream);
	OPENJPEG_H::opj_destroy_codec(pDecompressor);
	OPENJPEG_H::opj_image_destroy(psImage); // free allocated memory
	//psImage = NULL; // reset decoded output stream

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser

	// Setup the decoder (again)
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		qDebug() << "Error setting up decoder!";
		opj_destroy_codec(pDecompressor);
	}

	opj_codec_set_threads(pDecompressor, cpus); // set nr. of cores in new decoder
	delete img_buff; // clear char buffer

	emit finished();
}

void JP2K_Preview::save2File() {
	QFile file("D:/Master/Thesis/frame1.jp2");
	file.open(QIODevice::WriteOnly);
	//file.write(rRawData);
	file.close();
}

void JP2K_Preview::info_callback(const char *msg, void *client_data) {
	qDebug() << "INFO" << msg;
}

void JP2K_Preview::warning_callback(const char *msg, void *client_data) {
	qDebug() << "WARNING" << msg;
}

void JP2K_Preview::error_callback(const char *msg, void *client_data) {
	qDebug() << "ERROR" << msg;
}

QImage JP2K_Preview::DataToQImage()
{
	dataToQImageTime.restart();

	w = psImage->comps->w;
	h = psImage->comps->h;
	prec = psImage->comps->prec; // 8 bit, 10 bit...
	prec_shift = prec - 8;
	float max = pow(2, prec);

	int adjustYCbCr;
	if (prec == 8) {
		adjustYCbCr = 128;
	}
	else if (prec == 10) {
		adjustYCbCr = 512;
	}
	else if (prec == 12) {
		adjustYCbCr = 1024;
	}

	xpos, buff_pos;
	QImage image(w, h, QImage::Format_RGB888); // create image

	bytes_per_line = w * 3;
	img_buff = new unsigned char[bytes_per_line];

	if (convert_to_709 && convert_neccessary) {
		// change source encoding to REC.709
		switch (ColorSpace) {
		case Metadata::eColorSpace::YUV_2020_PQ:
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {

					xpos = (int)(y*w + x) / 2; // don't calculate twice
					buff_pos = x * 3; // don't calculate 3 times

					// get Yuv & convert to RGB
					Y = (float)psImage->comps[0].data[(int)(y*w + x)];
					Cb = (float)psImage->comps[1].data[xpos];
					Cr = (float)psImage->comps[2].data[xpos];

					r = (Y + 1.402 * (Cr - adjustYCbCr));
					g = (Y - 0.3441 * (Cb - adjustYCbCr) - (0.7141 * (Cr - adjustYCbCr))); 
					b = (Y + 1.772 * (Cb - adjustYCbCr));

					// clamp between 0... (2^n) - 1
					if (out_r < 0) { out_r = 0; }
					else if (out_r > max) { out_r = max; }
					if (out_g < 0) { out_g = 0; }
					else if (out_g > max) { out_g = max; }
					if (out_b < 0) { out_b = 0; }
					else if (out_b > max) { out_b = max; }

					// linearize (results in values between 0...1)
					r = oetf[(int)((r / max) * 4095.0f)];
					g = oetf[(int)((g / max) * 4095.0f)];
					b = oetf[(int)((b / max) * 4095.0f)];

					// Inverse of BT.2020 -> BT.709
					out_r = r*1.6605 + g*-0.5877 + b*-0.0728;
					out_g = r*-0.1246 + g*1.133 + b*-0.0084;
					out_b = r*-0.0182 + g*-0.1006 + b*1.1187;

					// clamp between 0...1
					if (out_r < 0) { out_r = 0; }
					else if (out_r > 1) { out_r = 1; }
					if (out_g < 0) { out_g = 0; }
					else if (out_g > 1) { out_g = 1; }
					if (out_b < 0) { out_b = 0; }
					else if (out_b > 1) { out_b = 1; }

					// unlinearize
					out_r = eotf[(int)(out_r * 4095.0f)] * max;
					out_g = eotf[(int)(out_g * 4095.0f)] * max;
					out_b = eotf[(int)(out_b * 4095.0f)] * max;

					// convert to 8 bit
					out_r8 = (int)out_r >> prec_shift;
					out_g8 = (int)out_g >> prec_shift;
					out_b8 = (int)out_b >> prec_shift;

					// clamp values between 0...255
					if (out_r8 < 0) { out_r8 = 0; }
					else if (out_r8 > 255) { out_r8 = 255; }
					if (out_g8 < 0) { out_g8 = 0; }
					else if (out_g8 > 255) { out_g8 = 255; }
					if (out_b8 < 0) { out_b8 = 0; }
					else if (out_b8 > 255) { out_b8 = 255; }

					// set data
					img_buff[buff_pos]     = (unsigned char)out_r8;
					img_buff[buff_pos + 1] = (unsigned char)out_g8;
					img_buff[buff_pos + 2] = (unsigned char)out_b8;
				}
				memcpy(image.scanLine(y), img_buff, bytes_per_line);
			}
			break;
		case Metadata::eColorSpace::YUV_2020_LIN:




			break;
		case Metadata::eColorSpace::RGB_2020_PQ:

			break;
		case Metadata::eColorSpace::RGB_P3D65:

			break;
		}
	}
	else {
		// RGB: keep original color encoding, or color encoding == REC.709
		if (ColorEncoding == Metadata::eColorEncoding::RGBA) {
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					xpos = (y*w + x); // don't calculate 3 times
					buff_pos = x * 3; // don't calculate 3 times

					img_buff[buff_pos] = psImage->comps[0].data[xpos] >> prec_shift;
					img_buff[buff_pos + 1] = psImage->comps[1].data[xpos] >> prec_shift;
					img_buff[buff_pos + 2] = psImage->comps[2].data[xpos] >> prec_shift;
				}
				memcpy(image.scanLine(y), img_buff, bytes_per_line);
			}
		}
		// Yuv:  keep original color encoding, or color encoding == REC.709
		else if (ColorEncoding == Metadata::eColorEncoding::CDCI) {
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					xpos = (int)(y*w + x) / 2; // don't calculate twice
					buff_pos = x * 3; // don't calculate 3 times

					// get Yuv & convert x bit -> 8 bit
					Y = psImage->comps[0].data[(int)(y*w + x)] >> prec_shift;
					Cb = psImage->comps[1].data[xpos] >> prec_shift;
					Cr = psImage->comps[2].data[xpos] >> prec_shift;

					// convert to rgb (http://softpixel.com/~cwright/programming/colorspace/yuv/)
					r = (int)(Y + 1.4075 * (Cr - 128));
					b = (int)(Y + 1.7790 * (Cb - 128));
					g = (int)(Y - 0.3455 * (Cb - 128) - (0.7169 * (Cr - 128)));

					// clamp values
					if (r < 0) { r = 0; }
					else if (r > 255) { r = 255; }
					if (g < 0) { g = 0; }
					else if (g > 255) { g = 255; }
					if (b < 0) { b = 0; }
					else if (b > 255) { b = 255; }

					img_buff[buff_pos] = r;
					img_buff[buff_pos + 1] = g;
					img_buff[buff_pos + 2] = b;
				}
				memcpy(image.scanLine(y), img_buff, bytes_per_line);
			}
		}
	}

	qDebug() << "DATA2RGB" << dataToQImageTime.elapsed();

	return image;
}

/* following code by Bruce Barton, see: https://groups.google.com/forum/#!searchin/openjpeg/How$20to$20replace$20file_stream$20by$20byte_stream$3F%7Csort:relevance/openjpeg/8cebr0u7JgY/NSieKiI0hVQJ */
//This will read from our memory to the buffer.
OPJ_SIZE_T JP2K_Preview::opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;//Our data.
	OPJ_SIZE_T l_nb_bytes_read = p_nb_bytes;//Amount to move to buffer.

	//Check if the current offset is outside our data buffer.
	if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T)-1;

	//Check if we are reading more than we have.
	if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset))
		l_nb_bytes_read = l_memory_stream->dataSize - l_memory_stream->offset;//Read all we have.

		//Copy the data to the internal buffer.
		memcpy(p_buffer, &(l_memory_stream->pData[l_memory_stream->offset]), l_nb_bytes_read);
		l_memory_stream->offset += l_nb_bytes_read;//Update the pointer to the new location.

	return l_nb_bytes_read;
}

//This will write from the buffer to our memory.
OPJ_SIZE_T JP2K_Preview::opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;//Our data.
	OPJ_SIZE_T l_nb_bytes_write = p_nb_bytes;//Amount to move to buffer.

	//Check if the current offset is outside our data buffer.
	if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T)-1;

	//Check if we are write more than we have space for.
	if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset))
		l_nb_bytes_write = l_memory_stream->dataSize - l_memory_stream->offset;//Write the remaining space.

	//Copy the data from the internal buffer.
	memcpy(&(l_memory_stream->pData[l_memory_stream->offset]), p_buffer, l_nb_bytes_write);

	l_memory_stream->offset += l_nb_bytes_write;//Update the pointer to the new location.

	return l_nb_bytes_write;
}

//Moves the pointer forward, but never more than we have.
OPJ_OFF_T JP2K_Preview::opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;
	OPJ_SIZE_T l_nb_bytes;
	if (p_nb_bytes < 0) return -1;//No skipping backwards.

	l_nb_bytes = (OPJ_SIZE_T)p_nb_bytes;//Allowed because it is positive.

	// Do not allow jumping past the end.
	if (l_nb_bytes > l_memory_stream->dataSize - l_memory_stream->offset)
		l_nb_bytes = l_memory_stream->dataSize - l_memory_stream->offset;//Jump the max.

	//Make the jump.
	l_memory_stream->offset += l_nb_bytes;

	//Returm how far we jumped.
	return l_nb_bytes;
}

//Sets the pointer to anywhere in the memory.
OPJ_BOOL JP2K_Preview::opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;
	if (p_nb_bytes < 0) return OPJ_FALSE;//No before the buffer.
	if (p_nb_bytes > (OPJ_OFF_T)l_memory_stream->dataSize) return OPJ_FALSE;//No after the buffer.
	l_memory_stream->offset = (OPJ_SIZE_T)p_nb_bytes;//Move to new position.

	return OPJ_TRUE;
}

//The system needs a routine to do when finished, the name tells you what I want it to do.
void JP2K_Preview::opj_memory_stream_do_nothing(void * p_user_data)
{
	OPJ_ARG_NOT_USED(p_user_data);
}

//Create a stream to use memory as the input or output.
opj_stream_t* JP2K_Preview::opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream)
{
	opj_stream_t* l_stream;
	if (!(l_stream = opj_stream_default_create(p_is_read_stream))) return (NULL);

	//Set how to work with the frame buffer.
	if (p_is_read_stream)
		opj_stream_set_read_function(l_stream, opj_memory_stream_read);
	else
		opj_stream_set_write_function(l_stream, opj_memory_stream_write);

	opj_stream_set_seek_function(l_stream, opj_memory_stream_seek);
	opj_stream_set_skip_function(l_stream, opj_memory_stream_skip);
	opj_stream_set_user_data(l_stream, p_memoryStream, opj_memory_stream_do_nothing);
	opj_stream_set_user_data_length(l_stream, p_memoryStream->dataSize);

	return l_stream;
}
