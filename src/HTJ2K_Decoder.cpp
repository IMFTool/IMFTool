/* Copyright(C) 2020 Wolfgang Ruppel
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
#include "HTJ2K_Decoder.h"
#include "global.h"
#include <QRunnable>
#include <QTime>
#include "AS_DCP_internal.h"
#include <QThreadPool>
#include "Player.h"


HTJ2K_Decoder::HTJ2K_Decoder(QSharedPointer<DecodedFrames> &rdecoded_shared, QSharedPointer<FrameRequest> &rRequest) {

	decoded_shared = rdecoded_shared;
	request = rRequest;
	reader = new AS_02::JP2K::MXFReader();

}

void HTJ2K_Decoder::run() {


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
	} // else : correct reader is already open

	cp_reduce = request->layer; // set current layer

	if (!extractFrame(request->frameNr)) {
		qDebug() << "Error extracting HTJ2K frame no " << request->TframeNr;
		request->error = true; // an error occured processing the frame
		return;
	}


	/*
	 * set asset metadata
	 */


	colorPrimaries = request->asset->GetMetadata().colorPrimaries;
	transferCharacteristics = request->asset->GetMetadata().transferCharcteristics;
	ColorEncoding = request->asset->GetMetadata().colorEncoding;
	QString jp2k_profile = SMPTE::vJ2K_Profiles[SMPTE::J2K_ProfilesMap[request->asset->GetMetadata().pictureEssenceCoding]];

	if (jp2k_profile != "HTJ2KPictureCodingSchemeGeneric") {
		qDebug() << "Unsupported profile: " << jp2k_profile;
		request->error = true; // an error occured processing the frame
		return;
	}

	/*
	 * decodeImage()
	 */

	if (!decodeImage()) {
		qDebug() << "Error decoding HTJ2K frame no " << request->TframeNr;
		request->error = true; // an error occurred processing the frame
		return;
	}

	// success:
	request->decoded = DataToQImage(); // create image
	request->done = true; // image is ready

	decoded_shared->decoded_total++;
	decoded_shared->pending_requests--;


}

