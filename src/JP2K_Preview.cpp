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
}

void JP2K_Preview::setUp() {

	mDecode_time.start();

	params.cp_reduce = 2; // (default)
	mCpus = opj_get_num_cpus();

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K);

	// Setup the decoder (first time), using user parameters
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		mMsg = "Error setting up decoder!"; // ERROR
		opj_destroy_codec(pDecompressor);
	}
	opj_codec_set_threads(pDecompressor, mCpus);

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
		transferCharacteristics = asset->GetMetadata().transferCharcteristics;
		ColorEncoding = asset->GetMetadata().colorEncoding;
		src_bitdepth = asset->GetMetadata().componentDepth;
		//WR
		ComponentMinRef = asset->GetMetadata().componentMinRef;
		ComponentMaxRef = asset->GetMetadata().componentMaxRef;
		//WR

		if (setCodingParameters() == false ) {
			mMsg = "Unknown color encoding"; // ERROR
			err = true;
		}

		if (mMxf_path != "") { // close old asset/reader
			reader->Close();
			reader->~MXFReader();
		}

		mMxf_path = asset->GetPath().absoluteFilePath(); // get new path

		reader = new AS_02::JP2K::MXFReader(defaultFactory); // create new reader

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
#ifdef DEBUG_JP2K
			qDebug() << "JP2K_Preview::decode" << mFrameNr;
#endif
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
			delete buff;

			psImage = NULL; // reset decoded output stream
			pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser
			mMsg = "Failed to decode image!"; // ERROR
			err = true;
			return false;
		}
		else {
			delete buff;
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

QImage JP2::DataToQImage()
{
	w = psImage->comps->w;
	h = psImage->comps->h;

	QImage image(w, h, QImage::Format_RGB888); // create image

	bytes_per_line = w * 3;
	img_buff = new unsigned char[bytes_per_line];

	if (convert_to_709) {

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				xpos = (y*w + x);
				switch (ColorEncoding) {
				case Metadata::RGBA:
					// get three color components
					cv_comp1 = psImage->comps[0].data[xpos];
					cv_comp2 = psImage->comps[1].data[xpos];
					cv_comp3 = psImage->comps[2].data[xpos];

					// Scale to full range 16 bit
					if (ComponentMinRef != 0) { //offset compensation for legal range signals
						cv_comp1 -= ComponentMinRef;
						cv_comp2 -= ComponentMinRef;
						cv_comp3 -= ComponentMinRef;
						cv_comp1 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						cv_comp2 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						cv_comp3 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
						if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
						if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;
					} else { // faster for full scale input
						cv_comp1 = cv_comp1 << (bitdepth - src_bitdepth);
						cv_comp2 = cv_comp2 << (bitdepth - src_bitdepth);
						cv_comp3 = cv_comp3 << (bitdepth - src_bitdepth);
						if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
						if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
						if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;
					}
					break;
				case Metadata::CDCI:
					index_422 = (y*w + x) / 2;
					cv_compY =  psImage->comps[0].data[xpos]  - offset_y;
					cv_compCb = psImage->comps[1].data[index_422]  - (max + 1) / 2;
					cv_compCr = psImage->comps[2].data[index_422]  - (max + 1) / 2;

					cv_compY =  cv_compY * max / range_y;
					cv_compCb = cv_compCb * max / range_c;
					cv_compCr = cv_compCr * max / range_c;

					cv_comp1 = cv_compY + ((2 * (1024 - Kr)*cv_compCr) / 1024);
					cv_comp2 = cv_compY - ((Kbg * cv_compCb + Krg * cv_compCr) / 1024);
					cv_comp3 = cv_compY + ((2 * (1024 - Kb)*cv_compCb) / 1024);

					if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max) cv_comp1 = max - 1;
					if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max) cv_comp2 = max - 1;
					if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max) cv_comp3 = max - 1;

					cv_comp1 = cv_comp1 << (bitdepth - src_bitdepth);
					cv_comp2 = cv_comp2 << (bitdepth - src_bitdepth);
					cv_comp3 = cv_comp3 << (bitdepth - src_bitdepth);
					break;
				default:
					return QImage(":/frame_error.png");  // abort!
				}

				if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
				if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
				if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;

				// linearize XYZ data
				if (linearize() == false) {
					qDebug() << "Unsupported Transfer Characteristic: " << SMPTE::vTransferCharacteristic[transferCharacteristics];
					return QImage(":/frame_error.png"); // abort!
				}
				if ((transferCharacteristics == SMPTE::TransferCharacteristic_ITU709) ||(transferCharacteristics == SMPTE::TransferCharacteristic_IEC6196624_xvYCC)) {
					// set data
					buff_pos = x * 3; // don't calculate 3 times
					img_buff[buff_pos] = (unsigned char)cv_comp1;
					img_buff[buff_pos + 1] = (unsigned char)cv_comp2;
					img_buff[buff_pos + 2] = (unsigned char)cv_comp3;
					continue;
				}
				if (colorTransform() == false) {
					qDebug() << "Unsupported Color Primaries: " << SMPTE::vColorPrimaries[colorPrimaries];
					return QImage(":/frame_error.png"); // abort!
				}

				//apply 709 OETF and set data
				buff_pos = x * 3; // don't calculate 3 times
				img_buff[buff_pos] = (unsigned char)oetf_709[out_ri];
				img_buff[buff_pos + 1] = (unsigned char)oetf_709[out_gi];
				img_buff[buff_pos + 2] = (unsigned char)oetf_709[out_bi];
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	else if (ColorEncoding == Metadata::eColorEncoding::RGBA) {
		// RGBA: keep original color encoding

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

				xpos = y*w + x; // don't calculate twice
				index_422 = (y*w + x) / 2;
				buff_pos = x * 3; // don't calculate 3 times
				cv_compY =  psImage->comps[0].data[xpos]  - offset_y;
				cv_compCb = psImage->comps[1].data[index_422]  - (max + 1) / 2;
				cv_compCr = psImage->comps[2].data[index_422]  - (max + 1) / 2;

				cv_compY =  (qint32)(cv_compY * max / range_y);
				cv_compCb = (qint32)(cv_compCb * max / range_c);
				cv_compCr = (qint32)(cv_compCr * max / range_c);

				cv_comp1 = cv_compY + ((2 * (1024 - Kr)*cv_compCr) / 1024);
				cv_comp2 = cv_compY - ((Kbg * cv_compCb + Krg * cv_compCr) / 1024);
				cv_comp3 = cv_compY + ((2 * (1024 - Kb)*cv_compCb) / 1024);

				if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max) cv_comp1 = max - 1;
				if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max) cv_comp2 = max - 1;
				if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max) cv_comp3 = max - 1;

				img_buff[buff_pos] = cv_comp1 >> prec_shift;
				img_buff[buff_pos + 1] = cv_comp2 >> prec_shift;
				img_buff[buff_pos + 2] = cv_comp3 >> prec_shift;
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	else { // unknown ColorEncoding
		return QImage(":/frame_error.png");
	}

	delete[] img_buff; // clear char buffer

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


