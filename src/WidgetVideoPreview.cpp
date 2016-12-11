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
#include "WidgetVideoPreview.h"
#include "WidgetImagePreview.h"
//#include "AudioPlayer.h"
#include "GraphicsWidgetComposition.h" // (k)
#include <QPushButton>
#include <QGridLayout>
#include <QKeyEvent>
#include <QWidget>
#include <QLabel>
#include <QTime>
#include <QCheckBox>
#include "JP2K_Preview.h"
#include "ImfPackage.h"
#include <QThread>
#include <QLineEdit>


#define PROPERTY_PLAY_SHOWN "Play"
using namespace xercesc;


WidgetVideoPreview::WidgetVideoPreview(QWidget *pParent) : QWidget(pParent),
mData(ImfXmlHelper::Convert(QUuid::createUuid()), ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()), ImfXmlHelper::Convert(UserText(tr("Unnamed"))), ImfXmlHelper::Convert(EditRate::EditRate24), cpl2016::CompositionPlaylistType::SegmentListType()) {

	// ############## - create 2 single frame extractors - ###################
	for (int i = 0; i <= 1; i++) {
		decoders[i] = new JP2K_Preview();
		decodingThreads[i] = new QThread();
		decoders[i]->moveToThread(decodingThreads[i]);

		connect(decodingThreads[i], SIGNAL(started()), decoders[i], SLOT(decode()));
		connect(decoders[i], SIGNAL(finished()), decodingThreads[i], SLOT(quit())); // quit thread after work is completed
		connect(decoders[i], SIGNAL(decodingStatus(qint64, QString)), this, SLOT(decodingStatus(qint64,QString))); // decoder -> this
		running[i] = false; // default
	}
	// ####################################################################

	// create player
	player = new JP2K_Player();
	player->setFps(decode_speed); // set default speed

	playerThread = new QThread();
	player->moveToThread(playerThread);

	connect(playerThread, SIGNAL(started()), player, SLOT(startPlay()));
	connect(player, SIGNAL(finished()), playerThread, SLOT(quit())); // stop thread
	connect(player, SIGNAL(currentPlayerPosition(qint64)), this, SIGNAL(currentPlayerPosition(qint64)));
	connect(player, SIGNAL(playbackEnded()), this, SLOT(rPlaybackEnded())); // player -> this

	// create message box
	mpMsgBox = new QMessageBox();
	mpMsgBox->setIcon(QMessageBox::Warning);
	connect(player, SIGNAL(ShowMsgBox(const QString&, int)), this, SLOT(rShowMsgBox(const QString&, int)));

	InitLayout();

}

WidgetVideoPreview::~WidgetVideoPreview(){
	player->deleteLater();
}


void WidgetVideoPreview::InitLayout() {
	
	// layout
	QGridLayout *p_layout = new QGridLayout();
	p_layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setVerticalSpacing(0);

	// create new gl widget
	mpImagePreview = new WidgetImagePreview(); 
	p_layout->addWidget(mpImagePreview, 1, 0, 1, 5);

	connect(this, SIGNAL(regionOptionsChanged(int)), mpImagePreview, SLOT(regionOptionsChanged(int)));
	connect(player, SIGNAL(showFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // player -> glWidget
	connect(decoders[0], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
	connect(decoders[1], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget

	// create menue bar
	menuBar = new QMenuBar(this);
	menuBar->setNativeMenuBar(false);
	menuBar->setObjectName("playerMenu");
	p_layout->addWidget(menuBar, 0, 0, 1, 5); // add menue bar to layout

	// framerates
	menuSpeed = new QMenu(tr("Speed"));
	menuSpeed->setStyleSheet("QMenu::item::checked{background-color:#0080ff;}");


	int i = 0, z = 0;
	while(i != 50){
		if (i >= 30) {
			i += 5;
		}
		else {
			i++;
		}
		z++;
		speeds[z] = new QAction(QString("%1").arg(i));
		speeds[z]->setData(i);
		speeds[z]->setCheckable(true);
		if (i == decode_speed) speeds[z]->setChecked(true);
		menuSpeed->addAction(speeds[z]);
	}

	connect(menuSpeed, SIGNAL(triggered(QAction*)), this, SLOT(rChangeSpeed(QAction*)));
	menuBar->addMenu(menuSpeed);

	// quality layers
	menuQuality = new QMenu(tr("Quality"));
	menuQuality->setStyleSheet("QMenu::item::checked{background-color:#0080ff;}");
	menuBar->addMenu(menuQuality);
	connect(menuQuality, SIGNAL(triggered(QAction*)), this, SLOT(rChangeQuality(QAction*)));

	// image processing
	menuProcessing = new QMenu(tr("Processing"));
	menuBar->addMenu(menuProcessing);
	connect(menuProcessing, SIGNAL(triggered(QAction*)), this, SLOT(rChangeProcessing(QAction*)));

	processing_extract_actions[0] = new QAction(tr("Smooth"));
	processing_extract_actions[0]->setCheckable(true);
	processing_extract_actions[0]->setChecked(true); // default
	processing_extract_actions[0]->setData(0);
	menuProcessing->addAction(processing_extract_actions[0]);

	processing_extract_actions[1] = new QAction(tr("Scaling"));
	processing_extract_actions[1]->setCheckable(true);
	processing_extract_actions[1]->setChecked(true); // default
	processing_extract_actions[1]->setData(1);
	menuProcessing->addAction(processing_extract_actions[1]);

	processing_extract_actions[2] = new QAction(tr("real-speed"));
	processing_extract_actions[2]->setCheckable(true);
	processing_extract_actions[2]->setChecked(false); // default
	processing_extract_actions[2]->setData(2);
	menuProcessing->addAction(processing_extract_actions[2]);

	processing_extract_actions[3] = new QAction(tr("convert to REC.709"));
	processing_extract_actions[3]->setCheckable(true);
	processing_extract_actions[3]->setChecked(false); // default
	processing_extract_actions[3]->setData(3);
	menuProcessing->addAction(processing_extract_actions[3]);

	// save image
	processing_extract_actions[4] = new QAction(tr("save image"));
	processing_extract_actions[4]->setData(4);
	menuProcessing->addAction(processing_extract_actions[4]);

	// extract
	processing_extract = menuProcessing->addMenu("extract");
	processing_extract->setObjectName("extractMenu");

	processing_extract_names = new QStringList();
	processing_extract_names->append("top left");
	processing_extract_names->append("top center");
	processing_extract_names->append("top right");
	processing_extract_names->append("center left");
	processing_extract_names->append("center center");
	processing_extract_names->append("center right");
	processing_extract_names->append("bottom left");
	processing_extract_names->append("bottom center");
	processing_extract_names->append("bottom right");

	// add actions
	for (int i = 5; i < (processing_extract_names->length() + 5); i++) {
		processing_extract_actions[i] = new QAction(processing_extract_names->at((i - 5)));
		processing_extract_actions[i]->setCheckable(true);
		processing_extract_actions[i]->setData(i); // set index
		if (i == 0) processing_extract_actions[i]->setChecked(true); // default
		processing_extract->addAction(processing_extract_actions[i]); // add action to menu
	}

	// play button
	mpPlayPauseButton = new QPushButton();
	mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	mpPlayPauseButton->setProperty(PROPERTY_PLAY_SHOWN, true);
	mpPlayPauseButton->setFlat(true);
	mpPlayPauseButton->setContentsMargins(10, 10, 10, 10);
	p_layout->addWidget(mpPlayPauseButton, 2, 2, 1, 1);
	connect(mpPlayPauseButton, SIGNAL(clicked(bool)), this, SLOT(rPlayPauseButtonClicked(bool)));

	// stop button
	mpStopButton = new QPushButton();
	mpStopButton->setIcon(QIcon(":/stop.png"));
	mpStopButton->setFlat(true);
	mpStopButton->setContentsMargins(10, 10, 10, 10);
	connect(mpStopButton, SIGNAL(clicked(bool)), this, SLOT(stopPlayback(bool)));
	p_layout->addWidget(mpStopButton, 2, 3, 1, 1);

	// info label
	decoding_time = new QLabel();
	decoding_time->setText("");
	decoding_time->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(player, SIGNAL(playerInfo(const QString&)), decoding_time, SLOT(setText(const QString&)));

	p_layout->addWidget(decoding_time, 2, 4, 1, 1);
	
	setLayout(p_layout);
}


// stop playback
void WidgetVideoPreview::stopPlayback(bool clicked) {

	mpPlayPauseButton->setIcon(QIcon(":/play.png"));

	player->stop(); // stop player
	playerThread->quit();

	// reset preview decoder
	if (decodingThreads[0]->isRunning()) decodingThreads[0]->quit();
	if (decodingThreads[1]->isRunning()) decodingThreads[1]->quit();
}

// show message box & handle input
void WidgetVideoPreview::rShowMsgBox(const QString &msg, int new_fps) {

	mpPlayPauseButton->setIcon(QIcon(":/play.png"));

	mpMsgBox->setText(tr("A problem occured during playback!"));
	mpMsgBox->setInformativeText(msg);
	mpMsgBox->setIcon(QMessageBox::Warning);

	if (new_fps == 0) {
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
	}
	else {
		mpMsgBox->setStandardButtons(QMessageBox::Apply | QMessageBox::Discard);
		mpMsgBox->setDefaultButton(QMessageBox::Apply);
	}
	
	int ret = mpMsgBox->exec(); // open message box

	if (ret == QMessageBox::Apply && new_fps > 0) {
		speeds[decode_speed]->setChecked(false); // uncheck old value
		decode_speed = new_fps;
		speeds[new_fps]->setChecked(true); // check new value
		player->setFps(new_fps); // set new framreate in player
		playerThread->start(QThread::TimeCriticalPriority); // resume playback
		mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
	}
}

void WidgetVideoPreview::rPlaybackEnded() {
	mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	playerThread->quit();
	player->playing = false;
}

void WidgetVideoPreview::xPosChanged(const QSharedPointer<AssetMxfTrack> &rAsset, const Duration &rOffset, const Timecode &rTimecode, const int &playlist_index){

	if (player->playing) return;

	qDebug() << "xpos changed" << rTimecode.GetOverallFrames();
	xSliderFrame = rOffset.GetCount(); // slider is pointing to this frame (within asset)
	xSliderTotal = rTimecode.GetOverallFrames(); // slider is pointing to this frame (in timeline)
	xSliderTime = rTimecode.GetAsString();
	xCurrentTime = rTimecode.GetSecondsF();
	currentAsset = rAsset;

	if (showTTML) getTTML(); // TTML

	if (decodingFrame != xSliderFrame) {

		current_playlist_index = playlist_index;

		player->setPos(xSliderFrame, xSliderTotal, playlist_index); // set current frame in player

		run = (int)now_running;

		if (running[run] == false && !decodingThreads[run]->isRunning()) {
			running[run] = true;

			//emit setVerticalIndicator(const Timecode &rCplTimecode);

			qDebug() << "start generating preview nr:" << xSliderFrame;

			decodingFrame = xSliderFrame;
			decodingTime = xSliderTime;

			decoders[run]->asset = currentAsset; // set new asset in current decoder
			decoders[run]->frameNr = xSliderFrame; // set current frame number in decoder								 
			decodingThreads[run]->start(QThread::HighestPriority); // start decoder
			decoding_time->setText("loading...");
		}
	}
}

void WidgetVideoPreview::decodingStatus(qint64 frameNr,QString status) {

	decoding_time->setText(status);

	running[(int)(now_running)] = false; // free up decoder
	now_running = !now_running; // flip value
	run = (int)now_running;

	// check if current xSlider Position matches last decoded frame
	if (decodingFrame != xSliderFrame && player->playing == false && !decodingThreads[run]->isRunning()){
		
		decodingFrame = xSliderFrame;
		decodingTime = xSliderTime;

		qDebug() << "now start decoding nr" << xSliderFrame;

		decoders[run]->asset = currentAsset; // set new asset in current decoder
		decoders[run]->frameNr = xSliderFrame; // set frame number in decoder
		player->setPos(xSliderFrame, xSliderTotal, current_playlist_index); // set current frame in player

		decodingThreads[run]->start(QThread::HighestPriority); // start decoder
		decoding_time->setText("loading...");

		// TTML
		if (showTTML) getTTML();
	}

}


void WidgetVideoPreview::rPlayPauseButtonClicked(bool checked) {
	
	if (playerThread->isRunning() && player->playing) {
		// pause playback
		playerThread->quit();
		currentPlayerPosition((int)(player->playing_frame_total)); // set current position in timeline
		mpPlayPauseButton->setIcon(QIcon(":/play.png"));
		decoding_time->setText("paused!");
		player->playing = false;
	}
	else if (player->playing) { // player is paused
		playerThread->start(QThread::TimeCriticalPriority); // resume playback
		mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
	}
	else { // start player
		mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
		playerThread->start(QThread::TimeCriticalPriority);
	}
}

void WidgetVideoPreview::keyPressEvent(QKeyEvent *pEvent) {

	if(pEvent->key() == Qt::Key_Space) {
		//rPlayPauseButtonClicked(false);
	}
}


void WidgetVideoPreview::Clear() {
	decoding_time->setText("");
	mpImagePreview->Clear();
}

void WidgetVideoPreview::InstallImp() {
	menuQuality->clear(); // clear prev. resolutions from menu
}

void WidgetVideoPreview::UninstallImp() {
	menuQuality->clear(); // clear prev. resolutions from menu
	mpImagePreview->Clear();
}

void WidgetVideoPreview::setPlaylist(QVector<PlayListElement> &rPlayList, QVector<TTMLtimelineSegment> &rTTMLs) {

	ttmls = rTTMLs; // set timed text elements
	currentPlaylist = rPlayList; // set playlist
	player->setPlaylist(rPlayList); // set playlist in player
	menuQuality->clear(); // clear prev. resolutions from menu

	if (rPlayList.length() > 0) {
		
		// check if asset is valid
		if (!rPlayList[0].asset) {
			decoding_time->setText("First playlist asset is not valid...");
			return;
		}

		// loop playlist & set resolutions
		int width = 0, height = 0;
		if (rPlayList[0].asset->GetMetadata().displayWidth > 0) { // check for displayWidth
			width = rPlayList[0].asset->GetMetadata().displayWidth;
			height = rPlayList[0].asset->GetMetadata().displayHeight;
		}
		else if (rPlayList[0].asset->GetMetadata().storedWidth > 0) { // check for storedWidth
			width = rPlayList[0].asset->GetMetadata().storedWidth;
			height = rPlayList[0].asset->GetMetadata().storedHeight;
		}

		for (int i = 0; i <= 5; i++) {
			int w = width / pow(2, i);
			int h = height / pow(2, i);
			qualities[i] = new QAction(QString("%1 x %2").arg(w).arg(h)); // create new action
			qualities[i]->setData(i);
			qualities[i]->setCheckable(true);
			if (i == decode_layer) qualities[i]->setChecked(true); // default
			menuQuality->addAction(qualities[i]);
		}

		// check for color space
		if (rPlayList[0].asset->GetMetadata().lutIndex > 1) { // non REC.709 color space...
			processing_extract_actions[3]->setChecked(true);
			decoders[0]->convert_to_709 = true;
			decoders[1]->convert_to_709 = true;
		}
		else {
			processing_extract_actions[3]->setChecked(false);
			decoders[0]->convert_to_709 = false;
			decoders[1]->convert_to_709 = false;
		}

		// load first picture in preview
		decoding_time->setText("loading...");
		currentAsset = rPlayList[0].asset;
		now_running = !now_running; // use same decoder (relevant frame is still set)
		decoders[0]->asset = currentAsset; // set new asset in decoder 1
		decoders[1]->asset = currentAsset; // set new asset in decoder 2
		decoders[(int)(now_running)]->frameNr = rPlayList[0].in; // set first frame
		decodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)

		if (showTTML) getTTML(); // look for TTML
	}
	else {
		decoding_time->setText("Empty playlist found...");
	}
}

void WidgetVideoPreview::rChangeSpeed(QAction *action) {
	
	int decode_speed_index = decode_speed;
	if (decode_speed > 30) decode_speed_index = 30 + (decode_speed - 30) / 5;

	if (decode_speed != action->data().value<int>()) { // value has changed

		speeds[decode_speed_index]->setChecked(false); // uncheck 'old' value
		decode_speed = action->data().value<int>();
		decode_speed_index = decode_speed;
		player->setFps(decode_speed);
	}
	else {
		if (decode_speed > 30) decode_speed_index = (decode_speed - 30) / 5;
		speeds[decode_speed_index]->setChecked(true); // check again!
	}
}

void WidgetVideoPreview::rChangeQuality(QAction *action) {

	if (decode_layer != action->data().value<int>()) {
		qualities[decode_layer]->setChecked(false); // uncheck 'old' layer

		decode_layer = action->data().value<int>();
		player->setLayer(decode_layer);
		decoders[0]->setLayer(decode_layer);
		decoders[1]->setLayer(decode_layer);

		// reload the same frame again (if player is not playing)
		if (!player->playing) {
			now_running = !now_running; // use same decoder (relevant frame is still set)
			decodingThreads[(int)now_running]->start(QThread::HighestPriority); // start decoder (again)
			decoding_time->setText("loading...");
		}
	}
	else {
		qualities[decode_layer]->setChecked(true); // check again!
	}
}

void WidgetVideoPreview::rChangeProcessing(QAction *action) {
	
	int nr = action->data().value<int>();

	switch (nr) {
	case 0: // smoothing
		if (action->isChecked()) {
			mpImagePreview->setSmoothing(true); // smoothing is on
		}
		else {
			mpImagePreview->setSmoothing(false); // smoothing is off
		}
		break;
	case 1: // scaling
		if (action->isChecked()) {
			mpImagePreview->setScaling(true); // scaling is on
			processing_extract_actions[processing_extract_action]->setChecked(false); // uncheck 'old' option
		}
		else {
			mpImagePreview->setScaling(false); // scaling is off
			processing_extract_actions[9]->setChecked(true); // center center is ON
			processing_extract_action = 9;
			mpImagePreview->setExtract(4);
		}
		break;
	case 2: // real speed
		if (action->isChecked()) {
			player->realspeed = true; // realspeed = on
		}
		else {
			player->realspeed = false; // realspeed = off
		}
		break;
	case 3: // color space conversion
		decoders[0]->convert_to_709 = action->isChecked();
		decoders[1]->convert_to_709 = action->isChecked();

		// reload current preview
		decoding_time->setText("loading...");
		now_running = !now_running; // use same decoder (relevant frame is still set)
		decodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)

		break;
	case 4: // save image
		mpImagePreview->saveImage();
		break;
	}

	if (nr >= 5) {
		if (action->isChecked()) {
			processing_extract_actions[1]->setChecked(false); // scaling is off
			mpImagePreview->setScaling(false); // scaling is off
		}
		else {
			processing_extract_actions[1]->setChecked(true); // turn scaling back on
			mpImagePreview->setScaling(true); // scaling is on
		}
		
		// extract action!
		if(processing_extract_action != nr) processing_extract_actions[processing_extract_action]->setChecked(false); // uncheck 'old' option
		processing_extract_action = nr;
		mpImagePreview->setExtract((nr - 5));
	}
}

void WidgetVideoPreview::getTTML() {

	if (!currentAsset) return; // current asset is not valid

	ttml_search_time.start();
	QString text;
	mpImagePreview->ttml_regions.clear();

	// adjust time
	float video_framerate = currentAsset->GetMetadata().editRate.GetQuotient();
	QVector<QString> timeString;

	float time = ((float)xSliderTotal / video_framerate);
	float frac_sec;
	double seconds;

	QString h, m, s, f;

	for (int i = 0; i < ttmls.length(); i++) {

		if (time > ttmls[i].timeline_in && time <= ttmls[i].timeline_out) {
			// right segment found -> search inside segment

			double rel_time = (time - ttmls[i].timeline_in) + ttmls[i].in;
			frac_sec = modf(rel_time, &seconds);

			h = QString("%1").arg((int)(seconds / 3600.0f), 2, 10, QChar('0')); // hours
			m = QString("%1").arg((int)(seconds / 60.0f), 2, 10, QChar('0')); // minutes
			s = QString("%1").arg((int)(seconds) % 60, 2, 10, QChar('0')); // seconds
			f = QString::number(qRound(frac_sec * ttmls[i].frameRate * (float)100) / (float)100, 'f', 2);
			timeString.append(QString("%1:%2:%3:%4").arg(h).arg(m).arg(s).arg(f)); // ttml timecode

			for (int z = 0; z < ttmls[i].items.length(); z++) {
				if (rel_time > (ttmls[i].items[z].beg) && rel_time <= (ttmls[i].items[z].end)) {

					// subtitle found!
					switch (ttmls[i].items[z].type) {
					case 0:
						text.append(ttmls[i].items[z].text); // text
						break;
					case 1:
						ttmls[i].items[z].region.bgImage = ttmls[i].items[z].bgImage; // image
						break;
					}

					mpImagePreview->ttml_regions.append(ttmls[i].items[z].region);
				}
			}
		}
	}

	emit ttmlChanged(timeString, text, ttml_search_time.elapsed());
}

