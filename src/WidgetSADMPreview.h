/* Copyright(C) 2024 Wolfgang Ruppel
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
#include <QDebug>
#include <QWidget>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QMap>
#include <QWizard>
#include <QWizardPage>
#include "ImfCommon.h"
#include "ImfPackage.h"

class MyTableView;

class WidgetSADMPreview : public QWidget {
	Q_OBJECT
public:
	WidgetSADMPreview(QWidget *pParent = NULL);
	void ClearSADM();
private:
	void InitLayout();
	void createButton(int, int, bool);
	void updateSadm(QString, int);

	QTextEdit *mSadmText;
	QTextEdit *mSadmPopUpText;
	QMap<quint8, QStringList> mSadmTextMap;
	QMap<quint8, int> mSadmScrollPositionMap;
	QMap<quint8, QUuid> mMGASADMTracks;
	int mLastActiveTrack = -1;
	QCheckBox *mWrapText;
	QCheckBox *mKeepScrollPosition;
	bool mWrapTextEnabled = true;
	bool mKeepScrollPos = true;
	bool mOutOfTimeline = false;
	QPushButton *mCopyClipboardButton;
	QPushButton *mPopOutButton;
	QPushButton *mNextButton;
	QPushButton *mPrevButton;
	QPushButton* mHighlightedButton;
	QSplitter *mSplitter;
	QStandardItemModel *mTableModel;
	QStandardItemModel *mTableModelPopUp;
	MyTableView *mTableView;
	MyTableView *mTableViewPopUp;
	Kumu::FileReaderFactory defaultFactory;
	QWizard *mSadmPopupWizard;
	QWizardPage *mSadmPageWizard;

private slots:
	void prevClicked();
	void nextClicked();
	void copyClipboard();
	void sadmPopOut();
	void wrapTextChanged(int);
	void keepScrollPosChanged(int);
	void pushButtonClicked();
public slots:
	void rSetSadmTimeline(const QMap<quint8, QUuid>&);
	void rXPosChanged(const QSharedPointer<AssetMxfTrack>&, const qint64&, const Timecode&, const QUuid&, const quint8&);
	void rOutOfTimeline(const qint64&);
signals:
	void PrevClicked();
	void NextClicked();
};

class MyTableView : public QTableView {
public:
MyTableView(QWidget* parent);

QSize sizeHint() const;

QSize minimumSizeHint() const;

};

