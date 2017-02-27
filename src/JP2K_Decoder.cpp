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

//#define DEBUG_JP2K

 // #################################################### MXFP_decode #######################################################
JP2K_Decoder::JP2K_Decoder(QSharedPointer<DecodedFrames> &rdecoded_shared, QSharedPointer<FrameRequest> &rRequest, float* &Roetf_709_shared, float* &Reotf_2020_shared, float* &Reotf_PQ_shared) {

	// set stuff
	decoded_shared = rdecoded_shared;
	request = rRequest;
	oetf_709 = Roetf_709_shared;
	eotf_2020 = Reotf_2020_shared;
	eotf_PQ = Reotf_PQ_shared;

	max_f = 1 << bitdepth;
	max_f_ = (float)(max_f)-1.0;

	reader = new AS_02::JP2K::MXFReader();

	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser

	//register callbacks (for debugging)
	OPENJPEG_H::opj_set_info_handler(pDecompressor, info_callback, 0);
	OPENJPEG_H::opj_set_warning_handler(pDecompressor, warning_callback, 0);
	OPENJPEG_H::opj_set_error_handler(pDecompressor, error_callback, 0);
}

void JP2K_Decoder::run() {

	// create new frame buffer
	buff = new ASDCP::JP2K::FrameBuffer();

	//register callbacks (for debugging)
#ifdef DEBUG_JP2K
	OPENJPEG_H::opj_set_info_handler(pDecompressor, info_callback, 0);
	OPENJPEG_H::opj_set_warning_handler(pDecompressor, warning_callback, 0);
	OPENJPEG_H::opj_set_error_handler(pDecompressor, error_callback, 0);
#endif

	if (request->asset != current_asset) { // asset has changed -> update reader!
		if (current_asset) { // close previous reader
			reader->Close();
			reader->~MXFReader();
		} // else : first reader 

		// create new reader
		reader = new AS_02::JP2K::MXFReader();

		Result_t result_o = reader->OpenRead(request->asset->GetPath().absoluteFilePath().toStdString()); // open file for reading
		if (!ASDCP_SUCCESS(result_o)) {

			request->errorMsg = QString("Failed to open reader: %1").arg(result_o.Label());
			request->error = true; // an error occured processing the frame
			return;
		}
		else {
			current_asset = request->asset;
		}
	} // else : correct reader is alrady open

	// calculate neccessary buffer size
	Result_t f_next = reader->AS02IndexReader().Lookup((request->frameNr + 1), IndexF2);
	if (ASDCP_SUCCESS(f_next)) { // next frame
		Result_t f_this = reader->AS02IndexReader().Lookup(request->frameNr, IndexF1);
		if (ASDCP_SUCCESS(f_this)) { // current frame
			buff->Capacity((IndexF2.StreamOffset - IndexF1.StreamOffset) - 20); // set buffer size
		}
		else {
			buff->Capacity(default_buffer_size); // set default size
		}
	}
	else {
		buff->Capacity(default_buffer_size); // set default size
	}

	// try reading requested frame number
	Result_t res = reader->ReadFrame(request->frameNr, *buff, NULL, NULL);
	if (ASDCP_SUCCESS(res)) {
		pMemoryStream.pData = (unsigned char*)buff->Data();
		pMemoryStream.dataSize = buff->Size();
	}
	else {
		request->errorMsg = QString("%1 -> Slow HDD? (speed: ~%2 Mb/s)").arg(res.Label()).arg((request->fps * pMemoryStream.dataSize) / 1024 / 1024);
		request->error = true; // an error occured processing the frame
		return;
	}
	
	pMemoryStream.offset = 0;
	pStream = opj_stream_create_default_memory_stream(&pMemoryStream, OPJ_TRUE);
	params.cp_reduce = request->layer; // set current layer
	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser

	// Setup the decoder
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {

		request->errorMsg = "Error setting up the decoder!";
		request->error = true; // an error occured processing the frame

		OPENJPEG_H::opj_stream_destroy(pStream);
		OPENJPEG_H::opj_destroy_codec(pDecompressor);
		return;
	}

	// try reading header
	if (!OPENJPEG_H::opj_read_header(pStream, pDecompressor, &psImage)) {

		request->errorMsg = QString("Failed to read header -> Slow HDD? (speed: ~%1 Mb/s)").arg((request->fps * pMemoryStream.dataSize) / 1024 / 1024);
		request->error = true; // an error occured processing the frame

		OPENJPEG_H::opj_stream_destroy(pStream);
		OPENJPEG_H::opj_destroy_codec(pDecompressor);
		return;
	}

	// try decoding image
	if (!OPENJPEG_H::opj_decode(pDecompressor, pStream, psImage)) {

		request->errorMsg = "Failed to decode JPX image";
		request->error = true; // an error occured processing the frame

		buff->~FrameBuffer();
		OPENJPEG_H::opj_destroy_codec(pDecompressor);
		OPENJPEG_H::opj_stream_destroy(pStream);
		OPENJPEG_H::opj_image_destroy(psImage);
		return;
	}

	// success:
	request->decoded = DataToQImage(); // create image
	request->done = true; // image is ready
	
	decoded_shared->decoded_total++;
	decoded_shared->pending_requests--;

	// clean up
	buff->~FrameBuffer();
	OPENJPEG_H::opj_stream_destroy(pStream);
	OPENJPEG_H::opj_destroy_codec(pDecompressor);
	OPENJPEG_H::opj_image_destroy(psImage); // free allocated memory
}