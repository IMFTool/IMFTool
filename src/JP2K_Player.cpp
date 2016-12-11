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

JP2K_Player::JP2K_Player() {

	decoded_shared = static_cast<QSharedPointer<DecodedFrames>>(new DecodedFrames);

	timer = new QTime();
	timer->start();

	threadPool = new QThreadPool();
	reader = new AS_02::JP2K::MXFReader();

	// create request array
	for (int i = 0; i < 50; i++) {
		request_queue[i] = new FrameRequest();
		pointer_queue[i] = static_cast<QSharedPointer<FrameRequest>>(request_queue[i]);
		decoder_queue[i] = new JP2K_Decoder(decoded_shared, pointer_queue[i]);
		decoder_queue[i]->setAutoDelete(false);
	}
}

void JP2K_Player::startPlay(){

	qDebug() << "playback started";

	buffer_fill_counter = 0;

	if (playlist.length() > 0) {
		buffering = true;
		playing = true;
		playLoop();
	}
	else {
		emit playerInfo("No/empty playlist...");
	}
}

void JP2K_Player::stop() {

	playing = false;
	emit playerInfo("Playback stopped!");
	clean();

	// reset all values
	if (playlist.length() > 0) {
		setPos(playlist[0].in, 0, 0);
		emit currentPlayerPosition(playlist[0].in);
	}
}

void JP2K_Player::setReader() {

	// check for previous asset
	if (CurrentDecodingAsset) {
		if (operator==(CurrentDecodingAsset, ple_decoding.asset)) {
			return; // asset is unchanged -> abort!
		}
		else {
			// close old reader
			reader->Close();
			reader_shared.clear(); // calls destructor of 'reader'
		}
	}

	// set current asset 
	CurrentDecodingAsset = ple_decoding.asset;

	// create new reader
	reader = new AS_02::JP2K::MXFReader();
	reader_shared = static_cast<QSharedPointer<AS_02::JP2K::MXFReader>>(reader);

	Result_t result_o = reader->OpenRead(CurrentDecodingAsset->GetPath().absoluteFilePath().toStdString()); // open file for reading
	if (!ASDCP_SUCCESS(result_o)) {
		emit playerInfo(QString("Failed initializing reader: %1").arg(result_o.Message()));
	}

	// check for color encoding & metadata
	Metadata::eColorEncoding ColorEncoding = CurrentDecodingAsset->GetMetadata().colorEncoding;
	Metadata::eColorSpace ColorSpace = CurrentDecodingAsset->GetMetadata().colorSpace;
	int prec = CurrentDecodingAsset->GetMetadata().componentDepth;
	int prec_shift = prec - 8;
	float adjustYCbCr;
	int max;

	switch (prec) {
	case 8:
		adjustYCbCr = 128.0f;
		max = 255;
		break;
	case 10:
		adjustYCbCr = 256.0f;
		max = 511;
		break;
	case 12:
		adjustYCbCr = 512.0f;
		max = 1023;
		break;
	case 14:
		adjustYCbCr = 1024.0f;
		max = 2047;
		break;
	case 16:
		adjustYCbCr = 2048.0f;
		max = 4095;
		break;
	}

	// set reader/params in decoders
	for (int i = 0; i < 50; i++) {
		decoder_queue[i]->reader = reader_shared;
		decoder_queue[i]->ColorEncoding = ColorEncoding;
		decoder_queue[i]->ColorSpace = ColorSpace;
		decoder_queue[i]->prec = prec;
		decoder_queue[i]->prec_shift = prec_shift;
		decoder_queue[i]->adjustYCbCr = adjustYCbCr;
		decoder_queue[i]->max = max;
	}
}

// lets the player know where the slider is at the moment
void JP2K_Player::setPos(int frameNr, int TframeNr, int playlist_index) {

	playing = false;

	if (playlist_index > -1 && playlist_index < playlist.length()) {

		if (!playlist[playlist_index].asset) return; // asset is invalid!

		decoding_ple_index = playlist_index;
		ple_decoding = playlist[playlist_index];

		playing_ple_index = playlist_index;
		ple_playing = playlist[playlist_index];

		decoding_frame = frameNr;
		decoding_frame_total = TframeNr;

		playing_frame = frameNr;
		playing_frame_total = TframeNr;

		video_framerate = ple_decoding.asset->GetMetadata().editRate.GetQuotient(); // set initial fps
		played_frames_total = 0;
		decoded_shared->decoded_total = 0;
		decoded_shared->pending_requests = 0;
		fps_counter = 0;

		playlist_length = playlist.length() - 1; // set playlist length (indexes)

		qDebug() << "setting player position to frame" << frameNr << "(total)" << TframeNr;

		if ((playlist_index + 1) == playlist.length()) {
			playlist_last_item = true;
			last_frame_nr = playlist[playlist_index].out;
		}

		setReader();
		clean();
	}
}

void JP2K_Player::clean() {

	// cancel all decoding processes & clear decoded images
	for (int i = 0; i < 50; i++) {
		threadPool->cancel(decoder_queue[i]);
		request_queue[i]->decoded = nullimage;
	}

	// reset vars
	requested_frames_total = 0;
	played_frames_total = 0;
	playing_count = 0;
	buffering = true; // waits for buffer to fill (again)
	decoded_shared->decoded_cycle = 0;
	decoded_shared->decoded_total = 0;
	decoded_shared->pending_requests = 0;
	buffer_fill_count = 0; // reset
}


void JP2K_Player::playLoop(){
	while (playing) {

		timer->restart(); // measure loop cycle duration
		
		decoding_frame_int = (int)decoding_frame; //qRound(decoding_frame);

		if (playing_frame > ple_playing.out && playing_ple_index < playlist_length) {
			playing_ple_index++;
			ple_playing = playlist[playing_ple_index]; // move to next section in timeline (decode)
			playing_frame = ple_playing.in;
			video_framerate = ple_decoding.asset->GetMetadata().editRate.GetQuotient(); // set original edit rate of video

			qDebug() << "ASSET CHANGED (playing)";

			if (decoding_ple_index == playlist_length) {
				playlist_last_item = true;
				last_frame_nr = ple_playing.out;
				qDebug() << "LAST ASSET";
			}
		}else if (decoding_frame_int > ple_decoding.out && decoding_ple_index < playlist_length) {

			decoding_ple_index++;
			ple_decoding = playlist[decoding_ple_index]; // move to next section in timeline (decode)
			decoding_frame_int = ple_decoding.in; // set decode-pointer to next section
			decoding_frame = ple_decoding.in; // set decode-pointer to next section

			setReader();
			qDebug() << "ASSET CHANGED (decoding) start:" << ple_decoding.in << "end:" << ple_decoding.out << "index:" << decoding_ple_index;
		}

		buffer_size = decoded_shared->decoded_total - played_frames_total;
		emit playerInfo(QString("buffer: %1").arg(buffer_size));

		// calculate buffer fill time
		if (decoded_shared->decoded_cycle >= fps) {
			//qDebug() << "cycles to fill the buffer:" << buffer_fill_count << "total decoded" << decoded_shared->decoded_total;
			if ((float)buffer_fill_count > (float)fps*1.2) { // reduce speed

				int new_fps = ((float)fps / (float)buffer_fill_count) * (float)fps + 1;
				if (new_fps == 0) {
					emit ShowMsgBox(QString("No images where decoded within one second.\nPlease consider selecting a smaller resolution. This may significantly increase decoding speed!"), 0);
				}
				else {
					emit ShowMsgBox(QString("Only %1 frames have been decoded instead of %2. Reduce framerate to %3?").arg(decoded_shared->decoded_cycle).arg(fps).arg(new_fps), new_fps);
				}
				
				playing = false;
				this->thread()->quit();
				return;
			}
			buffer_fill_count = 0; // reset
			decoded_shared->decoded_cycle = 0; // reset
		}


		if (buffer_size <= 0 && played_frames_total > 0) {
			buffering = true; // start buffering (again)
			emit playerInfo("buffering...");
		}else if (requested_frames_total >= fps) {
			buffering = false; // start playing out frames
		}

		//qDebug() << "pending requests" << decoded_shared->pending_requests << "buffer size" << buffer_size;

		// request new frames
		if (decoded_shared->pending_requests < fps || buffering) {

			//qDebug() << "now requesting frame nr" << decoding_frame;
			
			int request_index = requested_frames_total % 50;
			request_queue[request_index]->frameNr = decoding_frame;
			request_queue[request_index]->done = false;
			threadPool->start(decoder_queue[request_index], QThread::HighestPriority);

			requested_frames_total++;
			decoded_shared->pending_requests++;

			// attempt "real speed" playback?
			if (realspeed == true) {
				decoding_frame += (video_framerate / (double)fps);
				decoding_frame_total++; // = qRound((video_framerate / (double)fps));
			}
			else { // frame by frame playback
				decoding_frame++;
				decoding_frame_total++;
			}
		}

		if (!buffering && buffer_size > 0) { // buffer is filled with > 0 frames

			// show player position every fps nr of frames
			if (player_position_counter >= fps) {
				emit currentPlayerPosition(qRound(playing_frame_total)); // update every second
				player_position_counter = 0;
			}
			player_position_counter++;

			// attempt "real speed" playback?
			if (realspeed == true) {
				playing_frame_total += (video_framerate / (double)fps);
				playing_frame += (video_framerate / fps);
			}
			else { // frame by frame playback
				playing_frame_total++;
				playing_frame++;
			}

			if (request_queue[playing_count % 50]->done) { // frame exists -> show it

				emit showFrame(request_queue[playing_count % 50]->decoded);

				buffer_fill_counter--;
				played_frames_total++;
			} // else: no frame found!

			playing_count++;
		}

		// check if last frame was reached
		if (playing_frame >= last_frame_nr && playlist_last_item == true) {

			qDebug() << "last frame" << last_frame_nr;
			emit currentPlayerPosition(playing_frame_total);
			emit playbackEnded();
			emit playerInfo("playback ended!");
			stop();
		}

		buffer_fill_count++;
		QApplication::processEvents();

		if (timer->elapsed() < ms_wait) {
			QThread::currentThread()->msleep((ms_wait - timer->elapsed()));
		}
	}
}

// CPL selected/changed
void JP2K_Player::setPlaylist(QVector<PlayListElement> &rPlaylist) {

	qDebug() << "playlist items:" << rPlaylist.length();
	playlist = rPlaylist;

	if (rPlaylist.length() > 0 && rPlaylist.at(0).asset) {
		
		video_framerate = playlist[0].asset->GetMetadata().editRate.GetQuotient();
		if(playlist.length() > 0) setPos(playlist[0].in, 0, 0);

		// loop playlist (for testing purposes)
		for (int i = 0; i < playlist.length(); i++) {
			qDebug() << playlist.at(i).in << " -> " << playlist.at(i).out;
		}
	}

	if (rPlaylist.length() == 1) { // only (and last) playlist item!
		playlist_last_item = true;
		last_frame_nr = rPlaylist[0].out;
	}
	else {
		playlist_last_item = false;
	}

	clean();
}

void JP2K_Player::setFps(int set_fps){

	//qDebug() << "set fps to" << set_fps;
	ms_wait = qRound(1000 / (double)set_fps);
	fps = set_fps;
	emit playerInfo(QString("Set fps to: %1").arg(fps));
	clean();
}

void JP2K_Player::setLayer(int layer){

	emit playerInfo(QString("Set decoding layer to: %1").arg(layer));

	// set layer in decoders
	for (int i = 0; i < 50; i++) {
		decoder_queue[i]->layer = layer;
	}

	clean();
}
