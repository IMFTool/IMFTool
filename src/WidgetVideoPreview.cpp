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
#include "GraphicsWidgetComposition.h" // (k)
#include <QPushButton>
#include <QGridLayout>
#include <QKeyEvent>
#include <QWidget>
#include <QLabel>
#include <QTime>
#include <QCheckBox>
#include "JP2K_Preview.h"
#ifdef APP5_ACES
#include "ACES_Preview.h"
#endif
#ifdef CODEC_HTJ2K
#include "HTJ2K_Preview.h"
#endif
#include "ImfPackage.h"
#include <QThread>
#include <QLineEdit>

//#define DEBUG_JP2K

WidgetVideoPreview::WidgetVideoPreview(QWidget *pParent) : QWidget(pParent) {

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

#ifdef APP5_ACES
	// ############## - create 2 single frame extractors - ###################
	for (int i = 0; i <= 1; i++) {
		mpACESDecoders[i] = new ACES_Preview();
		mpACESDecoders[i]->setLayer(2);
		mpACESDecodingThreads[i] = new QThread();
		mpACESDecoders[i]->moveToThread(mpACESDecodingThreads[i]);

		connect(mpACESDecodingThreads[i], SIGNAL(started()), mpACESDecoders[i], SLOT(decode()));
		connect(mpACESDecoders[i], SIGNAL(finished()), mpACESDecodingThreads[i], SLOT(quit())); // quit thread after work is completed
		connect(mpACESDecoders[i], SIGNAL(decodingStatus(qint64, QString)), this, SLOT(decodingStatus(qint64,QString))); // decoder -> this
		mACESRunning[i] = false; // default
	}
	// ####################################################################
#endif
#ifdef CODEC_HTJ2K
	// ############## - create 2 single frame extractors - ###################
	for (int i = 0; i <= 1; i++) {
		mpHTJ2KDecoders[i] = new HTJ2K_Preview();
		mpHTJ2KDecoders[i]->setLayer(2);
		mpHTJ2KDecodingThreads[i] = new QThread();
		mpHTJ2KDecoders[i]->moveToThread(mpHTJ2KDecodingThreads[i]);

		connect(mpHTJ2KDecodingThreads[i], SIGNAL(started()), mpHTJ2KDecoders[i], SLOT(decode()));
		connect(mpHTJ2KDecoders[i], SIGNAL(finished()), mpHTJ2KDecodingThreads[i], SLOT(quit())); // quit thread after work is completed
		connect(mpHTJ2KDecoders[i], SIGNAL(decodingStatus(qint64, QString)), this, SLOT(decodingStatus(qint64,QString))); // decoder -> this
		mHTJ2KRunning[i] = false; // default
	}
	// ####################################################################
#endif

	// create player
	player = new Player(Player::Decoder_JP2K);
	player->setFps(decode_speed); // set default speed

	playerThread = new QThread();
	player->moveToThread(playerThread);
	player->setLayer(decode_layer); // set default layer

	connect(playerThread, SIGNAL(started()), player, SLOT(startPlay()));
	connect(player, SIGNAL(currentPlayerPosition(qint64, bool)), this, SLOT(forwardPlayerPosition(qint64, bool)));
	connect(player, SIGNAL(playTTML()), this, SLOT(getTTML()));
	connect(player, SIGNAL(playbackEnded()), this, SLOT(rPlaybackEnded())); // player -> this

	// create message box
	mpMsgBox = new QMessageBox();
	mpMsgBox->setIcon(QMessageBox::Warning);
	connect(player, SIGNAL(ShowMsgBox(const QString&, int)), this, SLOT(rShowMsgBox(const QString&, int)));

#ifdef APP5_ACES
	// create player
	mpACESPlayer = new Player(Player::Decoder_ACES);
	mpACESPlayer->setFps(decode_speed); // set default speed

	mpACESPlayerThread = new QThread();
	mpACESPlayer->moveToThread(mpACESPlayerThread);
	mpACESPlayer->setLayer(decode_layer); // set default layer

	connect(mpACESPlayerThread, SIGNAL(started()), mpACESPlayer, SLOT(startPlay()));
	connect(mpACESPlayer, SIGNAL(currentPlayerPosition(qint64, bool)), this, SLOT(forwardPlayerPosition(qint64, bool)));
	connect(mpACESPlayer, SIGNAL(playTTML()), this, SLOT(getTTML()));
	connect(mpACESPlayer, SIGNAL(playbackEnded()), this, SLOT(rPlaybackEnded())); // player -> this

	connect(mpACESPlayer, SIGNAL(ShowMsgBox(const QString&, int)), this, SLOT(rShowMsgBox(const QString&, int)));

#endif

#ifdef CODEC_HTJ2K
	// create player
	mpHTJ2KPlayer = new Player(Player::Decoder_HTJ2K);
	mpHTJ2KPlayer->setFps(decode_speed); // set default speed

	mpHTJ2KPlayerThread = new QThread();
	mpHTJ2KPlayer->moveToThread(mpHTJ2KPlayerThread);
	mpHTJ2KPlayer->setLayer(decode_layer); // set default layer

	connect(mpHTJ2KPlayerThread, SIGNAL(started()), mpHTJ2KPlayer, SLOT(startPlay()));
	connect(mpHTJ2KPlayer, SIGNAL(currentPlayerPosition(qint64, bool)), this, SLOT(forwardPlayerPosition(qint64, bool)));
	connect(mpHTJ2KPlayer, SIGNAL(playTTML()), this, SLOT(getTTML()));
	connect(mpHTJ2KPlayer, SIGNAL(playbackEnded()), this, SLOT(rPlaybackEnded())); // player -> this

	connect(mpHTJ2KPlayer, SIGNAL(ShowMsgBox(const QString&, int)), this, SLOT(rShowMsgBox(const QString&, int)));

#endif

	InitLayout();
}

WidgetVideoPreview::~WidgetVideoPreview(){
	player->~Player();
#ifdef APP5_ACES
	mpACESPlayer->~Player();
#endif
#ifdef CODEC_HTJ2K
	mpHTJ2KPlayer->~Player();
#endif
}


void WidgetVideoPreview::InitLayout() {
	
	// layout
	QGridLayout *p_layout = new QGridLayout();
	p_layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->setVerticalSpacing(0);

	// create new gl widget
	mpImagePreview = new WidgetImagePreview(this);
	p_layout->addWidget(mpImagePreview, 1, 0, 1, 5);

	connect(this, SIGNAL(regionOptionsChanged(int)), mpImagePreview, SLOT(regionOptionsChanged(int)));
	connect(player, SIGNAL(showFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // player -> glWidget
	connect(decoders[0], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
	connect(decoders[1], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
	connect(mpImagePreview, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(keyPressEvent(QKeyEvent*)));
#ifdef APP5_ACES
	connect(mpACESPlayer, SIGNAL(showFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // player -> glWidget
	connect(mpACESDecoders[0], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
	connect(mpACESDecoders[1], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
#endif
#ifdef CODEC_HTJ2K
	connect(mpHTJ2KPlayer, SIGNAL(showFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // player -> glWidget
	connect(mpHTJ2KDecoders[0], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
	connect(mpHTJ2KDecoders[1], SIGNAL(ShowFrame(const QImage&)), mpImagePreview, SLOT(ShowImage(const QImage&))); // decoder -> glWidget
#endif

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
	quint8 action_count = 0;

	processing_extract_actions[action_count] = new QAction(tr("Smooth"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(true); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

	processing_extract_actions[action_count] = new QAction(tr("Scaling"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(true); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

	menuProcessing->addSeparator();
	processing_extract_actions[action_count] = new QAction(tr("Show Native Resolution"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(true); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

	processing_extract_actions[action_count] = new QAction(tr("Show Active Area"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(false); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	menuProcessing->addSeparator();
	action_count++;

	processing_extract_actions[action_count] = new QAction(tr("Show subtitles"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(true); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

	processing_extract_actions[action_count] = new QAction(tr("real-speed"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(false); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

	processing_extract_actions[action_count] = new QAction(tr("convert to REC.709"));
	processing_extract_actions[action_count]->setCheckable(true);
	processing_extract_actions[action_count]->setChecked(true); // default
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

	// save image
	processing_extract_actions[action_count] = new QAction(tr("save image"));
	processing_extract_actions[action_count]->setData(action_count);
	menuProcessing->addAction(processing_extract_actions[action_count]);
	action_count++;

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
	for (int i = 0; i < processing_extract_names->length(); i++) {
		processing_extract_actions[action_count] = new QAction(processing_extract_names->at(i));
		processing_extract_actions[action_count]->setCheckable(true);
		processing_extract_actions[action_count]->setData(action_count); // set index
		processing_extract->addAction(processing_extract_actions[action_count]); // add action to menu
		action_count++;
	}

	// play button
	mpPlayPauseButton = new QPushButton();
	mpPlayPauseButton->setIcon(QIcon(":/play.png"));
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
#ifdef APP5_ACES
	connect(mpACESPlayer, SIGNAL(playerInfo(const QString&)), decoding_time, SLOT(setText(const QString&)));
#endif
#ifdef CODEC_HTJ2K
	connect(mpHTJ2KPlayer, SIGNAL(playerInfo(const QString&)), decoding_time, SLOT(setText(const QString&)));
#endif
	// full screen
	menuView = new QMenu(tr("View"));
	menuBar->addMenu(menuView);
	menuView->addAction("Toggle fullscreen");
	connect(menuView, SIGNAL(triggered(QAction*)), this, SLOT(rViewFullScreen(void)));


	p_layout->addWidget(decoding_time, 2, 4, 1, 1);
	
	setLayout(p_layout);
	setFocusPolicy(Qt::ClickFocus);
}

void WidgetVideoPreview::forwardPlayerPosition(qint64 frameNr, bool decode_at_new_pos) {
#ifdef DEBUG_JP2K
	qDebug() << "set frame indicator to:" << frameNr;
#endif
	if(!decode_at_new_pos) setFrameIndicator = true; // ignore next xPosChanged signal!
	emit currentPlayerPosition(frameNr);
}

// stop playback
void WidgetVideoPreview::stopPlayback(bool clicked) {

	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM))
		player->clean(); // reset player
#ifdef APP5_ACES
	else if (mImfApplication == ::App5)
		mpACESPlayer->clean();
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K))
		mpHTJ2KPlayer->clean();
#endif
	if (currentPlaylist.length() > 0) player->setPos(currentPlaylist[0].in, 0, 0);

	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
		if (playerThread->isRunning()) playerThread->quit();
	}
#ifdef APP5_ACES
	else if (mImfApplication == ::App5) {
		if (mpACESPlayerThread->isRunning()) mpACESPlayerThread->quit();
	}
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
		if (mpHTJ2KPlayerThread->isRunning()) mpHTJ2KPlayerThread->quit();
	}
#endif

	mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	
	// reset preview decoder
	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
		if (decodingThreads[0]->isRunning()) decodingThreads[0]->quit();
		if (decodingThreads[1]->isRunning()) decodingThreads[1]->quit();
	}
#ifdef APP5_ACES
	else if (mImfApplication == ::App5) {
		if (mpACESDecodingThreads[0]->isRunning()) mpACESDecodingThreads[0]->quit();
		if (mpACESDecodingThreads[1]->isRunning()) mpACESDecodingThreads[1]->quit();
	}
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
		if (mpHTJ2KDecodingThreads[0]->isRunning()) mpHTJ2KDecodingThreads[0]->quit();
		if (mpHTJ2KDecodingThreads[1]->isRunning()) mpHTJ2KDecodingThreads[1]->quit();
	}
#endif

	if(currentPlaylist.length() > 0) decodingFrame = -1; // force preview refresh
	setFrameIndicator = false;
	//if clicked==false --> Button Stop was pressed
	if (!clicked) emit currentPlayerPosition(0); // reset indicator to first frame
}

void WidgetVideoPreview::rViewFullScreen() {
	mpImagePreview->toggleFullScreen();
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
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
			player->setFps(new_fps); // set new framreate in player
			playerThread->start(QThread::TimeCriticalPriority); // resume playback
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5){
			mpACESPlayer->setFps(new_fps); // set new framreate in player
			mpACESPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)){
			mpHTJ2KPlayer->setFps(new_fps); // set new framreate in player
			mpHTJ2KPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
		}
#endif
		mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
	}
}

void WidgetVideoPreview::rPlaybackEnded() {
	mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
		if(playerThread->isRunning()) playerThread->quit();
	}
#ifdef APP5_ACES
	else if (mImfApplication == ::App5) {
		if(mpACESPlayerThread->isRunning()) mpACESPlayerThread->quit();
	}
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
		if(mpHTJ2KPlayerThread->isRunning()) mpHTJ2KPlayerThread->quit();
	}
#endif
}

void WidgetVideoPreview::xPosChanged(const QSharedPointer<AssetMxfTrack> &rAsset, const qint64 &rOffset, const Timecode &rTimecode, const int &playlist_index){

	xSliderFrame = rOffset; // slider is pointing to this frame (within asset)
	xSliderTotal = rTimecode.GetOverallFrames(); // slider is pointing to this frame (in timeline)
	currentAsset = rAsset;
	current_playlist_index = playlist_index;
#ifdef DEBUG_JP2K
	qDebug() << "xPosChanged" << rOffset << decodingFrame << xSliderTotal << currentAsset.isNull();
#endif
	if (setFrameIndicator) {
		setFrameIndicator = false;
		return; // ignore this signal (it is a response from setting the player position)
	}
	else if (player->playing) { // pause playback!
		player->playing = false;
		playerThread->quit();
		mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	}
#ifdef APP5_ACES
	else if (mpACESPlayer->playing) { // pause playback!
		mpACESPlayer->playing = false;
		mpACESPlayerThread->quit();
		mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	}
#endif
#ifdef CODEC_HTJ2K
	else if (mpHTJ2KPlayer->playing) { // pause playback!
		mpHTJ2KPlayer->playing = false;
		mpHTJ2KPlayerThread->quit();
		mpPlayPauseButton->setIcon(QIcon(":/play.png"));
	}
#endif

#ifdef DEBUG_JP2K
	qDebug() << "xpos asset:" << xSliderFrame << "total:" << xSliderTotal << "current_playlist_index:" << current_playlist_index;
#endif
	
	if (showTTML) getTTML(); // TTML

	if (decodingFrame != xSliderTotal) {

		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
			player->setPos(xSliderFrame, xSliderTotal, playlist_index); // set current frame in player
			// Terminate running decodes
			if (decodingThreads[run]->isRunning()) decodingThreads[run]->quit();
			run = (int)now_running;
#ifdef DEBUG_JP2K
			qDebug() << run << running[0] << running[1] << decodingThreads[0]->isRunning() << decodingThreads[1]->isRunning();
#endif

			if (running[run] == false && !decodingThreads[run]->isRunning()) {
				running[run] = true;

				//emit setVerticalIndicator(const Timecode &rCplTimecode);
#ifdef DEBUG_JP2K
				qDebug() << "start generating preview nr:" << xSliderFrame << currentAsset.isNull();
#endif
				decodingFrame = xSliderTotal;

				decoders[run]->asset = currentAsset; // set new asset in current decoder
				//decoders[run]->frameNr = xSliderFrame; // set current frame number in decoder
				if (currentPlaylist.length() == 0) {//Under some race conditions during Outgest currentPlaylist can be empty here.
#ifdef DEBUG_JP2K
					qDebug() << "WidgetVideoPreview::xPosChanged called with empty currentPlaylist";
#endif
					running[run] = false;
				} else {
					VideoResource vr = currentPlaylist[current_playlist_index];
					decoders[run]->mFrameNr = vr.in + (xSliderFrame - vr.in) % vr.Duration; // set current frame number in decoder
					decodingThreads[run]->start(QThread::HighestPriority); // start decoder
					decoding_time->setText("loading...");
				}
			}
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			mpACESPlayer->setPos(xSliderFrame, xSliderTotal, playlist_index); // set current frame in player
			// Terminate running decodes
			if (mpACESDecodingThreads[0]->isRunning()) mpACESDecodingThreads[0]->quit();
			if (mpACESDecodingThreads[1]->isRunning()) mpACESDecodingThreads[1]->quit();
			run = (int)now_running;
#ifdef DEBUG_JP2K
			qDebug() << run << running[0] << running[1] << decodingThreads[0]->isRunning() << decodingThreads[1]->isRunning();
#endif

			if (running[run] == false && !mpACESDecodingThreads[run]->isRunning()) {
				running[run] = true;

				//emit setVerticalIndicator(const Timecode &rCplTimecode);
#ifdef DEBUG_JP2K
				qDebug() << "start generating preview nr:" << xSliderFrame << currentAsset.isNull();
#endif
				decodingFrame = xSliderTotal;

				mpACESDecoders[run]->asset = currentAsset; // set new asset in current decoder
				//decoders[run]->frameNr = xSliderFrame; // set current frame number in decoder
				if (currentPlaylist.length() == 0) {//Under some race conditions during Outgest currentPlaylist can be empty here.
#ifdef DEBUG_JP2K
					qDebug() << "WidgetVideoPreview::xPosChanged called with empty currentPlaylist";
#endif
					running[run] = false;
				} else {
					VideoResource vr = currentPlaylist[current_playlist_index];
					mpACESDecoders[run]->mFrameNr = vr.in + (xSliderFrame - vr.in) % vr.Duration; // set current frame number in decoder
					mpACESDecodingThreads[run]->start(QThread::HighestPriority); // start decoder
					decoding_time->setText("loading...");
				}
			}

		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			mpHTJ2KPlayer->setPos(xSliderFrame, xSliderTotal, playlist_index); // set current frame in player
			// Terminate running decodes
			if (mpHTJ2KDecodingThreads[0]->isRunning()) mpHTJ2KDecodingThreads[0]->quit();
			if (mpHTJ2KDecodingThreads[1]->isRunning()) mpHTJ2KDecodingThreads[1]->quit();
			run = (int)now_running;
#ifdef DEBUG_JP2K
			qDebug() << run << running[0] << running[1] << decodingThreads[0]->isRunning() << decodingThreads[1]->isRunning();
#endif

			if (running[run] == false && !mpHTJ2KDecodingThreads[run]->isRunning()) {
				running[run] = true;

				//emit setVerticalIndicator(const Timecode &rCplTimecode);
#ifdef DEBUG_JP2K
				qDebug() << "start generating preview nr:" << xSliderFrame << currentAsset.isNull();
#endif
				decodingFrame = xSliderTotal;

				mpHTJ2KDecoders[run]->asset = currentAsset; // set new asset in current decoder
				//decoders[run]->frameNr = xSliderFrame; // set current frame number in decoder
				if (currentPlaylist.length() == 0) {//Under some race conditions during Outgest currentPlaylist can be empty here.
#ifdef DEBUG_JP2K
					qDebug() << "WidgetVideoPreview::xPosChanged called with empty currentPlaylist";
#endif
					running[run] = false;
				} else {
					VideoResource vr = currentPlaylist[current_playlist_index];
					mpHTJ2KDecoders[run]->mFrameNr = vr.in + (xSliderFrame - vr.in) % vr.Duration; // set current frame number in decoder
					mpHTJ2KDecodingThreads[run]->start(QThread::HighestPriority); // start decoder
					decoding_time->setText("loading...");
				}
			}

		}
#endif
	}
}

void WidgetVideoPreview::decodingStatus(qint64 frameNr,QString status) {

	decoding_time->setText(status);

	running[(int)(now_running)] = false; // free up decoder
	now_running = !now_running; // flip value
	run = (int)now_running;
#ifdef DEBUG_JP2K
	qDebug() << decodingFrame << xSliderTotal << player->playing << decodingThreads[run]->isRunning();
#endif
	// check if current xSlider Position matches last decoded frame
	if (decodingFrame != xSliderTotal && player->playing == false && !decodingThreads[run]->isRunning()){
		
		decodingFrame = xSliderTotal;
#ifdef DEBUG_JP2K
		qDebug() << "now start decoding nr" << xSliderFrame;
#endif
		//TODO
		VideoResource vr = currentPlaylist[current_playlist_index];
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
			decoders[run]->asset = currentAsset; // set new asset in current decoder
			//decoders[run]->frameNr = xSliderFrame; // set frame number in decoder
			decoders[run]->mFrameNr = vr.in + (xSliderFrame - vr.in) % vr.Duration; // set current frame number in decoder
			player->setPos(xSliderFrame, xSliderTotal, current_playlist_index); // set current frame in player

			decodingThreads[run]->start(QThread::HighestPriority); // start decoder
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			mpACESDecoders[run]->asset = currentAsset; // set new asset in current decoder
			//decoders[run]->frameNr = xSliderFrame; // set frame number in decoder
			mpACESDecoders[run]->mFrameNr = vr.in + (xSliderFrame - vr.in) % vr.Duration; // set current frame number in decoder
			mpACESPlayer->setPos(xSliderFrame, xSliderTotal, current_playlist_index); // set current frame in player

			mpACESDecodingThreads[run]->start(QThread::HighestPriority); // start decoder

		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			mpHTJ2KDecoders[run]->asset = currentAsset; // set new asset in current decoder
			//decoders[run]->frameNr = xSliderFrame; // set frame number in decoder
			mpHTJ2KDecoders[run]->mFrameNr = vr.in + (xSliderFrame - vr.in) % vr.Duration; // set current frame number in decoder
			mpHTJ2KPlayer->setPos(xSliderFrame, xSliderTotal, current_playlist_index); // set current frame in player

			mpHTJ2KDecodingThreads[run]->start(QThread::HighestPriority); // start decoder

		}
#endif
		decoding_time->setText("loading...");

		// TTML
		if (showTTML) getTTML();
	}

}

void WidgetVideoPreview::rPlayPauseButtonClicked(bool checked) {
	
	mpImagePreview->ttml_regions.clear();

	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
		if (playerThread->isRunning() && player->playing) { // pause playback

			player->playing = false;
			playerThread->quit();
			mpPlayPauseButton->setIcon(QIcon(":/play.png"));

			// set current position in timeline
			if(player->realspeed)
			{
				currentPlayerPosition((int)(player->frame_playing_total_float - player->skip_frames));
			}
			else {
				currentPlayerPosition((int)(player->frame_playing_total_float - 1));
			}
		}
		else if (player->playing) { // player is paused
			playerThread->start(QThread::TimeCriticalPriority); // resume playback
			mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
		}
		else if(currentPlaylist.length() > 0){ // start player
			mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
			playerThread->start(QThread::TimeCriticalPriority);
			emit ttmlChanged(QVector<visibleTTtrack>(), ttml_search_time.elapsed()); // clear subtitle preview
		}else
		{
			decoding_time->setText("Player could not be started - please open a CPL with a video track first!");
		}
	}
#ifdef APP5_ACES
	else if (mImfApplication == ::App5) {
		if (mpACESPlayerThread->isRunning() && mpACESPlayer->playing) { // pause playback

			mpACESPlayer->playing = false;
			mpACESPlayerThread->quit();
			mpPlayPauseButton->setIcon(QIcon(":/play.png"));

			// set current position in timeline
			if(mpACESPlayer->realspeed)
			{
				currentPlayerPosition((int)(mpACESPlayer->frame_playing_total_float - player->skip_frames));
			}
			else {
				currentPlayerPosition((int)(mpACESPlayer->frame_playing_total_float - 1));
			}
		}
		else if (mpACESPlayer->playing) { // player is paused
			mpACESPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
			mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
		}
		else if(currentPlaylist.length() > 0){ // start player
			mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
			mpACESPlayerThread->start(QThread::TimeCriticalPriority);
			emit ttmlChanged(QVector<visibleTTtrack>(), ttml_search_time.elapsed()); // clear subtitle preview
		}else
		{
			decoding_time->setText("Player could not be started - please open a CPL with a video track first!");
		}

	}
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
		if (mpHTJ2KPlayerThread->isRunning() && mpHTJ2KPlayer->playing) { // pause playback

			mpHTJ2KPlayer->playing = false;
			mpHTJ2KPlayerThread->quit();
			mpPlayPauseButton->setIcon(QIcon(":/play.png"));

			// set current position in timeline
			if(mpHTJ2KPlayer->realspeed)
			{
				currentPlayerPosition((int)(mpHTJ2KPlayer->frame_playing_total_float - player->skip_frames));
			}
			else {
				currentPlayerPosition((int)(mpHTJ2KPlayer->frame_playing_total_float - 1));
			}
		}
		else if (mpHTJ2KPlayer->playing) { // player is paused
			mpHTJ2KPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
			mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
		}
		else if(currentPlaylist.length() > 0){ // start player
			mpPlayPauseButton->setIcon(QIcon(":/pause.png"));
			mpHTJ2KPlayerThread->start(QThread::TimeCriticalPriority);
			emit ttmlChanged(QVector<visibleTTtrack>(), ttml_search_time.elapsed()); // clear subtitle preview
		}else
		{
			decoding_time->setText("Player could not be started - please open a CPL with a video track first!");
		}

	}
#endif
}

void WidgetVideoPreview::keyPressEvent(QKeyEvent *pEvent) {

	if(pEvent->key() == Qt::Key_Space) {
		rPlayPauseButtonClicked(false);
	}
	else if(pEvent->key() == Qt::Key_K) {
		if (player->playing) rPlayPauseButtonClicked(false);
	}
	else if(pEvent->key() == Qt::Key_L) {
		if (!player->playing) rPlayPauseButtonClicked(false);
	}
}

void WidgetVideoPreview::Clear() {
	decoding_time->setText("");
	mpImagePreview->Clear();
	emit stopPlayback(true); // stop playback
}

void WidgetVideoPreview::Reset() {
	Clear();
	decode_layer = 2;
	processing_extract_actions[2]->setChecked(true); // Data Window is on
	processing_extract_actions[3]->setChecked(false); // Display Window is off
	for (int i = 0; i <= 1; i++) {
		decoders[i]->setLayer(decode_layer);
#ifdef APP5_ACES
		mpACESDecoders[i]->setLayer(decode_layer);
		mpACESDecoders[i]->showActiveArea(false);
#endif
#ifdef CODEC_HTJ2K
		mpHTJ2KDecoders[i]->setLayer(decode_layer);
#endif
	}
}

void WidgetVideoPreview::InstallImp() {
	menuQuality->clear(); // clear prev. resolutions from menu
}

void WidgetVideoPreview::UninstallImp() {
	menuQuality->clear(); // clear prev. resolutions from menu
	mpImagePreview->Clear();
}

void WidgetVideoPreview::setPlaylist(QVector<VideoResource> &rPlayList, QVector<TTMLtimelineResource> &rTTMLs) {

	ttmls = &rTTMLs; // set timed text elements
	currentPlaylist = rPlayList; // set playlist
	if (! currentPlaylist.isEmpty() && !currentPlaylist.first().asset.isNull()) {
		SMPTE::eColorPrimaries color_primaries = currentPlaylist.first().asset->GetMetadata().colorPrimaries;
		SMPTE::eTransferCharacteristic transfer_characteristics = currentPlaylist.first().asset->GetMetadata().transferCharcteristics;
		Metadata::eEssenceType essence_type = currentPlaylist.first().asset->GetEssenceType();
		if (essence_type == Metadata::Jpeg2000) {
			if (color_primaries == SMPTE::ColorPrimaries_CinemaMezzanine) {
				if ( (transfer_characteristics == SMPTE::TransferCharacteristic_CinemaMezzanineDCDM) ||
						(transfer_characteristics == SMPTE::TransferCharacteristic_CinemaMezzanineDCDM_Wrong)) mImfApplication = ::App4DCDM;
				else mImfApplication = ::App4;
			}
			else mImfApplication = ::App2e;
		}
#ifdef CODEC_HTJ2K
		if (essence_type == Metadata::HTJ2K) {
			if (color_primaries == SMPTE::ColorPrimaries_CinemaMezzanine) {
				if ( (transfer_characteristics == SMPTE::TransferCharacteristic_CinemaMezzanineDCDM) ||
						(transfer_characteristics == SMPTE::TransferCharacteristic_CinemaMezzanineDCDM_Wrong)) mImfApplication = ::App4DCDM_HTJ2K;
				else return; // HTJ2K  in App4_linear is not supported.
			}
			else mImfApplication = ::App2e_HTJ2K;
		}
#endif
#ifdef APP5_ACES
		if (essence_type == Metadata::Aces) {
			mImfApplication = ::App5;
		}
#endif
	}

	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM))
		player->setPlaylist(rPlayList); // set playlist in player
#ifdef APP5_ACES
	else if (mImfApplication == ::App5)
		mpACESPlayer->setPlaylist(rPlayList); // set playlist in player
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K))
		mpHTJ2KPlayer->setPlaylist(rPlayList); // set playlist in player
#endif
	current_playlist_index = 0; // set to first item in playlist 

	// create array of TTMLtracks
	int track_index = 0;
	ttml_tracks = QMap<int, QVector<TTMLtimelineResource>>(); // clear all previus tracks
	QMap<int, int> track_indices;
	for (int i = 0; i < ttmls->length(); i++) {
		ttml_tracks[ ttmls->at(i).track_index ].append(ttmls->at(i));
	}

	if (rPlayList.length() > 0) {
		
		// loop playlist & set resolutions
		int count = -1;
		bool found = false;

		// look for first valid asset
		while (count < (rPlayList.length() - 1) && found == false) {
			count++;
			if (rPlayList.at(count).asset) {
				found = true;
			}
		}

		if (found == false) return; // no valid asset found in timeline

		// use first valid asset to get resolution

		if (rPlayList.at(count).asset->GetMetadata().storedWidth > 0) { // check for storedWidth
			mStoredWidth = rPlayList.at(count).asset->GetMetadata().storedWidth;
			mStoredHeight = rPlayList.at(count).asset->GetMetadata().storedHeight;
		}
		if (rPlayList.at(count).asset->GetMetadata().displayWidth > 0) { // check for displayWidth
			mDisplayWidth = rPlayList.at(count).asset->GetMetadata().displayWidth;
			mDisplayHeight = rPlayList.at(count).asset->GetMetadata().displayHeight;
		}

		setMenuQuality(mStoredWidth, mStoredHeight);

		// load first picture in preview
		decoding_time->setText("loading...");
		currentAsset = rPlayList[0].asset;
		now_running = !now_running; // use same decoder (relevant frame is still set)
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
			decoders[0]->asset = currentAsset; // set new asset in decoder 1
			decoders[1]->asset = currentAsset; // set new asset in decoder 2
			decoders[(int)(now_running)]->mFrameNr = rPlayList[0].in; // set first frame
			decodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			mpACESDecoders[0]->asset = currentAsset; // set new asset in decoder 1
			mpACESDecoders[1]->asset = currentAsset; // set new asset in decoder 2
			mpACESDecoders[(int)(now_running)]->mFrameNr = rPlayList[0].in; // set first frame
			mpACESDecodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)
		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			mpHTJ2KDecoders[0]->asset = currentAsset; // set new asset in decoder 1
			mpHTJ2KDecoders[1]->asset = currentAsset; // set new asset in decoder 2
			mpHTJ2KDecoders[(int)(now_running)]->mFrameNr = rPlayList[0].in; // set first frame
			mpHTJ2KDecodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)
		}
#endif

		if (showTTML) getTTML(); // look for TTML
#ifdef DEBUG_JP2K
		qDebug() << "setPos(rPlayList.at(0).in, xSliderTotal, current_playlist_index)" << rPlayList.at(0).in <<  xSliderTotal;
#endif
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM))
			player->setPos(rPlayList.at(0).in, xSliderTotal, current_playlist_index); // set current frame in player
#ifdef APP5_ACES
		else if (mImfApplication == ::App5)
			mpACESPlayer->setPos(rPlayList.at(0).in, xSliderTotal, current_playlist_index); // set current frame in player
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K))
			mpHTJ2KPlayer->setPos(rPlayList.at(0).in, xSliderTotal, current_playlist_index); // set current frame in player
#endif
	}
	else {
		xSliderFrame = 0;
		xSliderTotal = 0;
		decodingFrame = 0;
		decoding_time->setText("No/empty playlist!");
		Clear(); // stop player & clear preview
	}
}

void WidgetVideoPreview::setMenuQuality(int rWidth, int rHeight) {
	menuQuality->clear(); // clear prev. resolutions from menu
	for (int i = 0; i <= 5; i++) {
		int w = rWidth / (1 << i);
		int h = rHeight / (1 << i);
		qualities[i] = new QAction(QString("%1 x %2").arg(w).arg(h)); // create new action
		qualities[i]->setData(i);
		qualities[i]->setCheckable(true);
		if (i == decode_layer) qualities[i]->setChecked(true); // default
		menuQuality->addAction(qualities[i]);
	}
}

void WidgetVideoPreview::rChangeSpeed(QAction *action) {
	
	int decode_speed_index = decode_speed;
	if (decode_speed > 30) decode_speed_index = 30 + (decode_speed - 30) / 5;

	if (decode_speed != action->data().value<int>()) { // value has changed

		speeds[decode_speed_index]->setChecked(false); // uncheck 'old' value
		decode_speed = action->data().value<int>();
		decode_speed_index = decode_speed;
	
		bool was_playing = player->playing;
		if (playerThread->isRunning()) playerThread->quit();
		player->setFps(decode_speed);
		player->clean();

		if (was_playing) {
			player->playing = true;
			playerThread->start(QThread::TimeCriticalPriority); // resume playback
		}
#ifdef APP5_ACES
		was_playing = mpACESPlayer->playing;
		if (mpACESPlayerThread->isRunning()) mpACESPlayerThread->quit();
		mpACESPlayer->setFps(decode_speed);
		mpACESPlayer->clean();

		if (was_playing) {
			mpACESPlayer->playing = true;
			mpACESPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
		}

#endif
#ifdef CODEC_HTJ2K
		was_playing = mpHTJ2KPlayer->playing;
		if (mpHTJ2KPlayerThread->isRunning()) mpHTJ2KPlayerThread->quit();
		mpHTJ2KPlayer->setFps(decode_speed);
		mpHTJ2KPlayer->clean();

		if (was_playing) {
			mpHTJ2KPlayer->playing = true;
			mpHTJ2KPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
		}

#endif
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
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
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
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			mpACESPlayer->setLayer(decode_layer);
			mpACESDecoders[0]->setLayer(decode_layer);
			mpACESDecoders[1]->setLayer(decode_layer);

			// reload the same frame again (if player is not playing)
			if (!mpACESPlayer->playing) {
				now_running = !now_running; // use same decoder (relevant frame is still set)
				mpACESDecodingThreads[(int)now_running]->start(QThread::HighestPriority); // start decoder (again)
				decoding_time->setText("loading...");
			}
		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			mpHTJ2KPlayer->setLayer(decode_layer);
			mpHTJ2KDecoders[0]->setLayer(decode_layer);
			mpHTJ2KDecoders[1]->setLayer(decode_layer);

			// reload the same frame again (if player is not playing)
			if (!mpHTJ2KPlayer->playing) {
				now_running = !now_running; // use same decoder (relevant frame is still set)
				mpHTJ2KDecodingThreads[(int)now_running]->start(QThread::HighestPriority); // start decoder (again)
				decoding_time->setText("loading...");
			}
		}
#endif
	}
}

void WidgetVideoPreview::rChangeProcessing(QAction *action) {
	
	int nr = action->data().value<int>();
	bool player_playing;
	if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM))
		player_playing = player->playing;
#ifdef APP5_ACES
	else if (mImfApplication == ::App5)
		player_playing = mpACESPlayer->playing;
#endif
#ifdef CODEC_HTJ2K
	else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K))
		player_playing = mpHTJ2KPlayer->playing;
#endif

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
			processing_extract_actions[10]->setChecked(true); // center center is ON
			processing_extract_action = 10;
			mpImagePreview->setExtract(4);
		}
		break;
	case 2: // Data Window, default
		processing_extract_actions[nr]->setChecked(true); // Data Window is on
		processing_extract_actions[nr+1]->setChecked(false); // Display Window is off
		this->setMenuQuality(mStoredWidth, mStoredHeight);
		if (mImfApplication == ::App5) {
#ifdef APP5_ACES
			mpACESPlayer->showActiveArea(false);
			mpACESDecoders[0]->showActiveArea(false);
			mpACESDecoders[1]->showActiveArea(false);
			// reload the same frame again (if player is not playing)
			if (!mpACESPlayer->playing) {
				now_running = !now_running; // use same decoder (relevant frame is still set)
				mpACESDecodingThreads[(int)now_running]->start(QThread::HighestPriority); // start decoder (again)
				decoding_time->setText("loading...");
			}
#endif
		}
		break;
	case 3:
		processing_extract_actions[nr]->setChecked(true); // Display Window is on
		processing_extract_actions[nr-1]->setChecked(false); // Data Window is off
		this->setMenuQuality(mDisplayWidth, mDisplayHeight);
		if (mImfApplication == ::App5) {
#ifdef APP5_ACES
			mpACESPlayer->showActiveArea(true);
			mpACESDecoders[0]->showActiveArea(true);
			mpACESDecoders[1]->showActiveArea(true);
			// reload the same frame again (if player is not playing)
			if (!mpACESPlayer->playing) {
				now_running = !now_running; // use same decoder (relevant frame is still set)
				mpACESDecodingThreads[(int)now_running]->start(QThread::HighestPriority); // start decoder (again)
				decoding_time->setText("loading...");
			}
#endif
		}
		break;
	case 4: // Show subtitles
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
			player->show_subtitles = action->isChecked();
			this->showTTML = action->isChecked();
			if (!player->show_subtitles) {
				emit ttmlChanged(QVector<visibleTTtrack>(), ttml_search_time.elapsed()); // clear subtitle preview
				mpImagePreview->ttml_regions.clear();
				mpImagePreview->paintGL();
			} else {
				emit ttmlChanged(current_tt, ttml_search_time.elapsed());
				getTTML();
				mpImagePreview->paintGL();
			}
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			mpACESPlayer->show_subtitles = action->isChecked();
			if (!mpACESPlayer->show_subtitles) {
				emit ttmlChanged(QVector<visibleTTtrack>(), ttml_search_time.elapsed()); // clear subtitle preview
			}
		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			mpHTJ2KPlayer->show_subtitles = action->isChecked();
			if (!mpHTJ2KPlayer->show_subtitles) {
				emit ttmlChanged(QVector<visibleTTtrack>(), ttml_search_time.elapsed()); // clear subtitle preview
			}
		}
#endif
		break;
	case 5: // real speed

		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
			if(playerThread->isRunning()) playerThread->quit();
			player->realspeed = action->isChecked();
			player->clean();

			if (player_playing) {
				player->playing = true;
				playerThread->start(QThread::TimeCriticalPriority); // resume playback
			}
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			if(mpACESPlayerThread->isRunning()) mpACESPlayerThread->quit();
			mpACESPlayer->realspeed = action->isChecked();
			mpACESPlayer->clean();

			if (player_playing) {
				mpACESPlayer->playing = true;
				mpACESPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
			}
		}
#endif
#ifdef CODEC_HTJ2K
		else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			if(mpHTJ2KPlayerThread->isRunning()) mpHTJ2KPlayerThread->quit();
			mpHTJ2KPlayer->realspeed = action->isChecked();
			mpHTJ2KPlayer->clean();

			if (player_playing) {
				mpHTJ2KPlayer->playing = true;
				mpHTJ2KPlayerThread->start(QThread::TimeCriticalPriority); // resume playback
			}
		}
#endif
		break;
	case 6: // color space conversion
		if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
			if ((mImfApplication == ::App2) || (mImfApplication == ::App2e) || (mImfApplication == ::App4) || (mImfApplication == ::App4DCDM)) {
				decoders[0]->convert_to_709 = action->isChecked(); // set in decoder 0
				decoders[1]->convert_to_709 = action->isChecked(); // set in decoder 1
				player->convert_to_709(action->isChecked()); // set in player
			}
#ifdef CODEC_HTJ2K
			else if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
				mpHTJ2KDecoders[0]->convert_to_709 = action->isChecked(); // set in decoder 0
				mpHTJ2KDecoders[1]->convert_to_709 = action->isChecked(); // set in decoder 1
				mpHTJ2KPlayer->convert_to_709(action->isChecked());
			}
#endif
			if (!player_playing) {
				decoding_time->setText("loading...");
				now_running = !now_running; // use same decoder (relevant frame is still set)
				if ((mImfApplication == ::App4DCDM_HTJ2K) || (mImfApplication == ::App2e_HTJ2K)) {
#ifdef CODEC_HTJ2K
					mpHTJ2KDecodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)
#endif
				} else {
					// reload current preview
					decodingThreads[(int)(now_running)]->start(QThread::HighestPriority); // start decoder (again)
				}
			}
		}
#ifdef APP5_ACES
		else if (mImfApplication == ::App5) {
			action->setChecked(true);
		}
#endif

		break;
	case 7: // save image
		mpImagePreview->saveImage();
		break;
	}

	if (nr >= 8) {
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
		mpImagePreview->setExtract((nr - 8));
	}
}

void WidgetVideoPreview::getTTML() {

	float time = ((float)xSliderTotal / CPLEditRate);

	ttml_search_time.start();
	current_tt = QVector<visibleTTtrack>();
	mpImagePreview->ttml_regions.clear();

	float frac_sec;
	double seconds, rel_time;
	int rel_frame;
	QString h, m, s, f;

	// loop tracks
	QMap<int, QVector<TTMLtimelineResource>>::iterator x;
	for (x = ttml_tracks.begin(); x != ttml_tracks.end(); ++x) { // i.key(), i.value()

		// loop segments in track
		for (int i = 0; i < x.value().length(); i++) {

			TTMLtimelineResource resource = x.value().at(i);

			float beg_rounded = ceil(qRound(resource.timeline_in * CPLEditRate * 100.f) / 100.f);
			float end_rounded = ceil(qRound(resource.timeline_out * CPLEditRate * 100.f) / 100.f);

			// loop tt elements within segment
			if (xSliderTotal >= beg_rounded && xSliderTotal < end_rounded) {
				// visible segment found -> search inside segment

				visibleTTtrack tt;
				tt.resource = resource;

				if (resource.RepeatCount > 0) { // RepeatCount != 0
					if (modf((time - resource.timeline_in) / (resource.out - resource.in), &seconds) > 0.99) {
						rel_time = resource.in;
					}
					else {
						rel_time = modf((time - resource.timeline_in) / (resource.out - resource.in), &seconds) * (resource.out - resource.in) + resource.in;
					}
					rel_frame = (qRound((time - resource.timeline_in)*CPLEditRate) % qRound((resource.out - resource.in) * CPLEditRate)) + resource.in * CPLEditRate;
				}
				else { // no repeating elements
					rel_time = ((time - resource.timeline_in) + resource.in);
					rel_frame = qRound(((time - resource.timeline_in) + resource.in)*CPLEditRate);
				}

				frac_sec = modf(rel_time, &seconds);

				h = QString("%1").arg((int)(seconds / 3600.0f), 2, 10, QChar('0')); // hours
				m = QString("%1").arg((int)(seconds / 60.0f), 2, 10, QChar('0')); // minutes
				s = QString("%1").arg((int)(seconds) % 60, 2, 10, QChar('0')); // seconds
				tt.formatted_time = QString("%1 : %2 : %3").arg(h).arg(m).arg(s); // ttml timecode
				tt.fractional_frames = QString::number(qRound(frac_sec * resource.frameRate * (float)100) / (float)100, 'f', 2);

				for (int z = 0; z < resource.items.length(); z++) {

					TTMLelem ttelem = resource.items.at(z);
					float beg_rounded = ceil(qRound(ttelem.beg * CPLEditRate * 100.f) / 100.f);
					float end_rounded = ceil(qRound(ttelem.end * CPLEditRate * 100.f) / 100.f);

					if (rel_frame >= beg_rounded && rel_frame < end_rounded) {

						// add visible element
						tt.elements.append(ttelem);

						if (ttelem.type == 1) {
							ttelem.region.bgImage = ttelem.bgImage; // image
						}

						// append region
						mpImagePreview->ttml_regions.append(ttelem.region);
					}
				}
				current_tt.append(tt); // add visible track
			}
		}
	}

	emit ttmlChanged(current_tt, ttml_search_time.elapsed());
}

void WidgetVideoPreview::rPrevNextSubClicked(bool direction) {

	// calculate frame indicator time
	float next = -1, prev = -1, item_begin = 0;
	int repeat = 1;

	// loop tracks
	QMap<int, QVector<TTMLtimelineResource>>::iterator x;
	for (x = ttml_tracks.begin(); x != ttml_tracks.end(); ++x) { // i.key(), i.value()
		// loop resources within track
		for (int i = 0; i < x.value().count(); i++) {
			TTMLtimelineResource resource = x.value().at(i);

			if (resource.RepeatCount > 1) {
				repeat = resource.RepeatCount;
			}
			else {
				repeat = 1; // default
			}
#ifdef DEBUG_JP2K
					qDebug() << "resource.in" << resource.in << "resource.timeline_in" << resource.timeline_in
							<< "resource.out" << resource.out << "resource.timeline_out" << resource.timeline_out;
#endif

			// loop repeated elements
			for (int ii = 0; ii < repeat; ii++) {
				// loop items within resource
				for (int iii = 0; iii < resource.items.length(); iii++) {
					item_begin = ceil(qRound((resource.items.at(iii).beg - resource.in + resource.timeline_in) * CPLEditRate * 100.f) / 100.f) + qRound((resource.out - resource.in)*ii * CPLEditRate);
					if (item_begin < 0) item_begin = 0;
#ifdef DEBUG_JP2K
					qDebug() << "item" << item_begin << "resource.items.at(iii).beg" << resource.items.at(iii).beg;
#endif

					if (prev == -1) {
						prev = item_begin; // initialize prev
#ifdef DEBUG_JP2K
						qDebug() << "init prev.";
#endif
					}

					if (next == -1 && item_begin > xSliderTotal) { // make sure there is at least one frame difference to current frame
						next = item_begin; // initialize next
#ifdef DEBUG_JP2K
						qDebug() << "init next.";
#endif
					}

					if (item_begin < xSliderTotal && item_begin > prev) { // make sure there is at least one frame difference to current frame
						prev = item_begin;
#ifdef DEBUG_JP2K
						qDebug() << "update prev.";
#endif
					}
				}
			}
		}
	}
#ifdef DEBUG_JP2K
	qDebug() << "CPL fps:" << CPLEditRate << "frame indicator" << xSliderTotal << "prev" << prev << "next" << next;
#endif

	if (next == -1) { // no next found, use last time
		next = item_begin;
	}

	if (direction == true) { 
		emit currentPlayerPosition(next); // next
#ifdef DEBUG_JP2K
		qDebug() << "set NEXT to qRound of:" << (next * CPLEditRate);
#endif
	}
	else {
		emit currentPlayerPosition(prev); // previous
#ifdef DEBUG_JP2K
		qDebug() << "set PREV to qRound of:" << (prev * CPLEditRate);
#endif
	}
}

void WidgetVideoPreview::setApplication(eImfApplications rImfApplication) {
//	qDebug() << "Setting Application to" << rImfApplication;
	mImfApplication = rImfApplication;
}

eImfApplications WidgetVideoPreview::getApplication() {
	return mImfApplication;
}

