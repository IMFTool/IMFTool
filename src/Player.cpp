/* Copyright(C) 2020 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
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
#include "Player.h"
#include "global.h"
#include <QRunnable>
#include <QTime>
#include "AS_DCP_internal.h"
//#define DEBUG_MSGS

Player::Player(enum eDecoder rDecoder) {

	mDecoder = rDecoder;
	decoded_shared = static_cast<QSharedPointer<DecodedFrames>>(new DecodedFrames);

	mTimer = new QTime();
	mTimer->start();

	threadPool = new QThreadPool();

	// create request array
	for (int i = 0; i < decoders; i++) {
		request_queue[i] = new FrameRequest();
		pointer_queue[i] = static_cast<QSharedPointer<FrameRequest>>(request_queue[i]);
		if (mDecoder == Player::Decoder_JP2K) {
			decoder_queue_JP2K[i] = new JP2K_Decoder(decoded_shared, pointer_queue[i]);
			decoder_queue_JP2K[i]->setAutoDelete(false);
		}
#ifdef APP5_ACES
		else if (mDecoder == Player::Decoder_ACES) {
			decoder_queue_ACES[i] = new ACES_Decoder(decoded_shared, pointer_queue[i]);
			decoder_queue_ACES[i]->setAutoDelete(false);
		}
#endif
	}
}

Player::~Player()
{
	mTimer->~QTime();
	for (int i = 0; i < decoders; i++) {
		request_queue[i]->~FrameRequest();
		if (mDecoder == Player::Decoder_JP2K) {
			threadPool->cancel(decoder_queue_JP2K[i]);
		}
#ifdef APP5_ACES
		else if (mDecoder == Player::Decoder_ACES) {
			threadPool->cancel(decoder_queue_ACES[i]);
		}
#endif
	}
	threadPool->~QThreadPool();
}

void Player::startPlay(){

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
void Player::setPos(qint64 rframeNr, qint64 rTframeNr, int rplaylist_index) {

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

void Player::clean() {

	playing = false;

	// cancel all decoding processes
	for (int i = 0; i < decoders; i++) {
		if (mDecoder == Player::Decoder_JP2K) {
			threadPool->cancel(decoder_queue_JP2K[i]);
		}
#ifdef APP5_ACES
		else if (mDecoder == Player::Decoder_ACES) {
			threadPool->cancel(decoder_queue_ACES[i]);
		}
#endif
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


void Player::playLoop(){
	while (playing) {

		mTimer->restart(); // measure loop cycle duration

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
			request_queue[request_index]->mShowActiveArea = mShowActiveArea;

			// check if asset is valid
			if (playlist.at(decoding_index).asset) { // asset is valid -> open reader!

				request_queue[request_index]->asset = playlist.at(decoding_index).asset; // set asset in request
				request_queue[request_index]->done = false;
				if (mDecoder == Player::Decoder_JP2K) {
					threadPool->start(decoder_queue_JP2K[request_index], QThread::HighPriority);
				}
#ifdef APP5_ACES
				else if (mDecoder == Player::Decoder_ACES) {
					threadPool->start(decoder_queue_ACES[request_index], QThread::HighPriority);
				}
#endif
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

		if (mTimer->elapsed() < ms_wait) {
			QThread::currentThread()->msleep((ms_wait - mTimer->elapsed()));
		}
	}
}

// CPL selected/changed
void Player::setPlaylist(QVector<VideoResource> &rPlaylist) {

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
			src_bitdepth = rPlaylist.at(count).asset->GetMetadata().componentDepth;
			prec_shift = src_bitdepth - 8;
			float Kr = 0, Kg = 0, Kb = 0;

			max = (1 << src_bitdepth) - 1;

			ComponentMinRef = rPlaylist.at(count).asset->GetMetadata().componentMinRef;
			ComponentMaxRef = rPlaylist.at(count).asset->GetMetadata().componentMaxRef;

			if (ComponentMinRef && ComponentMaxRef) {
				RGBrange = ComponentMaxRef - ComponentMinRef;
				RGBmaxcv = (1 << src_bitdepth) - 1;
			}

			switch (colorPrimaries) {
			case SMPTE::ColorPrimaries_ITU709:
				// set YCbCr -> RGB conversion parameters
				Kr = 0.2126f;
				Kg = 0.7152f;
				Kb = 0.0722f;
				break;
			case SMPTE::ColorPrimaries_ITU2020:
				// set YCbCr -> RGB conversion parameters
				Kr = 0.2627f;
				Kg = 0.6780f;
				Kb = 0.0593f;
				break;
			case SMPTE::ColorPrimaries_SMPTE170M:
			case SMPTE::ColorPrimaries_ITU470_PAL:
				// set YCbCr -> RGB conversion parameters
				Kr = 0.299f;
				Kg = 0.587f;
				Kb = 0.114f;
				break;
			default: break;
			}

			// set params in decoders
			for (int i = 0; i < decoders; i++) {

				// color transformation
				if (mDecoder == Player::Decoder_JP2K) {
					decoder_queue_JP2K[i]->ColorEncoding = rPlaylist.at(count).asset->GetMetadata().colorEncoding;
					decoder_queue_JP2K[i]->colorPrimaries = colorPrimaries;
					decoder_queue_JP2K[i]->transferCharacteristics = rPlaylist.at(count).asset->GetMetadata().transferCharcteristics;
					decoder_queue_JP2K[i]->src_bitdepth = src_bitdepth;

					decoder_queue_JP2K[i]->ComponentMinRef = ComponentMinRef;
					decoder_queue_JP2K[i]->ComponentMaxRef = ComponentMaxRef;
					decoder_queue_JP2K[i]->RGBmaxcv = RGBmaxcv;
					decoder_queue_JP2K[i]->RGBrange = RGBrange;
					decoder_queue_JP2K[i]->Kr = Kr;
					decoder_queue_JP2K[i]->Kg = Kg;
					decoder_queue_JP2K[i]->Kb = Kb;

					decoder_queue_JP2K[i]->prec_shift = prec_shift;
					decoder_queue_JP2K[i]->max = max;
				}
#ifdef APP5_ACES
				else if (mDecoder == Player::Decoder_ACES) {
					decoder_queue_ACES[i]->ColorEncoding = rPlaylist.at(count).asset->GetMetadata().colorEncoding;
					decoder_queue_ACES[i]->colorPrimaries = colorPrimaries;
					decoder_queue_ACES[i]->transferCharacteristics = rPlaylist.at(count).asset->GetMetadata().transferCharcteristics;
					decoder_queue_ACES[i]->src_bitdepth = src_bitdepth;

					decoder_queue_ACES[i]->ComponentMinRef = ComponentMinRef;
					decoder_queue_ACES[i]->ComponentMaxRef = ComponentMaxRef;
					decoder_queue_ACES[i]->RGBmaxcv = RGBmaxcv;
					decoder_queue_ACES[i]->RGBrange = RGBrange;

					decoder_queue_ACES[i]->prec_shift = prec_shift;
					decoder_queue_ACES[i]->max = max;
				}
#endif
			}
		}
		count++;
	}

	clean();
}

void Player::setFps(int set_fps){

	ms_wait = qRound(1000 / (double)set_fps);
	fps = set_fps;
	if(video_framerate > 0) skip_frames = (video_framerate / (double)fps);

	emit playerInfo(QString("Set fps to: %1").arg(fps));
}

void Player::setLayer(int rLayer){
	layer = rLayer;
}

void Player::convert_to_709(bool convert) {

	for (int i = 0; i < decoders; i++) {
		if (mDecoder == Player::Decoder_JP2K) {
			decoder_queue_JP2K[i]->convert_to_709 = convert; // set in decoder [i]
		}
#ifdef APP5_ACES
		else if (mDecoder == Player::Decoder_ACES) {
			// option does not exist for ACES
		}
#endif
	}
}

void Player::showActiveArea(bool rShowActiveArea) {
	mShowActiveArea = rShowActiveArea;
}
