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
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetSegment.h"
#include <QGraphicsLinearLayout>
#include <QStyleOptionGraphicsItem>


GraphicsWidgetComposition::GraphicsWidgetComposition(QGraphicsItem *pParent /*= NULL*/) :
GraphicsWidgetBase(NULL), mpLayout(NULL), mParseCplInProgress(false) {

	setFlag(QGraphicsItem::ItemHasNoContents);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	InitLayout();
}

void GraphicsWidgetComposition::InitLayout() {

	mpLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	mpLayout->setContentsMargins(0, 0, 0, 0);
	mpLayout->setSpacing(0);
	setLayout(mpLayout);
}

void GraphicsWidgetComposition::AddSegment(GraphicsWidgetSegment *pSegment, int SegmentIndex) {

	if(SegmentIndex <= 0) {
		mpLayout->insertItem(0, pSegment);
	}
	else if(SegmentIndex >= mpLayout->count()) {
		mpLayout->insertItem(mpLayout->count(), pSegment);
	}
	else {
		mpLayout->insertItem(SegmentIndex, pSegment);
	}
}

void GraphicsWidgetComposition::MoveSegment(GraphicsWidgetSegment *pSegment, int NewSegmentIndex) {

	RemoveSegment(pSegment);
	AddSegment(pSegment, NewSegmentIndex);
}

void GraphicsWidgetComposition::RemoveSegment(GraphicsWidgetSegment *pSegment) {

	for(int i = 0; i < mpLayout->count(); i++) {
		if(mpLayout->itemAt(i) == pSegment) {
			mpLayout->removeAt(i);
			break;
		}
	}
}

GraphicsWidgetSegment* GraphicsWidgetComposition::GetSegment(int SegmentIndex) const {

	GraphicsWidgetSegment *p_seqment = NULL;
	if(SegmentIndex <= 0) {
		p_seqment = dynamic_cast<GraphicsWidgetSegment*>(mpLayout->itemAt(0));
	}
	else if(SegmentIndex >= mpLayout->count()) {
		p_seqment = dynamic_cast<GraphicsWidgetSegment*>(mpLayout->itemAt(mpLayout->count() - 1));
	}
	else {
		p_seqment = dynamic_cast<GraphicsWidgetSegment*>(mpLayout->itemAt(SegmentIndex));
	}
	return p_seqment;
}

GraphicsWidgetSegment* GraphicsWidgetComposition::GetSegment(const QUuid &rId) const {

	for(int i = 0; i < mpLayout->count(); i++) {
		GraphicsWidgetSegment *p_seqment = dynamic_cast<GraphicsWidgetSegment*>(mpLayout->itemAt(i));
		if(p_seqment && p_seqment->GetId() == rId) {
			return p_seqment;
		}
	}
	return NULL;
}

int GraphicsWidgetComposition::GetSegmentCount() const {

	return mpLayout->count();
}

int GraphicsWidgetComposition::GetSegmentIndex(GraphicsWidgetSegment *pSegment) const {

	for(int i = 0; i < GetSegmentCount(); i++) {
		if(GetSegment(i) == pSegment) return i;
	}
	return -1;
}

void GraphicsWidgetComposition::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF segment_rect(boundingRect());
	QPen pen;
	pen.setWidth(0); // cosmetic
	QBrush brush(Qt::SolidPattern);
	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(segment_rect.intersected(exposed_rect));
	visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());
	if(visible_rect.isEmpty() == false) {
		QColor color(Qt::red);
		color.setAlpha(50);
		brush.setColor(color);
		pen.setColor(color);
		pPainter->setPen(pen);
		pPainter->setBrush(brush);
		pPainter->drawRect(visible_rect);
	}
}

QSizeF GraphicsWidgetComposition::sizeHint(Qt::SizeHint which, const QSizeF &constraint /*= QSizeF()*/) const {

	return QGraphicsWidget::sizeHint(which, constraint);
}

void GraphicsWidgetComposition::resizeEvent(QGraphicsSceneResizeEvent *pEvent) {

	QGraphicsScene *p_scene = scene();
	if(p_scene) {
		QRectF scene_rect = boundingRect();
		scene()->setSceneRect(scene_rect);
	}
}
