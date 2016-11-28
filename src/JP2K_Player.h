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
#pragma once
#include <QObject>
#include <QImage>
#include <QThreadPool>
#include <QRunnable>
#include <QStringList>
#include <QFileInfo>
#include <QVariant>
#include "openjpeg.h"
#include "Error.h"
#include <QTime>
#include <QDebug>
#include <QTimer>
#include <chrono>
#include "ImfPackage.h"
#include "JP2K_Decoder.h"

class JP2K_Player;
class JP2K_Decoder;

class FrameRequest {
public:
	qint64 frameNr; // current frame in asset
	QImage decoded; // decoded image
	bool done;
};

class JP2K_Player : public QObject{
	Q_OBJECT
public:
	JP2K_Player(); // constructor

	// methods
	void stop();
	void setFps(int fps);
	void setLayer(int layer); // layer to decode
	void setPlaylist(QVector<PlayListElement> &rPlaylist);
	void setPos(int frameNr, int frame_total, int playlist_index);

	bool playing = false;
	float playing_frame = 0; // starting at startpos
	float decoding_frame = 0; // starting at startpos
	int playing_count = 0;
	float playing_frame_total = 0;
	bool realspeed = false;

private:

	QThreadPool *threadPool;
	QThread *extractFrameThread;
	AS_02::JP2K::MXFReader *reader;
	QSharedPointer<AS_02::JP2K::MXFReader> reader_shared;

	FrameRequest *request_queue[50];
	JP2K_Decoder *decoder_queue[50];
	QSharedPointer<FrameRequest> pointer_queue[50];

	QVector<PlayListElement> playlist;
	int playing_ple_index = 0; // ple currently playing
	PlayListElement ple_playing;

	bool playlist_last_item = false;
	int last_frame_nr = 0;

	
	int decoding_ple_index = 0; // decding pictures from this ple index
	PlayListElement ple_decoding;
	qreal video_framerate;
	QSharedPointer<AssetMxfTrack> CurrentDecodingAsset;
	QSharedPointer<DecodedFrames> decoded_shared;

	int decoding_frame_total = 0;
	int decoding_frame_int = 0;
	int playing_frame_int = 0;


	int playing_frame_mod = 0;

	int buffer_size = 0; // current buffer size
	int play_loop_delay = 0; // delay occurring in play loop
	float actual_fps = 0; // actual fps being displayed

	// fps & buffer mangement
	QTime *buffer_fill_timer; // time it took to fill buffer in order to be able to start playback
	int buffer_fill_counter = 0;

	qint64 asset_duration;
	int first_frame = 0;
	//PlayerPos pos;
	
	int played_frames_total = 0;
	int requested_frames_total = 0;
	int player_position_counter = 0;
	int playlist_length = 0;

	int average_decode_time = 0;
	int average_decode_time_total = 0;
	int average_decode_counter = 0;

	QImage nullimage;
	//QVector<QImage>(50) frameBuffer;
	QTime *timer;
	QTime *fps_timer; // count how many frames are really shown
	int fps_counter = 0;

	int skipped_frames = 0;

	bool buffering = true;
	int buffer_fill_count = 0; // count play-cycles it took to fill the buffer
	int fps;
	
	int ms_wait = 1000;
	void playLoop();
	void clean();

signals:
	void ShowMsgBox(const QString&, int);
	void getFrames(); // int, int, int
	void finished();
	void playerInfo(const QString&);
	void showFrame(const QImage&);
	void currentPlayerPosition(qint64);
	void playbackEnded();

	public slots:
	void startPlay(); // starts playback
	void setReader();
	//void receiveDecodedImage(); // QImage, qint64, int, int
};
