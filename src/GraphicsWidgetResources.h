/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel
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
#include "GraphicsCommon.h"
#include "ImfPackageCommon.h"
#include "ImfPackage.h"
#include "GraphicsViewScaleable.h"


class GraphicsWidgetSequence;

class AbstractGraphicsWidgetResource : public GraphicsWidgetBase {

	Q_OBJECT

public:
	enum eTrimHandlePosition {
		Left = 0,
		Right
	};

private:
	class TrimHandle : public QGraphicsItem, public AbstractViewTransformNotifier {

	public:

		TrimHandle(AbstractGraphicsWidgetResource *pParent, eTrimHandlePosition pos);
		virtual ~TrimHandle() {}
		eTrimHandlePosition GetDirection() const { return mPos; }
		void SetDirection(eTrimHandlePosition pos);
		void SetHeight(qreal height) { prepareGeometryChange(); mRect.setHeight(height); }
		void SetWidth(qreal width);
		virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *widget = 0);
		virtual QRectF boundingRect() const;

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		virtual void ViewTransformEvent(const QTransform &rTransform);
		virtual QGraphicsView* GetObservableView() const;
		virtual QVariant itemChange(GraphicsItemChange change, const QVariant &rValue);

	private:
		Q_DISABLE_COPY(TrimHandle);

		eTrimHandlePosition mPos;
		int				mMouseXOffset;
		bool			mMousePressed;
		QRectF		mRect;
	};

	class GraphicsItemDurationIndicator : public QGraphicsItem {

	public:
		GraphicsItemDurationIndicator(AbstractGraphicsWidgetResource *pParent);
		virtual ~GraphicsItemDurationIndicator() {}
		virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = 0);
		virtual QRectF boundingRect() const;
		void SetRect(const QRectF &rRect);

	private:
		Q_DISABLE_COPY(GraphicsItemDurationIndicator);
		QRectF mRect;
	};

public:
	//! Import existing Resource. pResource is owned by this.
	AbstractGraphicsWidgetResource(GraphicsWidgetSequence *pParent, cpl::BaseResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset = QSharedPointer<AssetMxfTrack>(NULL), const QColor &rColor = QColor(Qt::white));
	virtual ~AbstractGraphicsWidgetResource() { delete mpData; }
	QUuid GetId() const;
	UserText GetAnnotation() const;
	//! Equals AssetMxfTrack::GetId()
	EditRate GetEditRate() const;
	Duration GetIntrinsicDuration() const;
	Duration GetSourceDuration() const;
	Duration GetEntryPoint() const;
	int GetRepeatCount() const;
	void SetAnnotation(const UserText &rText) { mpData->setAnnotation(ImfXmlHelper::Convert(rText)); }
	void SetEntryPoint(const Duration &rEntryPoint);
	void SetSourceDuration(const Duration &rSourceDuration);
	void SetID (QUuid id);
	virtual std::auto_ptr<cpl::BaseResourceType> Write() const = 0;
	GraphicsWidgetSequence* GetSequence() const;
	QColor GetColor() const { return mColor; }
	//! Should return a copy of this but with different Ids and no parent assigned. The object is not owned by this.
	virtual AbstractGraphicsWidgetResource* Clone() const = 0;
	//! If the resource has no asset the cpl will be incomplete.
	bool HasAsset() const { return mAssset.data(); }
	QSharedPointer<AssetMxfTrack> GetAsset() const { return mAssset; }
	void SetColor(const QColor &rColor) { mColor = rColor; update(); }
	void EnableTrimHandle(eTrimHandlePosition pos, bool enable);
	void DisableTrimHandle(eTrimHandlePosition pos, bool disable);
	void EnableResourceDrag(bool enable);
	void DisableResourceDrag(bool disable);
	void MaximizeZValue();
	void RestoreZValue();

	//! Transforms a local Timecode to an absolute Cpl timecode. 
	Timecode MapToCplTimeline(const Timecode &rLocalTimecode) const;
	//! Transforms a local Duration to Cpl Duration (replaces Resource Edit Rate with Cpl Edit Rate).
	Duration MapToCplTimeline(const Duration &rLocalDuration) const;
	//! Transforms a Cpl Timecode to an absolute local timecode. 
	Timecode MapFromCplTimeline(const Timecode &rCplTimecode) const;
	//! Transforms a Cpl Duration to local Duration (replaces Cpl Edit Rate with Resource Edit Rate).
	Duration MapFromCplTimeline(const Duration &rCplDuration) const;

	//! Returns the local Timecode of the first visible frame.
	Timecode GetFirstVisibleFrame() const { return Timecode(GetEditRate(), GetEntryPoint()); }
	//! Returns the local Timecode of the last visible frame.
	Timecode GetLastVisibleFrame() const { return Timecode(GetEditRate(), GetEntryPoint() + GetSourceDuration() - 1); }

	//! Paints a rectangle representing the resource source duration with the color set by AbstractGraphicsWidgetResource::SetColor().
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);

signals:
	//! Is emitted immediately after the new duration was set but before the geometric shape changed.
	void SourceDurationChanged(const Duration &rOldSourceDuration, const Duration &rNewSourceDuration);
	void EntryPointChanged(const Duration &rOldEntryPoint, const Duration &rNewEntryPoint);

	private slots:
	void rAssetModified();

protected:
	virtual void resizeEvent(QGraphicsSceneResizeEvent *pEvent);
	virtual void hideEvent(QHideEvent *pEvent);
	//! Implement this to tack TrimHandle use. Is activated if TrimHandle is clicked or released. If you reimplement this method call base implementation first.
	virtual void TrimHandleInUse(eTrimHandlePosition pos, bool active);
	//! Implement this to tack TrimHandle movement. Default implementation does nothing.
	virtual void TrimHandleMoved(eTrimHandlePosition pos) {}
	//! DONT'T reimplement this.
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &rConstraint = QSizeF()) const;
	//! If != 1 reimplement this. For audio: sampling rate / composition edit rate. MUSTN'T be 0.
	virtual double ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const { return 1; }
	//! Check if we have to hide the trim handles.
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);
	//! Check if we have to show the trim handles.
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent);
	//! Extends snap grid
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const;
	//! Reacts on Cpl edit rate changes. Just calls QGraphicsWidget::updateGeometry().
	virtual void CplEditRateChanged();

	cpl::BaseResourceType* mpData;
	QSharedPointer<AssetMxfTrack> mAssset;

private:
	Q_DISABLE_COPY(AbstractGraphicsWidgetResource);
	//! pos and last pos are given in scene coordinates.
	void TrimResource(qint64 pos, qint64 lastPos, eTrimHandlePosition epos);

	QColor mColor;
	Duration mOldEntryPoint; // Backup: Is set if Trim Handle is clicked.
	Duration mOldSourceDuration; // Backup: Is set if Trim Handle is clicked.
	TrimHandle *mpLeftTrimHandle;
	TrimHandle *mpRightTrimHandle;
	GraphicsItemDurationIndicator *mpDurationIndicator;
	GraphicsObjectVerticalIndicator *mpVerticalIndicator;
};


class GraphicsWidgetFileResource : public AbstractGraphicsWidgetResource {

public:
	//! Import existing Resource. pResource is owned by this.
	GraphicsWidgetFileResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset = QSharedPointer<AssetMxfTrack>(NULL), const QColor &rColor = QColor(Qt::white));
	//! Creates new Resource.
	GraphicsWidgetFileResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset, const QColor &rColor = QColor(Qt::white));
	virtual ~GraphicsWidgetFileResource() {}
	virtual int type() const { return GraphicsWidgetFileResourceType; }
	QUuid GetTrackFileId() const { return ImfXmlHelper::Convert(static_cast<cpl::TrackFileResourceType*>(mpData)->getTrackFileId()); }
	virtual GraphicsWidgetFileResource* Clone() const;
	virtual std::auto_ptr<cpl::BaseResourceType> Write() const;

private:
	Q_DISABLE_COPY(GraphicsWidgetFileResource);
};


class GraphicsWidgetDummyResource : public GraphicsWidgetFileResource {

public:
	GraphicsWidgetDummyResource(GraphicsWidgetSequence *pParent, bool stretch = false);
	virtual ~GraphicsWidgetDummyResource() {}
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	virtual int type() const { return GraphicsWidgetDummyResourceType; }

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &rConstraint = QSizeF()) const;
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const { return false; }

private:
	Q_DISABLE_COPY(GraphicsWidgetDummyResource);

	bool mStretch;
};


class GraphicsWidgetVideoResource : public GraphicsWidgetFileResource {

	Q_OBJECT

public:
	//! Import existing Resource. pResource is owned by this.
	GraphicsWidgetVideoResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset = QSharedPointer<AssetMxfTrack>(NULL));
	//! Creates new Resource.
	GraphicsWidgetVideoResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset);
	virtual ~GraphicsWidgetVideoResource() {}
	virtual int type() const { return GraphicsWidgetVideoResourceType; }
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	virtual GraphicsWidgetVideoResource* Clone() const;
	void RefreshProxy();

	private slots:
	void rShowProxyImage(const QImage &rImage, const QVariant &rIdentifier = QVariant());
	void rSourceDurationChanged();
	void rEntryPointChanged();

protected:
	virtual double ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const;
	virtual void TrimHandleInUse(eTrimHandlePosition pos, bool active);
	virtual void CplEditRateChanged() { RefreshProxy(); } // TODO: Better Proxy calculation Trigger for Drop.

private:
	Q_DISABLE_COPY(GraphicsWidgetVideoResource);
	void RefreshFirstProxy();
	void RefreshSecondProxy();

	QImage mLeftProxyImage;
	QImage mRightProxyImage;
	bool mTrimActive;
};


class GraphicsWidgetAudioResource : public GraphicsWidgetFileResource {

public:
	//! Import existing Resource. pResource is owned by this.
	GraphicsWidgetAudioResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset = QSharedPointer<AssetMxfTrack>(NULL));
	//! Creates new Resource.
	GraphicsWidgetAudioResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset);
	virtual ~GraphicsWidgetAudioResource() {}
	virtual int type() const { return GraphicsWidgetAudioResourceType; }
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	virtual GraphicsWidgetAudioResource* Clone() const;
	SoundfieldGroup GetSoundfieldGroup() const;

protected:
	virtual double ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const;

private:
	Q_DISABLE_COPY(GraphicsWidgetAudioResource);
};


class GraphicsWidgetTimedTextResource : public GraphicsWidgetFileResource {

public:
	//! Import existing Resource. pResource is owned by this.
	GraphicsWidgetTimedTextResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset = QSharedPointer<AssetMxfTrack>(NULL));
	//! Creates new Resource.
	GraphicsWidgetTimedTextResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset);
	virtual ~GraphicsWidgetTimedTextResource() {}
	virtual int type() const { return GraphicsWidgetTimedTextResourceType; }
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	virtual GraphicsWidgetTimedTextResource* Clone() const;

protected:
	virtual double ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const;

private:
	Q_DISABLE_COPY(GraphicsWidgetTimedTextResource);
};


class GraphicsWidgetAncillaryDataResource : public GraphicsWidgetFileResource {

public:
	//! Import existing Resource. pResource is owned by this.
	GraphicsWidgetAncillaryDataResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset = QSharedPointer<AssetMxfTrack>(NULL));
	//! Creates new Resource.
	GraphicsWidgetAncillaryDataResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset);
	virtual ~GraphicsWidgetAncillaryDataResource() {}
	virtual int type() const { return GraphicsWidgetAncillaryDataResourceType; }
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	virtual GraphicsWidgetAncillaryDataResource* Clone() const;

protected:
	virtual double ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const;

private:
	Q_DISABLE_COPY(GraphicsWidgetAncillaryDataResource);
};


class GraphicsWidgetMarkerResource : public AbstractGraphicsWidgetResource {

	Q_OBJECT

private:
	class GraphicsWidgetMarker : public GraphicsObjectVerticalIndicator {

	public:
		GraphicsWidgetMarker(GraphicsWidgetMarkerResource *pParent, qreal width, qreal height, const MarkerLabel &rLabel, const QColor &rColor);
		virtual ~GraphicsWidgetMarker() {}
		virtual int type() const { return GraphicsWidgetMarkerType; }
		UserText GetAnnotation() const { return mAnnotation; }
		MarkerLabel GetMarkerLabel() const { return mLabel; }
		void SetLabel(const MarkerLabel &rLabel) { mLabel = rLabel; }
		void SetAnnotation(const UserText &rAnnotation) { mAnnotation = rAnnotation; }

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *pEvent);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent);
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);

	private:
		Q_DISABLE_COPY(GraphicsWidgetMarker);

		UserText mAnnotation;
		MarkerLabel mLabel;
	};

public:
	//! Import existing Resource. pResource is owned by this.
	GraphicsWidgetMarkerResource(GraphicsWidgetSequence *pParent, cpl::MarkerResourceType *pResource);
	//! Creates new Resource.
	GraphicsWidgetMarkerResource(GraphicsWidgetSequence *pParent);
	virtual ~GraphicsWidgetMarkerResource() {}
	virtual int type() const { return GraphicsWidgetMarkerResourceType; }
	virtual GraphicsWidgetMarkerResource* Clone() const;
	virtual std::auto_ptr<cpl::BaseResourceType> Write() const;
	void SetIntrinsicDuaration(const Duration &rIntrinsicDuration);

protected:
	virtual double ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const;
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent);
	virtual void CplEditRateChanged();
	virtual void resizeEvent(QGraphicsSceneResizeEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsWidgetMarkerResource);
	void MoveMarker(GraphicsWidgetMarker *pMarker, qint64 pos, qint64 lastPos);
	void MarkerInUse(GraphicsWidgetMarker *pMarker, bool active);
	void InitMarker();

	QPointF mActiveMarkerOldPosition;
	Duration mOldSourceDuration;
	Duration mOldIntrinsicDuration;
};
