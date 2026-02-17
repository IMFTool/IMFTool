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
#include "GraphicsCommon.h"


class GraphicsWidgetSegment;
class QGraphicsLinearLayout;
class GraphicsWidgetDummyResource;

class GraphicsWidgetSequence : public GraphicsWidgetBase {

	Q_OBJECT

public:
	GraphicsWidgetSequence(GraphicsWidgetSegment *pParent, eSequenceType type, const QUuid &rTrackId = QUuid::createUuid(), const QUuid &rId = QUuid::createUuid());
	virtual ~GraphicsWidgetSequence() {}
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);
	void AddResource(AbstractGraphicsWidgetResource *pResource, int ResourceIndex);
	void MoveResource(AbstractGraphicsWidgetResource *pResource, int NewResourceIndex);
	//! Doesn't delete pResource. pResource is still a child of this.
	void RemoveResource(AbstractGraphicsWidgetResource *pResource);
	AbstractGraphicsWidgetResource* GetResource(int ResourceIndex) const;
	AbstractGraphicsWidgetResource* GetResource(const QUuid &rId) const;
	QList<AbstractGraphicsWidgetResource*> GetResources() const;
	int GetResourceCount() const;
	int GetResourceIndex(AbstractGraphicsWidgetResource *pResource) const;
	bool IsEmpty() const { return !GetResourceCount(); }

	QUuid GetId() const { return mId; }
	QUuid GetTrackId() const { return mTrackId; }

	eSequenceType GetType() const { return mType; }
	Duration GetEffectiveDuration() const;
	GraphicsWidgetSegment* GetSegment() const { return qobject_cast<GraphicsWidgetSegment*>(parentObject()); }

signals:
	//! Sum of source duration of all resources.
	void EffectiveDurationChanged(GraphicsWidgetSequence *pSender, const Duration &rNewDuration);

	public slots:
	void SetHeight(qint32 height);

	private slots:
	void rResourceSourceDurationChanged(const Duration &rOldSourceDuration, const Duration &rNewSourceDuration);

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &rConstraint = QSizeF()) const;
	virtual bool ExtendGrid(QPointF &rPoint, eGridPosition which) const;
	virtual qreal HeightAdviceForHorizontalGrid() const;

private:
	Q_DISABLE_COPY(GraphicsWidgetSequence);
	void InitLayout();

	QUuid mId;
	QUuid mTrackId;
	eSequenceType mType;
	qint32 mHeight;
	QGraphicsLinearLayout *mpLayout;
	GraphicsWidgetDummyResource *mpFillerResource;
};
