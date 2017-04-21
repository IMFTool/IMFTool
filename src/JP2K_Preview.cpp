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
	if(!current_asset.isNull())
	{
		reader->Close();
		reader->~MXFReader();
	}else
	{
		qDebug() << "no reader found!";
	}
	
	delete eotf_709;
	delete oetf_709;
	delete eotf_2020;
	delete oetf_2020;
	delete eotf_PQ;
	delete oetf_PQ;
}

void JP2K_Preview::setUp() {

	mDecode_time.start();

	params.cp_reduce = 3; // (default)
	mCpus = opj_get_num_cpus();

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K);

	opj_codec_set_threads(pDecompressor, mCpus);

	// Setup the decoder (first time), using user parameters
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		mMsg = "Error setting up decoder!"; // ERROR
		opj_destroy_codec(pDecompressor);
	}

	// create lookup tables
	max_f = 1 << bitdepth;
	max_f_ = (float)(max_f)-1.0;

	eotf_709 = new float[max_f];
	oetf_709 = new float[max_f];

	float alpha = 1.09929682680944;
	float beta = 0.018053968510807;
	eotf_2020 = new float[max_f];
	oetf_2020 = new float[max_f];

	float m1 = 0.1593017578125;
	float m2 = 78.84375;
	float c1 = 0.8359375;
	float c2 = 18.8515625;
	float c3 = 18.6875;
	eotf_PQ = new float[max_f];
	oetf_PQ = new float[max_f];

	for (int i = 0; i < max_f; i++) {

		float input = (float)(i / max_f_); // convert input to value between 0...1

		// BT.709 - OETF
		oetf_709[i] = pow(input, 1.0f / 2.4f);

		/*
		// BT.709 - EOTF
		if (input < (4.5 * 0.018)) {
			eotf_709[i] = input / 4.5;
		}
		else {
			eotf_709[i] = pow(((input + (alpha - 1)) / alpha), 1.0 / 0.45);
		}

		// BT.2020 - OETF
		if (input < beta) {
			oetf_2020[i] = 4.5 * input;
		}
		else {
			oetf_2020[i] = alpha * pow(input, 0.45) - (alpha - 1);
		}
		*/

		// BT.2020 - EOTF
		if (input < (4.5 * beta)) {
			eotf_2020[i] = input / 4.5;
		}
		else {
			eotf_2020[i] = pow(((input + (alpha - 1)) / alpha), 1.0 / 0.45);
		}

		// SMPTE ST 2084 (PQ)
		eotf_PQ[i] = pow(((pow(input, (1.0 / m2)) - c1)) / (c2 - c3 *pow(input, (1.0 / m2))), 1.0 / m1) * 10000;
		//oetf_PQ[i] = pow((c1 + c2*pow(input, m1)) / (1 + c3*pow(input, m1)), m2);

	}

	eotf_PQ[0] = 0;
	oetf_PQ[0] = 0;

}

void JP2K_Preview::getProxy() {

	mCpus = 1; // (default for proxys)
	convert_to_709 = false; // (default for proxys)
	params.cp_reduce = 4; // (default for proxy)

	setAsset(); // initialize reader
	QImage p1, p2;

	// FIRST PROXY
	if (!err && extractFrame(mFirst_proxy)) { // frame extraction was successfull -> decode frame
		if (decodeImage()) { // try to decode image
			p1 = DataToQImage();
			cleanUp();
		}
		else {
			p1 = QImage(":/proxy_unknown.png");
		}
	}
	else {
		p1 = QImage(":/proxy_unknown.png");
	}
	
	// SECOND PROXY
	if (!err && extractFrame(mSecond_proxy)) { // frame extraction was successful -> decode frame
		if (decodeImage()) { // try to decode image
			p2 = DataToQImage();
			cleanUp();
		}
		else {
			p2 = QImage(":/proxy_unknown.png");
		}
	}
	else {
		p2 = QImage(":/proxy_unknown.png");
	}

	emit proxyFinished(p1, p2);
	emit finished();
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
		colorPrimaries = asset->GetMetadata().colorPrimaries;
		transferCharactersitics = asset->GetMetadata().transferCharcteristics;
		ColorEncoding = asset->GetMetadata().colorEncoding;
		src_bitdepth = asset->GetMetadata().componentDepth;
		//WR
		ComponentMinRef = asset->GetMetadata().componentMinRef;
		ComponentMaxRef = asset->GetMetadata().componentMaxRef;
		//WR

		if (ComponentMinRef && ComponentMaxRef) {
			RGBrange = ComponentMaxRef - ComponentMinRef;
			RGBmaxcv = (1 << src_bitdepth) - 1;
		}

		prec_shift = src_bitdepth - 8;
		max = (1 << src_bitdepth) - 1;

		switch (colorPrimaries) {
		case SMPTE::ColorPrimaries_ITU709:
			// set YCbCr -> RGB conversion parameters
			Kr = 0.2126;
			Kg = 0.7152;
			Kb = 0.0722;
			break;
		case SMPTE::ColorPrimaries_ITU2020:
			// set YCbCr -> RGB conversion parameters
			Kr = 0.2627;
			Kg = 0.6780;
			Kb = 0.0593;
			break;
		case SMPTE::ColorPrimaries_SMPTE170M:
		case SMPTE::ColorPrimaries_ITU470_PAL:
			// set YCbCr -> RGB conversion parameters
			Kr = 0.299;
			Kg = 0.587;
			Kb = 0.114;
			break;
		case SMPTE::ColorPrimaries_P3D65:
			//P365 is 4:4:4 only
			break;
		default:
			mMsg = "Unknown color encoding"; // ERROR
			err = true;
			break; // abort!
		}

		if (mMxf_path != "") { // close old asset/reader
			reader->Close();
			reader->~MXFReader();
		}

		mMxf_path = asset->GetPath().absoluteFilePath(); // get new path

		reader = new AS_02::JP2K::MXFReader(); // create new reader

		Result_t result_o = reader->OpenRead(mMxf_path.toStdString()); // open file for reading
		if (!ASDCP_SUCCESS(result_o)) {
			mMsg = QString("Failed to init. reader: %1").arg(result_o.Message()); // ERROR
			err = true;
		}
	}
	else {
		mMsg = "Asset is invalid!"; // ERROR
		err = true;
	}
}

void JP2K_Preview::decode() {

#ifdef DEBUG_JP2K
	qDebug() << "begin decoding image nr. " << mFrameNr;
#endif

	//register callbacks (for debugging)
#ifdef DEBUG_JP2K
	OPENJPEG_H::opj_set_info_handler(pDecompressor, info_callback, 0);
	OPENJPEG_H::opj_set_warning_handler(pDecompressor, warning_callback, 0);
	OPENJPEG_H::opj_set_error_handler(pDecompressor, error_callback, 0);
#endif

	err = false; // reset
	mDecode_time.restart(); // start calculating decode time

	if (operator!=(asset, current_asset) || !asset) { // asset changed!!
		setAsset();
	}

	if (!err && extractFrame(mFrameNr)) { // frame extraction was successfull -> decode frame

		// try to decode image
		if (decodeImage() && !err) {

			emit ShowFrame(DataToQImage());

			if (!err) mMsg = QString("Decoded frame %1 in %2 ms").arg(mFrameNr).arg(mDecode_time.elapsed()); // no error

			emit decodingStatus(mFrameNr, mMsg);
			QApplication::processEvents();
			cleanUp();
			emit finished();
		}
		else { // error decoding image
			emit ShowFrame(QImage(":/frame_error.png"));
			emit decodingStatus(mFrameNr, mMsg);
			emit finished();
		}
	}
	else {
		emit ShowFrame(QImage(":/frame_blank.png")); // show empty image
		emit decodingStatus(mFrameNr, mMsg);
		emit finished();
	}
}

bool JP2K_Preview::extractFrame(qint64 frameNr) {

	// create new buffer
	buff = new ASDCP::JP2K::FrameBuffer();

	// calculate neccessary buffer size
	if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup((frameNr + 1), IndexF2))) { // next frame
		if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup(frameNr, IndexF1))) { // current frame
			if (!ASDCP_SUCCESS(buff->Capacity((IndexF2.StreamOffset - IndexF1.StreamOffset) - 20))) { // set buffer size

				mMsg = QString("Could not set FrameBuffer size to: %1").arg((IndexF2.StreamOffset - IndexF1.StreamOffset) - 20); // ERROR
				err = true;
				return false;
			}
		}
		else {
			mMsg = QString("Frame does not exist: %1").arg(frameNr); // ERROR
			err = true;
			return false;
		}
	}
	else {
		buff->Capacity(default_buffer_size); // set default size
	}
	
	// try reading requested frame number
	if (ASDCP_SUCCESS(reader->ReadFrame(frameNr, *buff, NULL, NULL))) {
		pMemoryStream.pData = (unsigned char*)buff->Data();
		pMemoryStream.dataSize = buff->Size();
		return true;
	}
	else {
		mMsg = "Error reading frame!"; // ERROR
		err = true;
		return false;
	}

	return false; // default
}

bool JP2K_Preview::decodeImage() {

	pMemoryStream.offset = 0;

	pStream = opj_stream_create_default_memory_stream(&pMemoryStream, OPJ_TRUE);

	// try reading header
	if (!OPENJPEG_H::opj_read_header(pStream, pDecompressor, &psImage))
	{
		OPENJPEG_H::opj_stream_destroy(pStream);
		OPENJPEG_H::opj_destroy_codec(pDecompressor);
		mMsg = "Failed to read image header!"; // ERROR
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
			buff->~FrameBuffer();

			psImage = NULL; // reset decoded output stream
			pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser
			mMsg = "Failed to decode image!"; // ERROR
			err = true;
			return false;
		}
		else {
			buff->~FrameBuffer();
			return true;
		}
	}

	mMsg = "There was an error decoding the image";
	err = true;
	return false;
}

void JP2K_Preview::cleanUp() {

	OPENJPEG_H::opj_stream_destroy(pStream);
	OPENJPEG_H::opj_destroy_codec(pDecompressor);
	OPENJPEG_H::opj_image_destroy(psImage); // free allocated memory

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser

	// Setup the decoder (again)
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		qDebug() << "Error setting up decoder!";
		opj_destroy_codec(pDecompressor);
	}

	opj_codec_set_threads(pDecompressor, mCpus); // set nr. of cores in new decoder
}

void JP2K_Preview::save2File() {
	QFile file("D:/Master/Thesis/frame1.jp2");
	file.open(QIODevice::WriteOnly);
	//file.write(rRawData);
	file.close();
}

QImage JP2::DataToQImage()
{
	w = psImage->comps->w;
	h = psImage->comps->h;

	QImage image(w, h, QImage::Format_RGB888); // create image

	bytes_per_line = w * 3;
	img_buff = new unsigned char[bytes_per_line];

	int offset = 16 << (src_bitdepth - 8);
	int maxcv = (1 << src_bitdepth) - 1;
	int maxcv_plus_1 = maxcv + 1;
	int range_y = 219 << (src_bitdepth - 8);
	int range_c = (maxcv_plus_1 - 2 * offset);
	if (convert_to_709) {

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {

				switch (ColorEncoding) {
				case Metadata::RGBA:

					xpos = (y*w + x); // don't calculate 3 times

					// get RGB																
					r = (float)psImage->comps[0].data[xpos];
					g = (float)psImage->comps[1].data[xpos];
					b = (float)psImage->comps[2].data[xpos];

					//WR
					if (ComponentMinRef && ComponentMaxRef) { // QE.2 If ComponentMinRef is != 0, it's Legal Range. Don't convert if ComponentMaxRef is (accidentally) zero, or not set.
						r = (r - ComponentMinRef) / (ComponentMaxRef - ComponentMinRef) * maxcv;
						g = (g - ComponentMinRef) / (ComponentMaxRef - ComponentMinRef) * maxcv;
						b = (b - ComponentMinRef) / (ComponentMaxRef - ComponentMinRef) * maxcv;
						if (r < 0) r = 0;
						if (g < 0) g = 0;
						if (b < 0) b = 0;
					}
					//WR
					break;
				case Metadata::CDCI:

					xpos = (int)(y*w + x) / 2; // don't calculate twice

					// get YCbCr values
					Y = (float)psImage->comps[0].data[(int)(y*w + x)];
					Cb = (float)psImage->comps[1].data[xpos];
					Cr = (float)psImage->comps[2].data[xpos];

					// convert to rgb
					r = (Y - offset)*maxcv / range_y + 2 * (1 - Kr)*(Cr - maxcv_plus_1 / 2)*maxcv / range_c;
					g = (Y - offset)*maxcv / range_y - 2 * Kb*(1 - Kb) / Kg*(Cb - maxcv_plus_1 / 2)*maxcv / range_c - 2 * Kr*(1 - Kr) / Kg*(Cr - maxcv_plus_1 / 2)*maxcv / range_c;
					b = (Y - offset)*maxcv / range_y + 2 * (1 - Kb)*(Cb - maxcv_plus_1 / 2)*maxcv / range_c;

					// clamp between 0...CVmax
					if (r < 0) { r = 0; }
					else if (r >= maxcv) { r = maxcv; }
					if (g < 0) { g = 0; }
					else if (g >= maxcv) { g = maxcv; }
					if (b < 0) { b = 0; }
					else if (b >= maxcv) { b = maxcv; }
					break;
				default: return QImage(":/frame_error.png"); break; // abort!
				}

				// now we have rgb data -> linearize the data!
				switch (transferCharactersitics) {
				case SMPTE::TransferCharacteristic_ITU709:

					// convert to 8 bit
					out_r8 = (int)r >> prec_shift;
					out_g8 = (int)g >> prec_shift;
					out_b8 = (int)b >> prec_shift;

					// set data
					buff_pos = x * 3; // don't calculate 3 times
					img_buff[buff_pos] = (unsigned char)out_r8;
					img_buff[buff_pos + 1] = (unsigned char)out_g8;
					img_buff[buff_pos + 2] = (unsigned char)out_b8;

					continue; // jump to next pixel

					break;
				case SMPTE::TransferCharacteristic_ITU2020:

					r = eotf_2020[(int)((r / max) * max_f_)]; // 0...1
					g = eotf_2020[(int)((g / max) * max_f_)]; // 0...1
					b = eotf_2020[(int)((b / max) * max_f_)]; // 0...1

					break;
				case SMPTE::TransferCharacteristic_SMPTEST2084:

					r = eotf_PQ[(int)((r / max) * max_f_)]; // 0...10 000
					g = eotf_PQ[(int)((g / max) * max_f_)]; // 0...10 000
					b = eotf_PQ[(int)((b / max) * max_f_)]; // 0...10 000

					// convert to 0.0..100.0 ( 1.0 = 100 nits)
					r /= 100;
					g /= 100;
					b /= 100;

					break;
				case SMPTE::TransferCharacteristic_IEC6196624_xvYCC:

					// convert to 8 bit
					out_r8 = (int)r >> prec_shift;
					out_g8 = (int)g >> prec_shift;
					out_b8 = (int)b >> prec_shift;

					// clamp values between 0...255
					if (out_r8 < 0) { out_r8 = 0; }
					else if (out_r8 > 255) { out_r8 = 255; }
					if (out_g8 < 0) { out_g8 = 0; }
					else if (out_g8 > 255) { out_g8 = 255; }
					if (out_b8 < 0) { out_b8 = 0; }
					else if (out_b8 > 255) { out_b8 = 255; }

					// set data
					buff_pos = x * 3; // don't calculate 3 times
					img_buff[buff_pos] = (unsigned char)out_r8;
					img_buff[buff_pos + 1] = (unsigned char)out_g8;
					img_buff[buff_pos + 2] = (unsigned char)out_b8;

					continue; // jump to next pixel

					break;
				default: return QImage(":/frame_error.png"); break; // abort!
				}

				switch (colorPrimaries) {
				case SMPTE::ColorPrimaries_ITU2020:

					// convert from BT.2020 -> BT.709
					out_r = r*1.6605 + g*-0.5877 + b*-0.0728;
					out_g = r*-0.1246 + g*1.1330 + b*-0.0084;
					out_b = r*-0.0182 + g*-0.1006 + b*1.1187;

					break;
				case SMPTE::ColorPrimaries_P3D65:

					// convert from DCI-P3 -> BT.709
					out_r = r*1.2248 - g*0.2249 - b*0.0001;
					out_g = -r*0.042 + g*1.042;
					out_b = -r*0.0196 - g*0.0786 + b*1.0983;

					break;
				default: return QImage(":/frame_error.png"); break; // abort!
				}

				// clamp between 0...1
				if (out_r < 0) { out_r = 0; }
				else if (out_r > 1) { out_r = 1; }
				if (out_g < 0) { out_g = 0; }
				else if (out_g > 1) { out_g = 1; }
				if (out_b < 0) { out_b = 0; }
				else if (out_b > 1) { out_b = 1; }

				// unlinearize
				out_r = oetf_709[(int)(out_r * max_f_)] * max;
				out_g = oetf_709[(int)(out_g * max_f_)] * max;
				out_b = oetf_709[(int)(out_b * max_f_)] * max;

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
				buff_pos = x * 3; // don't calculate 3 times
				img_buff[buff_pos] = (unsigned char)out_r8;
				img_buff[buff_pos + 1] = (unsigned char)out_g8;
				img_buff[buff_pos + 2] = (unsigned char)out_b8;
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	else if (ColorEncoding == Metadata::eColorEncoding::RGBA) {
		// RGB: keep original color encoding, or color encoding == REC.709

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
				Y = psImage->comps[0].data[(int)(y*w + x)];
				Cb = psImage->comps[1].data[xpos];
				Cr = psImage->comps[2].data[xpos];

				// convert to rgb
				r = (Y - offset)*maxcv / range_y + 2 * (1 - Kr)          *(Cr - maxcv_plus_1 / 2)*maxcv / range_c;
				g = (Y - offset)*maxcv / range_y - 2 * Kb*(1 - Kb) / Kg*(Cb - maxcv_plus_1 / 2)*maxcv / range_c - 2 * Kr*(1 - Kr) / Kg*(Cr - maxcv_plus_1 / 2)*maxcv / range_c;
				b = (Y - offset)*maxcv / range_y + 2 * (1 - Kb)          *(Cb - maxcv_plus_1 / 2)*maxcv / range_c;

				out_r8 = (int)r >> prec_shift;
				out_g8 = (int)g >> prec_shift;
				out_b8 = (int)b >> prec_shift;

				// clamp values between 0...255
				if (out_r8 < 0) { out_r8 = 0; }
				else if (out_r8 > 255) { out_r8 = 255; }
				if (out_g8 < 0) { out_g8 = 0; }
				else if (out_g8 > 255) { out_g8 = 255; }
				if (out_b8 < 0) { out_b8 = 0; }
				else if (out_b8 > 255) { out_b8 = 255; }

				// set data
				buff_pos = x * 3; // don't calculate 3 times
				img_buff[buff_pos] = (unsigned char)out_r8;
				img_buff[buff_pos + 1] = (unsigned char)out_g8;
				img_buff[buff_pos + 2] = (unsigned char)out_b8;
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	else { // unknown ColorEncoding
		return QImage(":/frame_error.png");
	}

	delete img_buff; // clear char buffer

	return image;
}

/* following code by Bruce Barton, see: https://groups.google.com/forum/#!searchin/openjpeg/How$20to$20replace$20file_stream$20by$20byte_stream$3F%7Csort:relevance/openjpeg/8cebr0u7JgY/NSieKiI0hVQJ */
// This will read from our memory to the buffer.
OPJ_SIZE_T JP2::opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data; // Our data.
	OPJ_SIZE_T l_nb_bytes_read = p_nb_bytes; // Amount to move to buffer.

											 // Check if the current offset is outside our data buffer.
	if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T)-1;

	// Check if we are reading more than we have.
	if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset))
		l_nb_bytes_read = l_memory_stream->dataSize - l_memory_stream->offset; // Read all we have.

																			   // Copy the data to the internal buffer.
	memcpy(p_buffer, &(l_memory_stream->pData[l_memory_stream->offset]), l_nb_bytes_read);
	l_memory_stream->offset += l_nb_bytes_read; // Update the pointer to the new location.

	return l_nb_bytes_read;
}

// This will write from the buffer to our memory.
OPJ_SIZE_T JP2::opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data; // Our data.
	OPJ_SIZE_T l_nb_bytes_write = p_nb_bytes; // Amount to move to buffer.

											  // Check if the current offset is outside our data buffer.
	if (l_memory_stream->offset >= l_memory_stream->dataSize) return (OPJ_SIZE_T)-1;

	// Check if we are write more than we have space for.
	if (p_nb_bytes > (l_memory_stream->dataSize - l_memory_stream->offset))
		l_nb_bytes_write = l_memory_stream->dataSize - l_memory_stream->offset; // Write the remaining space.

																				// Copy the data from the internal buffer.
	memcpy(&(l_memory_stream->pData[l_memory_stream->offset]), p_buffer, l_nb_bytes_write);

	l_memory_stream->offset += l_nb_bytes_write; // Update the pointer to the new location.

	return l_nb_bytes_write;
}

//Moves the pointer forward, but never more than we have.
OPJ_OFF_T JP2::opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;
	OPJ_SIZE_T l_nb_bytes;
	if (p_nb_bytes < 0) return -1; // No skipping backwards.

	l_nb_bytes = (OPJ_SIZE_T)p_nb_bytes; // Allowed because it is positive.

										 // Do not allow jumping past the end.
	if (l_nb_bytes > l_memory_stream->dataSize - l_memory_stream->offset)
		l_nb_bytes = l_memory_stream->dataSize - l_memory_stream->offset; // Jump the max.

																		  // Make the jump.
	l_memory_stream->offset += l_nb_bytes;

	// Returm how far we jumped.
	return l_nb_bytes;
}

// Sets the pointer to anywhere in the memory.
OPJ_BOOL JP2::opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;
	if (p_nb_bytes < 0) return OPJ_FALSE; // No before the buffer.
	if (p_nb_bytes >(OPJ_OFF_T)l_memory_stream->dataSize) return OPJ_FALSE; // No after the buffer.
	l_memory_stream->offset = (OPJ_SIZE_T)p_nb_bytes; // Move to new position.

	return OPJ_TRUE;
}

// The system needs a routine to do when finished, the name tells you what I want it to do.
void JP2::opj_memory_stream_do_nothing(void * p_user_data)
{
	OPJ_ARG_NOT_USED(p_user_data);
}

// Create a stream to use memory as the input or output.
opj_stream_t* JP2::opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream)
{
	opj_stream_t* l_stream;
	if (!(l_stream = opj_stream_default_create(p_is_read_stream))) return (NULL);

	// Set how to work with the frame buffer.
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

void JP2::info_callback(const char *mMsg, void *client_data) {
	qDebug() << "INFO" << mMsg;
}

void JP2::warning_callback(const char *mMsg, void *client_data) {
	qDebug() << "WARNING" << mMsg;
}

void JP2::error_callback(const char *mMsg, void *client_data) {
	qDebug() << "ERROR" << mMsg;
}


