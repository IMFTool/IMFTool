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
#include "ImfPackage.h"
#include <QWidget>
#include <QThread>
#include "JP2K_Preview.h"
#include "qcombobox.h"
#include "qxmlstream.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTextEdit>
#include <QMessageBox>
#ifdef APP5_ACES
#include "ACES_Preview.h"
#endif
#ifdef CODEC_HTJ2K
#include "HTJ2K_Preview.h"
#endif
#include "Player.h"

class WidgetImagePreview;
class QPushButton;
class QLabel;

class WidgetVideoPreview : public QWidget {

	Q_OBJECT

public:
	WidgetVideoPreview(QWidget *pParent = NULL);
	void InstallImp();
	void UninstallImp();
	void setPlaylist(QVector<VideoResource> &rPlayList, QVector<TTMLtimelineResource> &rTTMLs);
	void setMenuQuality(int rWidth, int rHeight);
	Player *player;
#ifdef APP5_ACES
	Player *mpACESPlayer;
#endif
#ifdef CODEC_HTJ2K
	Player *mpHTJ2KPlayer;
#endif
	void setApplication(eImfApplications rImfApplication);
	eImfApplications getApplication();
	~WidgetVideoPreview();
	void Clear();
	void Reset();
	float CPLEditRate;
signals:
	void currentPlayerPosition(qint64);
	void ShowImage(const QImage&);
	void ttmlChanged(const QVector<visibleTTtrack>&, int);
	void regionOptionsChanged(int);
public slots :
	void xPosChanged(const QSharedPointer<AssetMxfTrack>&, const qint64&, const Timecode&, const int&);
	void rShowMsgBox(const QString&, int);
	void rPrevNextSubClicked(bool);
	void getTTML();
	void forwardPlayerPosition(qint64 ,bool);
private slots:
	void rChangeSpeed(QAction*);
	void rChangeQuality(QAction*);
	void rChangeProcessing(QAction*);
	void rPlayPauseButtonClicked(bool checked);
	void rPlaybackEnded();
	void decodingStatus(qint64, QString);
	void stopPlayback(bool clicked);
	void rViewFullScreen();
public slots:
	virtual void keyPressEvent(QKeyEvent *pEvent);

private:
	Q_DISABLE_COPY(WidgetVideoPreview);
	void InitLayout();

	// TTML
	bool showTTML = true;
	QVector<TTMLtimelineResource> *ttmls;
	QVector<visibleTTtrack> current_tt; // currently visible timed text elements
	QMap<int, QVector<TTMLtimelineResource>> ttml_tracks;
	QTime ttml_search_time;
	qint64 next_ttml;
	qint64 prev_ttml;

	// widgets
	WidgetImagePreview *mpImagePreview;

	// player control ui
	QMenuBar *menuBar;
	QMenu *menuSpeed;
	QAction *speeds[50];
	QMenu *menuQuality;
	QAction *qualities[6];
	QMenu *menuProcessing;
	QMenu *menuView;
	QAction *view_actions[1];
	QMenu *processing_extract;
	QStringList *processing_extract_names;
	QAction *processing_extract_actions[17];
	int processing_extract_action = 3; // default
	QPushButton *mpStopButton;
	QPushButton *mpPlayPauseButton;
	QLabel *labelTest;
	QLabel *decoding_time;
	QLabel *labelImg;
	QComboBox *set_fps;
	QComboBox *set_layer;
	QMessageBox *mpMsgBox;

	// player
	int decode_layer = 2; // default
	int decode_speed = 5; // default (fps in player)
	QThread *playerThread;
#ifdef APP5_ACES
	QThread *mpACESPlayerThread;
#endif
#ifdef CODEC_HTJ2K
	QThread *mpHTJ2KPlayerThread;
#endif
	eImfApplications mImfApplication = ::App2;
	int current_playlist_index = 0; // frame indicator position within the playlisqt
	QVector<VideoResource> currentPlaylist; // playlist ressources
	bool setFrameIndicator = false; // ignore next signal xPosChanged?
	QSharedPointer<AssetMxfTrack> currentAsset;

	// preview decoding
	JP2K_Preview *decoders[2];
	QThread *decodingThreads[2];
#ifdef APP5_ACES
	ACES_Preview *mpACESDecoders[2];
	QThread *mpACESDecodingThreads[2];
#endif
#ifdef CODEC_HTJ2K
	HTJ2K_Preview *mpHTJ2KDecoders[2];
	QThread *mpHTJ2KDecodingThreads[2];
	bool mHTJ2KRunning[2];
#endif
	bool running[2];
	bool mACESRunning[2];
	bool now_running = false; // false: 0, true: 1
	int run;
	
	qint64 decodingFrame = 0; // currently decoding frame
	qint64 xSliderFrame = 0;
	qint64 xSliderTotal = 0;
	int mStoredWidth = 0;
	int mStoredHeight = 0;
	int mDisplayWidth = 0;
	int mDisplayHeight = 0;
};
