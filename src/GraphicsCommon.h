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
#include "GraphicScenes.h"
#include "GraphicsViewScaleable.h"
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>


#define CPL_COLOR_TIMELINE_TOP 125, 124, 127
#define CPL_COLOR_TIMELINE_BOTTOM 108, 109, 113
#define CPL_COLOR_RESOURCE_NOT_DROPPABLE 198, 43, 43
#define CPL_COLOR_VIDEO_RESOURCE 116, 102, 171
#define CPL_COLOR_AUDIO_RESOURCE 110, 162, 110
#define CPL_COLOR_TIMED_TEXT_RESOURCE 191, 159, 72
#define CPL_COLOR_ISXD_RESOURCE 176, 180, 142
#define CPL_COLOR_IAB_RESOURCE 153, 72, 191
#define CPL_COLOR_ANC_RESOURCE 187, 104, 23
#define CPL_COLOR_DUMMY_RESOURCE 129, 129, 129
#define CPL_COLOR_MARKER_RESOURCE 255, 255, 255, 50
#define CPL_COLOR_DURATION_INDICATOR 255, 0, 93
#define CPL_COLOR_DEFAULT_SNAP_INDICATOR 255, 0, 93
#define CPL_COLOR_DEFAULT_MARKER 255, 111, 79
#define CPL_COLOR_CURRENT_FRAME_INDICATOR 255, 194, 0
#define CPL_COLOR_BACKGROUND 33, 33, 33
#define CPL_BORDER_COLOR 83, 83, 85
#define CPL_FONT_COLOR 58, 58, 58
#define CPL_COLOR_TIMELINE_TEXT_MARK 0, 0, 0

enum eSequenceType {
	AncillaryDataSequence = (1u << 0),
	CommentarySequence = (1u << 1),
	HearingImpairedCaptionsSequence = (1u << 2),
	KaraokeSequence = (1u << 3),
	MainAudioSequence = (1u << 4),
	MainImageSequence = (1u << 5),
	SubtitlesSequence = (1u << 6),
	VisuallyImpairedTextSequence = (1u << 7),
	MarkerSequence = (1u << 8),
	IABSequence  = (1u << 9),  // SMPTE ST 2067-201
	ISXDSequence  = (1u << 10), // SMPTE RDD 47
	ForcedNarrativeSequence = (1u << 11), // SMPTE ST 2067-2:2020
	Unknown = (1u << 12)
};


enum eGraphicsItemType {

	GraphicsWidgetBaseType = QGraphicsItem::UserType + 1,
	GraphicsObjectBaseType,
	GraphicsObjectVerticalIndicatorType,
	GraphicsWidgetCompositionType,
	GraphicsWidgetFileResourceType,
	GraphicsWidgetDummyResourceType,
	GraphicsWidgetVideoResourceType,
	GraphicsWidgetAudioResourceType,
	GraphicsWidgetTimedTextResourceType,
	GraphicsWidgetAncillaryDataResourceType,
	GraphicsWidgetMarkerResourceType,
	GraphicsWidgetMarkerType,
	GraphicsWidgetTimelineType,
	GraphicsWidgetDrawnTimelineType,
	GraphicsWidgetIABResourceType,
	GraphicsWidgetISXDResourceType
};


enum eGridPosition {
	VideoHorizontal = (1u << 0),
	AudioHorizontal = (1u << 1),
	TimedTextHorizontal = (1u << 2),
	DataHorizontal = (1u << 3),
	MarkerHorizontal = (1u << 4),
	ISXDHorizontal = (1u << 5),
	IABHorizontal = (1u << 6),
	Vertical = (1u << 7),
};


class QTimer;

class AbstractGridExtension {

	friend GraphicsSceneBase::GridInfo GraphicsSceneBase::SnapToGrid(const QPointF &rPoint, GridPosition which, const QRectF &rSearchRect, QList<AbstractGridExtension*> ignoreItems) const;

public:
	AbstractGridExtension() {}
	virtual ~AbstractGridExtension() {}

protected:
	/*! \brief
	You have to implement this method by aligning the rPoint to the grid and return true. Otherwise return false
	"which" specifies the grid. rPoint is given in scene coordinates.
	*/
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const = 0;
	virtual qreal HeightAdviceForHorizontalGrid() const { return -1; }
	virtual QColor ColorAdviceForGrid() const { return QColor(CPL_COLOR_DEFAULT_SNAP_INDICATOR); }

private:
	Q_DISABLE_COPY(AbstractGridExtension);
};


//! This is the base class for every graphics widget in the composition scene.
/*!
Every graphics widget derived from this class may extend the snap grid by reimplementing
GraphicsWidgetBase::ExtendGrid().
*/
class GraphicsWidgetBase : public QGraphicsWidget, public AbstractGridExtension {

public:
	GraphicsWidgetBase(QGraphicsItem *pParent = NULL);
	virtual ~GraphicsWidgetBase() {}
	virtual int type() const { return GraphicsWidgetBaseType; }

protected:
	EditRate GetCplEditRate() const;
	virtual void CplEditRateChanged() {}
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const { return false; }
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &rValue);
	virtual void customEvent(QEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsWidgetBase);
};


//! This is the base class for every graphics object in the composition scene.
/*!
Every graphics object derived from this class may extend the snap grid by reimplementing
GraphicsItemBase::ExtendGrid().
*/
class GraphicsObjectBase : public QGraphicsObject, public AbstractGridExtension {

public:
	GraphicsObjectBase(QGraphicsItem *pParent = NULL);
	virtual ~GraphicsObjectBase() {}
	virtual int type() const { return GraphicsObjectBaseType; }

protected:
	EditRate GetCplEditRate() const;
	virtual void CplEditRateChanged() {}
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const { return false; }
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &rValue);
	virtual void customEvent(QEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsObjectBase);
};


// vertical timeline frame indicator
class GraphicsObjectVerticalIndicator : public GraphicsObjectBase, public AbstractViewTransformNotifier {

	Q_OBJECT

private:
	class GraphicsItemLine : public QGraphicsItem {

	public:
		GraphicsItemLine(qreal width, qreal height, GraphicsObjectVerticalIndicator *pParent, const QColor &rColor);
		virtual ~GraphicsItemLine() {}
		virtual int type() const { return GraphicsObjectVerticalIndicatorType; }
		virtual QRectF boundingRect() const;
		virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
		void SetColor(const QColor &rColor) { mColor = rColor; update(); }
		void SetSize(const QSize &rSize) { prepareGeometryChange(); mLineSize = rSize; }

	private:
		Q_DISABLE_COPY(GraphicsItemLine);

		QColor mColor;
		QSize mLineSize;
	};

public:
	GraphicsObjectVerticalIndicator(qreal width, qreal height, const QColor &rColor, QGraphicsItem *pParent = NULL);
	virtual ~GraphicsObjectVerticalIndicator() {};
	virtual QRectF boundingRect() const;
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	QColor GetColor() const { return mColor; }
	QSize GetHeadSize() const { return boundingRect().size().toSize(); }
	void SetColor(const QColor &rColor) { mColor = rColor; update(); mpLine->SetColor(rColor); }
	//! This works only if no head image was set before.
	void SetHeadSize(const QSize &rSize) { prepareGeometryChange(); mHeadSize = rSize; }
	void SetHeadWidth(int width) { SetHeadSize(QSize(width, mHeadSize.height())); }
	void SetHeadHeight(int height) { SetHeadSize(QSize(mHeadSize.width(), height)); }
	void SetHeadImage(const QPixmap &rPic) { prepareGeometryChange(); mHeadImage = rPic; }
	void SetHeadText(const QString &rText) { mText = rText; update(); }
	void SetWidth(int width) const { mpLine->SetSize(QSize(width, mpLine->boundingRect().height())); }
	void SetHeight(int height) const { mpLine->SetSize(QSize(mpLine->boundingRect().width(), height)); }
	void SetSize(const QSize &rSize) const { mpLine->SetSize(rSize); }
	void RemoveHeadImage() { prepareGeometryChange(); mHeadImage = QPixmap(); }
	void RemoveHeadText() { mText.clear(); update(); }

	void ShowHead() { setFlag(QGraphicsItem::ItemHasNoContents, false); }
	void HideHead() { setFlag(QGraphicsItem::ItemHasNoContents, true); }
	void ShowLine() const { mpLine->setFlag(QGraphicsItem::ItemHasNoContents, false); }
	void HideLine() const { mpLine->setFlag(QGraphicsItem::ItemHasNoContents, true); }
	void EnableGridExtension(bool enable) { mExtendGrid = enable; }
	void DisableGridExtension(bool disable) { mExtendGrid = !disable; }

signals:
	void XPosChanged(qreal xPos);

public slots:
	void SetXPos(qreal xPos);

protected:
	virtual void ViewTransformEvent(const QTransform &rViewTransform);
	QVariant itemChange(GraphicsItemChange change, const QVariant &rValue);
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const;
	virtual QColor ColorAdviceForGrid() const { return mColor; }
	virtual QGraphicsView* GetObservableView() const;

private:
	Q_DISABLE_COPY(GraphicsObjectVerticalIndicator);

	QColor mColor;
	GraphicsItemLine *mpLine;
	QPixmap mHeadImage;
	QString mText;
	QSize mHeadSize;
	bool mExtendGrid;
};


class GraphicsWidgetHollowProxyWidget : public QGraphicsWidget, public AbstractViewTransformNotifier {

	Q_OBJECT

private:
	class GraphicsItemHover : public QGraphicsItem {

	public:
		GraphicsItemHover(QGraphicsItem *pParent);
		virtual ~GraphicsItemHover() {};
		virtual QRectF boundingRect() const;
		virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
		void SetHeight(qreal height) { prepareGeometryChange(), mHeight = height; }

	protected:
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);

	private:
		qreal mHeight;
	};

	class GraphicsProxyWidget : public QGraphicsProxyWidget {

	public:
		GraphicsProxyWidget(QGraphicsItem *pParent) : QGraphicsProxyWidget(pParent) { setAcceptHoverEvents(true); }
		virtual ~GraphicsProxyWidget() {}

	protected:
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent);
	};

public:
	GraphicsWidgetHollowProxyWidget(QGraphicsItem *pParent = NULL);
	virtual ~GraphicsWidgetHollowProxyWidget() {}
	//! Calls QGraphicsProxyWidget::setWidget().
	void SetWidget(QWidget *pWidget);
	//! Calls QGraphicsProxyWidget::widget().
	QWidget* GetWidget() const;

private slots:
void HideProxyWidget() { mpProxyWidget->hide(); }
void ShowProxyWidget() { mpProxyWidget->show(); }

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
	virtual void ViewTransformEvent(const QTransform &rViewTransform);
	virtual QGraphicsView* GetObservableView() const;
	virtual void resizeEvent(QGraphicsSceneResizeEvent *pEvent);

private:
	void HoverItemActive(bool active);
	void HoverProxyActive(bool active);

	GraphicsItemHover *mpHover;
	GraphicsProxyWidget *mpProxyWidget;
	QTimer *mpTimerHideProxy;
	QTimer *mpTimerShowProxy;
};


class GraphicsHelper {

public:
	static qreal GetDefaultFontHeight();
	static QColor GetSegmentColor(int SegmentIndex, bool transparent = false);
};
