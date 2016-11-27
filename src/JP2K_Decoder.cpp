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
#include "JP2K_Player.h"
#include "JP2K_Decoder.h"
#include "global.h"
#include <QRunnable>
#include <QTime>
#include "openjpeg.h"
#include "AS_DCP_internal.h"
#include <QThreadpool>  


// #################################################### MXFP_decode #######################################################
JP2K_Decoder::JP2K_Decoder(QSharedPointer<DecodedFrames> &rdecoded_shared, QSharedPointer<FrameRequest> &rRequest) {
	// set stuff
	decoded_shared = rdecoded_shared;
	request = rRequest;

	buff = new ASDCP::JP2K::FrameBuffer();
	buff->Capacity(3000000); // default
}

void JP2K_Decoder::run() {

	// EXTRACT
	// try reading requested frame number
	if (ASDCP_SUCCESS(reader->ReadFrame(request->frameNr, *buff, NULL, NULL))) {
		pMemoryStream.pData = (unsigned char*)buff->Data();
		pMemoryStream.dataSize = buff->Size(); // raw_data.size();
	}
	else {
		qDebug() << "error reading frame" << request->frameNr;
		return;
	}

	pMemoryStream.offset = 0;
	pStream = opj_stream_create_default_memory_stream(&pMemoryStream, OPJ_TRUE);

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser
	params.cp_reduce = request->decode_layer;

	// Setup the decoder
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		qDebug() << "Error setting up decoder!";
		opj_destroy_codec(pDecompressor);
		return;
	}

	// Setup the decoder decoding parameters using user parameters
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		qDebug() << "Error setting up decoder";
		opj_destroy_codec(pDecompressor);
		return;
	}

	// try reading header
	if (!OPENJPEG_H::opj_read_header(pStream, pDecompressor, &psImage)) {
		qDebug() << "Failed to read JPX header";
		opj_stream_destroy(pStream);
		opj_destroy_codec(pDecompressor);
		opj_image_destroy(psImage);
		return;
	}

	// try decoding image
	if (!OPENJPEG_H::opj_decode(pDecompressor, pStream, psImage)) {
		qDebug() << "Failed to decode JPX image";
		opj_destroy_codec(pDecompressor);
		opj_stream_destroy(pStream);
		opj_image_destroy(psImage);
		return;
	}

	// success:
	DataToQImage();
	decoded_shared->decoded_total++;
	decoded_shared->decoded_cycle++;
	decoded_shared->pending_requests--;
	
	// clean up
	OPENJPEG_H::opj_stream_destroy(pStream);
	OPENJPEG_H::opj_destroy_codec(pDecompressor);
	pDecompressor = NULL;
	OPENJPEG_H::opj_image_destroy(psImage);
	psImage = NULL;
}


void JP2K_Decoder::DataToQImage()
{
	w = psImage->comps->w;
	h = psImage->comps->h;

	request->decoded = QImage(w, h, QImage::Format_RGB888); // create image

	bytes_per_line = w * 3;
	img_buff = new unsigned char[bytes_per_line];

	if (convert_to_709 && convert_neccessary) {
		// change source encoding to REC.709
		switch (ColorSpace) {
		case Metadata::eColorSpace::YUV_2020_PQ:
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
				memcpy(request->decoded.scanLine(y), img_buff, bytes_per_line);
			}
		}
		// YCbCr:  keep original color encoding, or color encoding == REC.709
		else if (ColorEncoding == Metadata::eColorEncoding::CDCI) {
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					xpos = (int)(y*w + x) / 2; // don't calculate twice
					buff_pos = x * 3; // don't calculate 3 times

					// get YCbCr & convert x bit -> 8 bit
					Y = psImage->comps[0].data[(int)(y*w + x)] >> prec_shift;
					Cb = psImage->comps[1].data[xpos] >> prec_shift;
					Cr = psImage->comps[2].data[xpos] >> prec_shift;

					// convert to rgb (http://softpixel.com/~cwright/programming/colorspace/yuv/)
					r = (int)(Y + 1.4075 * (Cr - 128.0F));
					b = (int)(Y + 1.7790 * (Cb - 128.0F));
					g = (int)(Y - 0.3455 * (Cb - 128.0F) - (0.7169 * (Cr - 128.0F)));

					// clamp values
					if (r < 0) { r = 0; }
					else if (r > 255) { r = 255; }
					if (g < 0) { g = 0; }
					else if (g > 255) { g = 255; }
					if (b < 0) { b = 0; }
					else if (b > 255.0f) { b = 255; }

					img_buff[buff_pos] = r;
					img_buff[buff_pos + 1] = g;
					img_buff[buff_pos + 2] = b;
				}
				memcpy(request->decoded.scanLine(y), img_buff, bytes_per_line);
			}
		}
	}

	request->done = true; // image is ready
	delete img_buff; // clear char buffer
}


//This will read from our memory to the buffer.
OPJ_SIZE_T JP2K_Decoder::opj_memory_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)

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
OPJ_SIZE_T JP2K_Decoder::opj_memory_stream_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)

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
OPJ_OFF_T JP2K_Decoder::opj_memory_stream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)

{

	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;

	OPJ_SIZE_T l_nb_bytes;


	if (p_nb_bytes < 0) return -1;//No skipping backwards.

	l_nb_bytes = (OPJ_SIZE_T)p_nb_bytes;//Allowed because it is positive.

										// Do not allow jumping past the end.

	if (l_nb_bytes >l_memory_stream->dataSize - l_memory_stream->offset)

		l_nb_bytes = l_memory_stream->dataSize - l_memory_stream->offset;//Jump the max.

																		 //Make the jump.

	l_memory_stream->offset += l_nb_bytes;

	//Returm how far we jumped.

	return l_nb_bytes;

}

//Sets the pointer to anywhere in the memory.
OPJ_BOOL JP2K_Decoder::opj_memory_stream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)

{

	opj_memory_stream* l_memory_stream = (opj_memory_stream*)p_user_data;


	if (p_nb_bytes < 0) return OPJ_FALSE;//No before the buffer.

	if (p_nb_bytes >(OPJ_OFF_T)l_memory_stream->dataSize) return OPJ_FALSE;//No after the buffer.

	l_memory_stream->offset = (OPJ_SIZE_T)p_nb_bytes;//Move to new position.

	return OPJ_TRUE;

}

//The system needs a routine to do when finished, the name tells you what I want it to do.
void JP2K_Decoder::opj_memory_stream_do_nothing(void * p_user_data)
{
	OPJ_ARG_NOT_USED(p_user_data);
}

//Create a stream to use memory as the input or output.
opj_stream_t* JP2K_Decoder::opj_stream_create_default_memory_stream(opj_memory_stream* p_memoryStream, OPJ_BOOL p_is_read_stream)

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
