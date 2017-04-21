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
#include "AS_DCP_internal.h"
#include <QThreadpool>  
//#define DEBUG_MSGS

JP2K_Player::JP2K_Player() {

	decoded_shared = static_cast<QSharedPointer<DecodedFrames>>(new DecodedFrames);

	timer = new QTime();
	timer->start();

	// create lookup tables
	max_f = 1 << bitdepth;
	max_f_ = (float)(max_f)-1.0;

	oetf_709 = new float[max_f];

	float alpha = 1.09929682680944;
	float beta = 0.018053968510807;
	eotf_2020 = new float[max_f];

	float m1 = 0.1593017578125;
	float m2 = 78.84375;
	float c1 = 0.8359375;
	float c2 = 18.8515625;
	float c3 = 18.6875;
	eotf_PQ = new float[max_f];

	for (int i = 0; i < max_f; i++) {

		float input = (float)(i / max_f_); // convert input to value between 0...1

		// BT.709 - OETF (Inverse of BT.1886 EOTF)
		oetf_709[i] = pow(input, 1.0f / 2.4f);

		// BT.2020 - EOTF
		if (input < (4.5 * beta)) {
			eotf_2020[i] = input / 4.5;
		}
		else {
			eotf_2020[i] = pow(((input + (alpha - 1)) / alpha), 1.0 / 0.45);
		}

		// PQ
		eotf_PQ[i] = pow(((pow(input, (1.0 / m2)) - c1)) / (c2 - c3 *pow(input, (1.0 / m2))), 1.0 / m1) * 10000;
	}

	eotf_PQ[0] = 0;

	threadPool = new QThreadPool();

	// create request array
	for (int i = 0; i < decoders; i++) {
		request_queue[i] = new FrameRequest();
		pointer_queue[i] = static_cast<QSharedPointer<FrameRequest>>(request_queue[i]);
		decoder_queue[i] = new JP2K_Decoder(decoded_shared, pointer_queue[i], oetf_709, eotf_2020, eotf_PQ);
		decoder_queue[i]->setAutoDelete(false);
	}
}

JP2K_Player::~JP2K_Player()
{
	timer->~QTime();
	for (int i = 0; i < decoders; i++) {
		request_queue[i]->~FrameRequest();
		threadPool->cancel(decoder_queue[i]);
	}
	threadPool->~QThreadPool();

	delete oetf_709;
	delete eotf_2020;
	delete eotf_PQ;
}

void JP2K_Player::startPlay(){

	// reset all requests
	for (int i = 0; i < decoders; i++) {
		request_queue[i]->decoded = nullimage;
		request_queue[i]->done = false;
		request_queue[i]->error = false;
		request_queue[i]->errorMsg.clear();
		request_queue[i]->frameNr = -1;
		request_queue[i]->fps = fps;
	}

	clean();

	if (playlist.length() > 0) {
		buffering = true;
		playing = true;
		playLoop();
	}
	else {
		emit playerInfo("No/empty playlist!");
	}
}

// lets the player know where the slider is at the moment
void JP2K_Player::setPos(qint64 rframeNr, qint64 rTframeNr, int rplaylist_index) {

	decoding_index = rplaylist_index; 
	playing_index = rplaylist_index; 
	playlist_index = rplaylist_index; 

	frame_decoding_asset_float = rframeNr; 
	frame_playing_asset_float = rframeNr;
	frameNr = rframeNr;

	frame_playing_total_float = (float)rTframeNr; 
	TframeNr = rTframeNr;

	clean();
}

void JP2K_Player::clean() {

	playing = false;

	// cancel all decoding processes
	for (int i = 0; i < decoders; i++) {
		threadPool->cancel(decoder_queue[i]);
	}

	// reset vars
	decoded_shared->decoded_total = 0;
	decoded_shared->pending_requests = 0;

	player_position_counter = 0;
	requested_frames_total = 0;
	played_frames_total = 0;
	request_index = 0;

	if (!started_playing) { // reset values to last cursor position!
		decoding_index = playlist_index;
		playing_index = playlist_index;

		frame_decoding_asset_float = frameNr;
		frame_playing_asset_float = frameNr;

		frame_playing_total_float = (float)TframeNr;
		frame_decoding_total_float = (float)TframeNr;
	}
	else {
		playlist_index = playing_index;
		TframeNr = (int)frame_playing_total_float;
		frameNr = (int)(frame_playing_asset_float);
		frame_decoding_total_float = frame_playing_total_float;
		frame_decoding_asset_float = frame_playing_asset_float;
	}

	started_playing = false;
}


void JP2K_Player::playLoop(){
	while (playing) {

		timer->restart(); // measure loop cycle duration

		// calculate buffer fill time
		if (decoded_shared->pending_requests >= (float)fps*1.2) {

			int new_fps = fps - (decoded_shared->pending_requests - fps);

			if (new_fps == 0) {
				emit ShowMsgBox(QString("No images where decoded within one second.\nPlease consider selecting a smaller resolution. This may significantly increase decoding speed!"), 0);
			}
			else {
				emit ShowMsgBox(QString("There are %1 pending requests! Reduce frame rate from %2 to %3?").arg(decoded_shared->pending_requests).arg(fps).arg(new_fps), new_fps);
			}

			playing = false;
			this->thread()->quit();
			return;
		}

		// calculate buffer size
		buffer_size = decoded_shared->decoded_total - played_frames_total;
		emit playerInfo(QString("Buffer: %1").arg(buffer_size));

		// request new frame
		if (buffer_size <= fps && frame_decoding_total_float <= last_frame_total && decoding_index < playlist.size()) {

			request_index = requested_frames_total % decoders;
			
			request_queue[request_index]->frameNr = playlist.at(decoding_index).in + ((int)frame_decoding_asset_float - playlist.at(decoding_index).in) % playlist.at(decoding_index).Duration;
			//request_queue[request_index]->frameNr = (int)frame_decoding_asset_float;
			request_queue[request_index]->TframeNr = (int)(frame_decoding_total_float);
			request_queue[request_index]->error = false;
			request_queue[request_index]->layer = layer;

			// check if asset is valid
			if (playlist.at(decoding_index).asset) { // asset is valid -> open reader!

				request_queue[request_index]->asset = playlist.at(decoding_index).asset; // set asset in request
				request_queue[request_index]->done = false;
				threadPool->start(decoder_queue[request_index], QThread::HighPriority);
			}
			else { // asset is invalid? -> set error image
				request_queue[request_index]->done = true;
				request_queue[request_index]->decoded = QImage(":/frame_blank.png");

				decoded_shared->decoded_total++;
				decoded_shared->pending_requests--;
			}

			requested_frames_total++;
			decoded_shared->pending_requests++;

			// attempt "real speed" playback?
			if (realspeed) {
				frame_decoding_asset_float += skip_frames;
				frame_decoding_total_float += skip_frames;
			}
			else { // frame by frame playback
				frame_decoding_asset_float++;
				frame_decoding_total_float++;
			}
			if (frame_decoding_asset_float >= playlist[decoding_index].out) {
				if (decoding_index < (playlist.length() - 1)) {
					decoding_index++; // move on to next asset
					frame_decoding_asset_float = (frame_decoding_asset_float - playlist.at(decoding_index - 1).out) + playlist.at(decoding_index).in;
				}
			}
		}
		else {
			buffering = false; // start playing out frames (again)
		}

		// play out frames?
		if (!buffering && (decoded_shared->decoded_total > fps || frame_decoding_total_float >= last_frame_total) && buffer_size > 0) {

			// show player position every fps nr of frames
			if (player_position_counter >= fps) {
				emit currentPlayerPosition((int)frame_playing_total_float, false); // update every second
				if (show_subtitles) emit playTTML();
				player_position_counter = 0;
			}

			if (request_queue[played_frames_total % decoders]->done) { // frame exists -> show it

				emit showFrame(request_queue[played_frames_total % decoders]->decoded);

				if (frame_playing_total_float >= last_frame_total) { // last frame was played!
					
					if(player_position_counter > 0) emit currentPlayerPosition(last_frame_total, true); // set last position
					emit playbackEnded();

					setPos(playlist.at(0).in, 0, 0);

					clean();
					return;
				}

				if (frame_playing_asset_float >= playlist[playing_index].out) {
					if (playing_index < (playlist.length() - 1)) {

						playing_index++; // move on to next asset
						frame_playing_asset_float = (frame_playing_asset_float - playlist.at(playing_index - 1).out) + playlist.at(playing_index).in;
					}
				}

				// attempt "real speed" playback?
				if (realspeed) {
					frame_playing_total_float += skip_frames;
					frame_playing_asset_float += skip_frames;
				}
				else { // frame by frame playback
					frame_playing_total_float++;
					frame_playing_asset_float++;
				}
			}
			else if (request_queue[played_frames_total % decoders]->error) { // an error occured during the decoding process!
					
				if (player_position_counter > 0) emit currentPlayerPosition((int)frame_playing_total_float, false); // set player position
				emit playerInfo(QString("DECODING ERROR: %1, FRAME: %2").arg(request_queue[played_frames_total % decoders]->errorMsg).arg(request_queue[played_frames_total % decoders]->frameNr));
				emit playbackEnded();

				clean();
				return;
			}
			else { // frame not found!
				emit showFrame(nullimage);
				emit playerInfo("PLAYER ERROR: decoded frame not found!");
			}

			player_position_counter++; // count to fps, then move frame indicator
			played_frames_total++; // total number of frames played out in current play-cycle
			started_playing = true;
		}
		else if (frame_playing_total_float >= last_frame_total) { // playback has ended!

			emit currentPlayerPosition(last_frame_total, true); // update every second
			emit playbackEnded();
			emit playerInfo("playback ended!");

			setPos(playlist.at(0).in, 0, 0);

			clean();
			return;
		}

		QApplication::processEvents();

		if (timer->elapsed() < ms_wait) {
			QThread::currentThread()->msleep((ms_wait - timer->elapsed()));
		}
	}
}

// CPL selected/changed
void JP2K_Player::setPlaylist(QVector<VideoResource> &rPlaylist) {

	playlist = rPlaylist;
	if(playlist.length() == 0) emit playerInfo("No/empty playlist!");

	last_frame_total = 0; //playlist.length() - 1;

	// loop playlist items
	for (int i = 0; i < playlist.length(); i++) {
		//last_frame_total += (playlist.at(i).Duration * playlist.at(i).RepeatCount) - 1;
		last_frame_total += playlist.at(i).Duration * playlist.at(i).RepeatCount;
	}
	last_frame_total -= 1;

	// look for first valid asset
	int count = 0;
	bool found = false;
	skip_frames = 1;

	while (count < rPlaylist.length() && found == false) {
		if (rPlaylist.at(count).asset) {

			video_framerate = rPlaylist.at(count).asset->GetMetadata().editRate.GetQuotient(); // set initial fps
			skip_frames = (video_framerate / (double)fps);

			found = true;

			// get metadata (these values may not change within same CPL?)
			SMPTE::eColorPrimaries colorPrimaries = rPlaylist.at(count).asset->GetMetadata().colorPrimaries;
			int src_bitdepth = rPlaylist.at(count).asset->GetMetadata().componentDepth;
			int prec_shift = src_bitdepth - 8;
			int RGBrange = 0, RGBmaxcv = 0;
			float Kr = 0, Kg = 0, Kb = 0;

			int max = pow(2, src_bitdepth) - 1;

			int ComponentMinRef = rPlaylist.at(count).asset->GetMetadata().componentMinRef;
			int ComponentMaxRef = rPlaylist.at(count).asset->GetMetadata().componentMaxRef;

			if (ComponentMinRef && ComponentMaxRef) {
				RGBrange = ComponentMaxRef - ComponentMinRef;
				RGBmaxcv = (1 << src_bitdepth) - 1;
			}

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
			default: break;
			}

			// set params in decoders
			for (int i = 0; i < decoders; i++) {

				// color transformation
				decoder_queue[i]->ColorEncoding = rPlaylist.at(count).asset->GetMetadata().colorEncoding;
				decoder_queue[i]->colorPrimaries = colorPrimaries;
				decoder_queue[i]->transferCharactersitics = rPlaylist.at(count).asset->GetMetadata().transferCharcteristics;
				decoder_queue[i]->src_bitdepth = src_bitdepth;

				decoder_queue[i]->ComponentMinRef = ComponentMinRef;
				decoder_queue[i]->ComponentMaxRef = ComponentMaxRef;
				decoder_queue[i]->RGBmaxcv = RGBmaxcv;
				decoder_queue[i]->RGBrange = RGBrange;

				// YCbCr -> RGB conv. params.
				decoder_queue[i]->Kr = Kr;
				decoder_queue[i]->Kg = Kg;
				decoder_queue[i]->Kb = Kb;

				decoder_queue[i]->prec_shift = prec_shift;
				decoder_queue[i]->max = max;
			}
		}
		count++;
	}

	clean();
}

void JP2K_Player::setFps(int set_fps){

	//qDebug() << "set fps to" << set_fps;
	ms_wait = qRound(1000 / (double)set_fps);
	fps = set_fps;
	if(video_framerate > 0) skip_frames = (video_framerate / (double)fps);

	emit playerInfo(QString("Set fps to: %1").arg(fps));
}

void JP2K_Player::setLayer(int rLayer){
	layer = rLayer;
}

void JP2K_Player::convert_to_709(bool convert) {

	for (int i = 0; i < decoders; i++) {
		decoder_queue[i]->convert_to_709 = convert; // set in decoder [i]
	}
}
