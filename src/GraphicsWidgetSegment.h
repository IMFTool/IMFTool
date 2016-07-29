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
#include "GraphicsWidgetTimeline.h"
#include "ImfPackageCommon.h"
#include "GraphicsCommon.h"
#include <QGraphicsWidget>
#include <QGraphicsItem>
#include <QList>


class GraphicsWidgetSequence;
class GraphicsWidgetDummyResource;
class GraphicsWidgetComposition;

class GraphicsWidgetSegment : public GraphicsWidgetBase {

	Q_OBJECT

public:
	GraphicsWidgetSegment(GraphicsWidgetComposition *pParent, const QColor &rColor, const QUuid &rId = QUuid::createUuid(), const UserText &rAnnotationText = QString());
	virtual ~GraphicsWidgetSegment() {}
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	void AddSequence(GraphicsWidgetSequence *pSequence, int SequenceIndex);
	void MoveSequence(GraphicsWidgetSequence *pSequence, int NewSequenceIndex);
	//! Doesn't delete pSequence. pSequence is still a child of this.
	void RemoveSequence(GraphicsWidgetSequence *pSequence);
	GraphicsWidgetSequence* GetSequence(int SequenceIndex) const;
	GraphicsWidgetSequence* GetSequence(const QUuid &rId) const;
	QList<GraphicsWidgetSequence*> GetSequences() const;
	GraphicsWidgetSequence* GetSequenceWithTrackId(const QUuid &rTrackId) const;
	int GetSequenceCount() const;
	int GetSequenceIndex(GraphicsWidgetSequence *pSequence) const;
	bool IsEmpty() const { return !GetSequenceCount(); }
	void SetDuration(const Duration &rDuration);

	QUuid GetId() const { return mId; }
	UserText GetAnnotationText() const { return mAnnotationText; }
	Duration GetDuration() const { return mDuration; }
	void SetAnnotationText(const UserText &rAnnotationText) { mAnnotationText = rAnnotationText; }
	void Highlight(bool active) { setFlag(QGraphicsItem::ItemHasNoContents, !active); }
	bool IsHighlighted() const { return !(flags() & QGraphicsItem::ItemHasNoContents); }
	GraphicsWidgetComposition* GetComposition() const { return qobject_cast<GraphicsWidgetComposition*>(parentObject()); }

signals:
	void DurationChanged(const Duration &rDuration);

	private slots:
	void rSequenceEffectiveDurationChanged(GraphicsWidgetSequence *pSender, const Duration &rNewDuration);
	void rSegmentIndicatorHoverActive(bool active);

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &rConstraint = QSizeF()) const;

private:
	Q_DISABLE_COPY(GraphicsWidgetSegment);
	void InitLayout();

	QUuid mId;
	UserText mAnnotationText;
	Duration mDuration;
	QColor mColor;
	QGraphicsLinearLayout *mpLayout;
};


class GraphicsWidgetSegmentIndicator : public GraphicsWidgetBase {

	Q_OBJECT

public:
	GraphicsWidgetSegmentIndicator(GraphicsWidgetTimeline *pParent, const QColor &rColor, const QUuid &rId);
	virtual ~GraphicsWidgetSegmentIndicator() {}
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	QUuid GetId() const { return mId; }
	GraphicsWidgetSegmentIndicator* Clone() const;

signals:
	void HoverActive(bool active);

	private slots:
	void rSegmentDurationChange(const Duration &rNewDuration);

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &rConstraint = QSizeF()) const;
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent);
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const;
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &rValue);

private:
	QUuid mId;
	Duration mDuration;
	QColor mColor;
};
