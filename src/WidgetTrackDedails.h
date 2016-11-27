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
#include "ImfCommon.h"
#include "GraphicsCommon.h"
#include <QFrame>


class QLabel;
class QButtonGroup;
class QToolButton;

class AbstractWidgetTrackDetails : public QFrame {

	Q_OBJECT

public:
	AbstractWidgetTrackDetails(QWidget *pParent = NULL);
	virtual ~AbstractWidgetTrackDetails() {}
	virtual QUuid GetId() const = 0;
	virtual eSequenceType GetType() const = 0;

signals:
	void HeightAboutToChange(qint32 height);
	void DeleteClicked(const QUuid &rId);

protected:
	virtual void resizeEvent(QResizeEvent *pEvent);
};


class WidgetTrackDetailsTimeline : public AbstractWidgetTrackDetails {

	Q_OBJECT

private:
	enum eButtons {
		ButtonSettings = 0,
		ButtonLock
	};

public:
	WidgetTrackDetailsTimeline(QWidget *pParent = NULL);
	virtual ~WidgetTrackDetailsTimeline() {}
	virtual QUuid GetId() const { return QUuid(); }
	virtual eSequenceType GetType() const { return Unknown; }

signals:
	void LockToggled(bool enabled);

	public slots:
	void SetTimecode(const Timecode &rTimeCode);

	private slots:
	void 	rButtonClicked(int id);
	void 	rButtonToggled(int id, bool checked);

private:
	Q_DISABLE_COPY(WidgetTrackDetailsTimeline);
	void InitLayout();

	QLabel *mpTimecodeLabel;
	QButtonGroup *mpButtonGroup;
	//QLabel *labelTest; // (k)
};


class WidgetTrackDetails : public AbstractWidgetTrackDetails {

	Q_OBJECT

public:
	WidgetTrackDetails(const QUuid &rTrackId, eSequenceType type, QWidget *pParent = NULL);
	virtual ~WidgetTrackDetails() {}
	virtual QSize sizeHint() const;
	virtual QUuid GetId() const { return mTrackId; }
	virtual eSequenceType GetType() const { return mType; }

	private slots:
	void DeleteAction() { if(mType != MainImageSequence) emit DeleteClicked(mTrackId); }
	void EnableDeleteAction();

private:
	Q_DISABLE_COPY(WidgetTrackDetails);
	void InitLayout();

	QAction *mpDelete;
	QUuid mTrackId;
	eSequenceType mType;
};


class WidgetAudioTrackDetails : public AbstractWidgetTrackDetails {

	Q_OBJECT

public:
	WidgetAudioTrackDetails(const QUuid &rTrackId, QWidget *pParent = NULL);
	virtual ~WidgetAudioTrackDetails() {}
	virtual QSize sizeHint() const;
	virtual QUuid GetId() const { return mTrackId; }
	virtual eSequenceType GetType() const { return MainAudioSequence; }
	QToolButton* GetSoloButton() const { return mpSoloButton; }

	private slots:
	void DeleteAction() { emit DeleteClicked(mTrackId); }

private:
	Q_DISABLE_COPY(WidgetAudioTrackDetails);
	void InitLayout();

	QUuid mTrackId;
	eSequenceType mType;
	QToolButton *mpSoloButton;
};
