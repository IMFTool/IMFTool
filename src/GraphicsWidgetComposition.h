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

class GraphicsWidgetComposition : public GraphicsWidgetBase {

	Q_OBJECT

public:
	GraphicsWidgetComposition(QGraphicsItem *pParent = NULL);
	virtual ~GraphicsWidgetComposition() {}
	virtual int type() const { return GraphicsWidgetCompositionType; }
	virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);

	//! Takes ownership.
	void AddSegment(GraphicsWidgetSegment *pSegment, int SegmentIndex);
	void MoveSegment(GraphicsWidgetSegment *pSegment, int NewSegmentIndex);
	//! Doesn't delete pSegment. pSegment is still a child of this.
	void RemoveSegment(GraphicsWidgetSegment *pSegment);
	GraphicsWidgetSegment* GetSegment(int SegmentIndex) const;
	GraphicsWidgetSegment* GetSegment(const QUuid &rId) const;
	int GetSegmentCount() const;
	int GetSegmentIndex(GraphicsWidgetSegment *pSegment) const;
	bool IsEmpty() const { return !GetSegmentCount(); }

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
	virtual void resizeEvent(QGraphicsSceneResizeEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsWidgetComposition);
	void InitLayout();

	QGraphicsLinearLayout *mpLayout;
};
