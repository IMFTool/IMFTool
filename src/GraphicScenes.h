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
#include <QGraphicsScene>


class QUndoCommand;
class AbstractGridExtension;
class AbstractGraphicsWidgetResource;
class GraphicsWidgetTimeline;
class GraphicsWidgetComposition;
class GraphicsObjectVerticalIndicator;
class GraphicsWidgetSegmentIndicator;
class GraphicsWidgetSegment;

typedef unsigned int GridPosition;
typedef unsigned int SequenceTypes;

class GraphicsSceneBase : public QGraphicsScene {

	Q_OBJECT

public:

	struct GridInfo {
		bool IsHoizontalSnap;
		bool IsVerticalSnap;
		QPointF SnapPos;
		QColor ColorAdvice;
		qreal HeightAdvice;
		AbstractGridExtension *HorizontalOrigin;
		AbstractGridExtension *VerticalOrigin;
	};

	GraphicsSceneBase(const EditRate &rCplEditRate, QObject *pParent = NULL);
	virtual ~GraphicsSceneBase() {}
	EditRate GetCplEditRate() const { return mCplEditRate; }
	void SetCplEditRate(const EditRate &rCplEditRate);
	/*! Snaps the point rPoint to the grid. The search rect specifies a subset of the grid as finding the graphics items intersecting with the search rect and expanding the snap grid.
	The scene may specify items that can't be excluded from search (e.g.: the current-frame-indicator).
	If the search rects width and/or height is less or equal 0 all potential items in the scene will be taken into account.
	If you want to exclude graphics items use the list ignoreItems.
	"which" is a ored combination of eGridPosition.
	rPoint, rSearchRect and the return value are given in scene coordinates.
	*/
	GridInfo SnapToGrid(const QPointF &rPoint, GridPosition which, const QRectF &rSearchRect = QRectF(), QList<AbstractGridExtension*> ignoreItems = QList<AbstractGridExtension*>()) const;
	GridInfo SnapToGrid(const QPointF &rPoint, GridPosition which, const QRectF &rSearchRect = QRectF(), AbstractGridExtension *pIgnoreItem = NULL) const;
	void SetSnapWidth(int width) { mSnapWidth = width; }

signals:
	void ClearSelectionRequest();

	private slots:
	void rSceneRectChanged(const QRectF &rRect) { GraphicsSceneResizeEvent(rRect); }

protected:
	//! Resizing an item in this method may cause infinite recursion if the bounding rect of the item extends the scene rect and sceneRect property is unset or set to null.
	virtual void GraphicsSceneResizeEvent(const QRectF &rNewSceneRect) {}
	virtual QList<AbstractGridExtension*> AddPermanentSnapItems() const { return QList<AbstractGridExtension*>(); }
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsSceneBase);

	int mSnapWidth;
	EditRate mCplEditRate;
};


class GraphicsSceneComposition : public GraphicsSceneBase {

	Q_OBJECT

public:
	GraphicsSceneComposition(const EditRate &rCplEditRate = EditRate::EditRate24, QObject *pParent = NULL);
	virtual ~GraphicsSceneComposition() {}
	GraphicsWidgetComposition* GetComposition() const { return mpComposition; }
	GraphicsObjectVerticalIndicator* GetCurrentFrameIndicator() const { return mpCurrentFrameIndicator; }
	void DelegateCommand(QUndoCommand *pCommand) { emit PushCommand(pCommand); }
	//! Sets an edit at the position rTimecode points at concerning all resources.
	void SetEditRequest(const Timecode &rCplTimecode, QList<AbstractGraphicsWidgetResource*>resources);
	GraphicsWidgetSegment* GetSegmentAt(const Timecode &rCplTimecode) const;
	QList<AbstractGraphicsWidgetResource*> GetResourcesAt(const Timecode &rCplTimecode, SequenceTypes filter) const;

signals:
	void PushCommand(QUndoCommand *pCommand);

	public slots:
	//! Sets an edit at the current frame indicator position. If a resource is selected only this resource will be edited. Dummy and Marker resources are ignored.
	void SetEditRequest();

	private slots:
	void rCompositionGeometryChanged();

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent);
	virtual void keyPressEvent(QKeyEvent *pEvent);
	virtual void keyReleaseEvent(QKeyEvent *pEvent);
	virtual QList<AbstractGridExtension*> AddPermanentSnapItems() const;
	virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *pEvent);
	virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *pEvent);
	virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *pEvent);
	virtual void dropEvent(QGraphicsSceneDragDropEvent *pEvent);

private:
	enum eDragMode {
		OverwriteMode = 0,
		InsertMode
	};
	struct DragDropInfo {
		enum eState {
			InitDrag = 0,
			DragMove,
			Drop
		} state;
		bool isDropable;
		bool isSnapToGridMode;
		bool dropSucceeded;
		QPoint lastScenePos;
		QPoint hotspot;
		eDragMode mode;
		AbstractGraphicsWidgetResource *pOriginResource;
		GraphicsWidgetSegment *pSegment; // This pointer contains the right segment.
	};
	Q_DISABLE_COPY(GraphicsSceneComposition);
	/*! \brief Creates an edit command if needed with pRootCommand as its parent (Sometimes an edit is needless e.g.: resource bounds).
	Returns true if at least one edit was set. Otherwise returns false.
	You may check QUndoCommand::childCount() to dertermine the number of edits set.
	Does nothing if pRootCommand equals NULL.
	*/
	bool AppendEditCommand(const Timecode &rCplTimecode, QList<AbstractGraphicsWidgetResource*>resources, QUndoCommand *pRootCommand);
	bool AppendEditCommand(const Timecode &rCplTimecode, AbstractGraphicsWidgetResource *pResource, QUndoCommand *pRootCommand);
	void ProcessInitDrag(DragDropInfo &rInfo, AbstractGraphicsWidgetResource *pOriginResource, const QPoint &rScenePos, const QPoint &rHotspot);
	void ProcessDragMove(DragDropInfo &rInfo, const QPoint &rScenePos, bool snapToGrid, eDragMode mode);
	void ProcessDrop(DragDropInfo &rInfo, bool addResource = false);
	void ProcessCleanUp(DragDropInfo &rInfo);

	GraphicsWidgetComposition *mpComposition;
	AbstractGraphicsWidgetResource *mpGhost;
	GraphicsObjectVerticalIndicator *mpSnapIndicator;
	GraphicsObjectVerticalIndicator *mpInsertIndicatorTop;
	GraphicsObjectVerticalIndicator *mpInsertIndicatorBottom;
	GraphicsObjectVerticalIndicator *mpCurrentFrameIndicator;
	DragDropInfo mDropInfo;
	bool mDragActive;
};


class GraphicsSceneTimeline : public GraphicsSceneBase {

	Q_OBJECT

public:
	GraphicsSceneTimeline(const EditRate &rCplEditRate = EditRate::EditRate24, QObject *pParent = NULL);
	virtual ~GraphicsSceneTimeline() {}
	GraphicsWidgetTimeline* GetTimeline() { return mpTimeline; }
	GraphicsObjectVerticalIndicator* GetCurrentFrameIndicator() const { return mpCurrentFrameIndicator; }

signals:
	void CurrentFrameChanged(const Timecode &rNewFrame);
	void FrameInicatorActive(bool active);
	void MoveSegmentRequest(const QUuid &rSegment, const QUuid &rTargetSegment);

private slots:
	void rXPosChanged(qreal xPos) { 
		emit CurrentFrameChanged(Timecode(GetCplEditRate(), xPos)); 
		// "cursor" position changed -> emit signal for current frame to be loaded:
	}
	void rTimelineGeometryChanged();

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsSceneTimeline);

	GraphicsWidgetTimeline *mpTimeline;
	GraphicsObjectVerticalIndicator *mpCurrentFrameIndicator;
	GraphicsObjectVerticalIndicator *mpSnapIndicator;
	GraphicsWidgetSegmentIndicator *mpSegmentGhost;
	QPair<bool, QUuid> mExecuteDrop;
	bool mDragActive;
};
