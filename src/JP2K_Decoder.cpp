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
JP2K_Decoder::JP2K_Decoder(QSharedPointer<DecodedFrames> &rdecoded_shared, QSharedPointer<FrameRequest> &rRequest, float* &Roetf_709_shared, float* &Reotf_2020_shared, float* &Reotf_PQ_shared) {
	
	// set stuff
	decoded_shared = rdecoded_shared;
	request = rRequest;
	oetf_709 = Roetf_709_shared;
	eotf_2020 = Reotf_2020_shared;
	eotf_PQ = Reotf_PQ_shared;

	max_f = 1 << bitdepth;
	max_f_ = (float)(max_f)-1.0;

	buff = new ASDCP::JP2K::FrameBuffer();

	buff->Capacity(default_buffer_size); // set default size
}

void JP2K_Decoder::run() {

	// calculate neccessary buffer size
	if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup((request->frameNr + 1), IndexF2))) { // next frame

		Result_t result_f1 = reader->AS02IndexReader().Lookup(request->frameNr, IndexF1); // current frame
		if (ASDCP_SUCCESS(result_f1)) {
			buff->Capacity((IndexF2.StreamOffset - IndexF1.StreamOffset) - 20); // set buffer size
		}
	} // else: stick with default bufer size

	// try reading requested frame number
	Result_t result = reader->ReadFrame(request->frameNr, *buff, NULL, NULL);
	if (ASDCP_SUCCESS(result)) {

		pMemoryStream.pData = (unsigned char*)buff->Data();
		pMemoryStream.dataSize = buff->Size();
	}
	else {
		request->errorMsg = QString("Frame %1 could no be read!").arg(request->frameNr);
		request->error = true; // an error occured processing the frame
		return;
	}

	pMemoryStream.offset = 0;
	pStream = opj_stream_create_default_memory_stream(&pMemoryStream, OPJ_TRUE);
	params.cp_reduce = layer; // set current layer
	pDecompressor = OPENJPEG_H::opj_create_decompress(OPJ_CODEC_J2K); // create new decompresser

	// Setup the decoder
	if (!OPENJPEG_H::opj_setup_decoder(pDecompressor, &params)) {
		request->errorMsg = "Error setting up the decoder!";
		request->error = true; // an error occured processing the frame
		opj_destroy_codec(pDecompressor);
		return;
	}

	// try reading header
	if (!OPENJPEG_H::opj_read_header(pStream, pDecompressor, &psImage)) {
		request->errorMsg = "Failed to read JPX header";
		request->error = true; // an error occured processing the frame
		opj_stream_destroy(pStream);
		opj_destroy_codec(pDecompressor);
		opj_image_destroy(psImage);
		return;
	}

	// try decoding image
	if (!OPENJPEG_H::opj_decode(pDecompressor, pStream, psImage)) {
		request->errorMsg = "Failed to decode JPX image";
		request->error = true; // an error occured processing the frame
		opj_destroy_codec(pDecompressor);
		opj_stream_destroy(pStream);
		opj_image_destroy(psImage);
		return;
	}

	// success:
	request->decoded = DataToQImage(); // create image
	request->done = true; // image is ready

	decoded_shared->decoded_total++;
	decoded_shared->decoded_cycle++;
	decoded_shared->pending_requests--;
	
	// clean up
	OPENJPEG_H::opj_stream_destroy(pStream);
	OPENJPEG_H::opj_destroy_codec(pDecompressor);
	OPENJPEG_H::opj_image_destroy(psImage);
}