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
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetResources.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicScenes.h"
#include "CompositionPlaylistCommands.h"
#include <QPen>
#include <QMenu>
#include <QPropertyAnimation>
#include <QPainter>
#include <QStyleOption>


GraphicsWidgetSegment::GraphicsWidgetSegment(GraphicsWidgetComposition *pParent, const QColor &rColor, const QUuid &rId /*= QUuid::createUuid()*/, const UserText &rAnnotationText /*= QString()*/) :
GraphicsWidgetBase(pParent), mId(rId), mAnnotationText(rAnnotationText), mDuration(1), mColor(rColor), mpLayout(NULL), mpWidgetComposition(pParent) {

	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption | QGraphicsItem::ItemHasNoContents); // enables pOption::exposedRect in GraphicsWidgetTimeline::paint()
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	InitLayout();
}

QSizeF GraphicsWidgetSegment::sizeHint(Qt::SizeHint which, const QSizeF &rConstraint /*= QSizeF()*/) const {

	return QSizeF(mDuration.GetCount(), QGraphicsWidget::sizeHint(which, rConstraint).height());
}

void GraphicsWidgetSegment::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

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
		pen.setColor(mColor);
		brush.setColor(mColor);
		pPainter->setPen(pen);
		pPainter->setBrush(brush);
		pPainter->drawRect(visible_rect);
	}
}

void GraphicsWidgetSegment::AddSequence(GraphicsWidgetSequence *pSequence, int SequenceIndex) {

	if(SequenceIndex <= 0) {
		mpLayout->insertItem(0, pSequence);
	}
	else if(SequenceIndex >= mpLayout->count()) {
		mpLayout->insertItem(mpLayout->count(), pSequence);
	}
	else {
		mpLayout->insertItem(SequenceIndex, pSequence);
	}
	connect(pSequence, SIGNAL(EffectiveDurationChanged(GraphicsWidgetSequence*, const Duration&)), this, SLOT(rSequenceEffectiveDurationChanged(GraphicsWidgetSequence*, const Duration&)));
	rSequenceEffectiveDurationChanged(pSequence, pSequence->GetEffectiveDuration());
}

void GraphicsWidgetSegment::MoveSequence(GraphicsWidgetSequence *pSequence, int NewSequenceIndex) {

	RemoveSequence(pSequence);
	AddSequence(pSequence, NewSequenceIndex);
}

void GraphicsWidgetSegment::RemoveSequence(GraphicsWidgetSequence *pSequence) {

	for(int i = 0; i < mpLayout->count(); i++) {
		if(mpLayout->itemAt(i) == pSequence) {
			mpLayout->removeAt(i);
			disconnect(pSequence, NULL, this, NULL);
			rSequenceEffectiveDurationChanged(pSequence, pSequence->GetEffectiveDuration());
			break;
		}
	}
}

GraphicsWidgetSequence* GraphicsWidgetSegment::GetSequence(int SequenceIndex) const {

	GraphicsWidgetSequence *p_sequence = NULL;
	if(SequenceIndex <= 0) {
		p_sequence = dynamic_cast<GraphicsWidgetSequence*>(mpLayout->itemAt(0));
	}
	else if(SequenceIndex >= mpLayout->count()) {
		p_sequence = dynamic_cast<GraphicsWidgetSequence*>(mpLayout->itemAt(mpLayout->count() - 1));
	}
	else {
		p_sequence = dynamic_cast<GraphicsWidgetSequence*>(mpLayout->itemAt(SequenceIndex));
	}
	return p_sequence;
}

GraphicsWidgetSequence* GraphicsWidgetSegment::GetSequence(const QUuid &rId) const {

	for(int i = 0; i < mpLayout->count(); i++) {
		GraphicsWidgetSequence *p_sequence = dynamic_cast<GraphicsWidgetSequence*>(mpLayout->itemAt(i));
		if(p_sequence && p_sequence->GetId() == rId) {
			return p_sequence;
		}
	}
	return NULL;
}

int GraphicsWidgetSegment::GetSequenceCount() const {

	return mpLayout->count();
}

GraphicsWidgetSequence* GraphicsWidgetSegment::GetSequenceWithTrackId(const QUuid &rTrackId) const {

	for(int i = 0; i < mpLayout->count(); i++) {
		GraphicsWidgetSequence *p_sequence = dynamic_cast<GraphicsWidgetSequence*>(mpLayout->itemAt(i));
		if(p_sequence && p_sequence->GetTrackId() == rTrackId) {
			return p_sequence;
		}
	}
	return NULL;
}

void GraphicsWidgetSegment::InitLayout() {

	mpLayout = new QGraphicsLinearLayout(Qt::Vertical);
	mpLayout->setContentsMargins(0, 0, 0, 0);
	mpLayout->setSpacing(0);
	setLayout(mpLayout);
}

void GraphicsWidgetSegment::rSequenceEffectiveDurationChanged(GraphicsWidgetSequence *pSender, const Duration &rNewDuration) {

	Duration max = 0;
	int marker_sequence_index = -1;
	for(int i = 0; i < GetSequenceCount(); i++) {
		if (GetSequence(i)->GetType() != MarkerSequence ) {
			Duration dur = GetSequence(i)->GetEffectiveDuration();
			if(dur > max) max = dur;
		} else {
			marker_sequence_index = i;
		}
	}
	if(max < 1) max = 1;
	if(max != mDuration) {
		mDuration = max;
		if (marker_sequence_index >= 0 ) {
			GraphicsWidgetSequence* marker_seq = GetSequence(marker_sequence_index);
			int marker_seq_duration = 0;
			int i = 0;
			while ( (marker_seq_duration <= max.GetCount()) && (i < marker_seq->GetResourceCount())) {
				marker_seq_duration += marker_seq->GetResource(i)->GetSourceDuration().GetCount();
				i++;
			}
			for (int ii = marker_seq->GetResourceCount(); ii > i; ii--) { // delete all following marker resources, if existing
				AbstractGraphicsWidgetResource* res = marker_seq->GetResource(ii);
				if (res && !mpWidgetComposition->GetParseCplInProgress()) {
					marker_seq->RemoveResource(res);
					res->hide();
					res->setParentItem(NULL);
				}

			}
			i--;
			GraphicsWidgetMarkerResource* marker_res = dynamic_cast<GraphicsWidgetMarkerResource*>(marker_seq->GetResource(i));
			if (marker_res  && !mpWidgetComposition->GetParseCplInProgress()) {
				Duration old_source_duration = marker_res->GetSourceDuration();
				Duration new_source_duration = Duration(marker_res->GetSourceDuration().GetCount() + (max.GetCount() - marker_seq_duration));
				if(old_source_duration > new_source_duration) {
					marker_res->SetSourceDuration(new_source_duration);
					marker_res->SetIntrinsicDuaration(Duration(marker_res->GetEntryPoint() + new_source_duration));
				}
				else {
					marker_res->SetIntrinsicDuaration(Duration(marker_res->GetEntryPoint() + new_source_duration));
					marker_res->SetSourceDuration(new_source_duration);
				}
				if (mpWidgetComposition && !mpWidgetComposition->GetParseCplInProgress())
					marker_res->RemoveIrrelevantMarkers();
			}
		}
		emit DurationChanged(mDuration);
		updateGeometry();
	}
}

QList<GraphicsWidgetSequence*> GraphicsWidgetSegment::GetSequences() const {

	QList<GraphicsWidgetSequence*> sequences;
	for(int i = 0; i < GetSequenceCount(); i++) {
		sequences.push_back(GetSequence(i));
	}
	return sequences;
}

void GraphicsWidgetSegment::rSegmentIndicatorHoverActive(bool active) {

	Highlight(active);
}

int GraphicsWidgetSegment::GetSequenceIndex(GraphicsWidgetSequence *pSequence) const {

	for(int i = 0; i < GetSequenceCount(); i++) {
		if(GetSequence(i) == pSequence) return i;
	}
	return -1;
}

void GraphicsWidgetSegment::SetDuration(const Duration &rDuration) {

	mDuration = rDuration;
	emit DurationChanged(mDuration);
	updateGeometry();
}

GraphicsWidgetSegmentIndicator::GraphicsWidgetSegmentIndicator(GraphicsWidgetTimeline *pParent, const QColor &rColor, const QUuid &rId) :
GraphicsWidgetBase(pParent), mId(rId), mDuration(1), mColor(rColor) {

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption | QGraphicsItem::ItemSendsGeometryChanges |QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
}

QSizeF GraphicsWidgetSegmentIndicator::sizeHint(Qt::SizeHint which, const QSizeF &rConstraint /*= QSizeF()*/) const {

	QSizeF size;
	qint64 duration_to_width = mDuration.GetCount();
	if(rConstraint.isValid() == false) {
		switch(which) {
			case Qt::MinimumSize:
				size = QSizeF(duration_to_width, GraphicsHelper::GetDefaultFontHeight());
				break;
			case Qt::PreferredSize:
				size = QSizeF(duration_to_width, GraphicsHelper::GetDefaultFontHeight());
				break;
			case Qt::MaximumSize:
				size = QSizeF(duration_to_width, std::numeric_limits<qint32>::max());
				break;
			case Qt::MinimumDescent:
				size = QSizeF(-1, -1);
				break;
			case Qt::NSizeHints:
				size = QSizeF(-1, -1);
				break;
			default:
				size = QSizeF(-1, -1);
				break;
		}
	}
	else {
		qWarning() << "sizeHint() is constraint.";
		size = rConstraint;
	}
	return size;
}

void GraphicsWidgetSegmentIndicator::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF segment_rect(boundingRect());
	QPen pen;
	pen.setWidth(0); // cosmetic
	QBrush brush(Qt::SolidPattern);
	QColor color(mColor);
	QColor light_color(mColor.lighter(220));
	QColor lighter_color(mColor.lighter(187));
	QColor light_color_border(mColor.lighter(141));
	QColor dark_color_border(mColor.darker(118));

	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(segment_rect.intersected(exposed_rect));
	if(visible_rect.isEmpty() == false) {
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());

		if(acceptHoverEvents() == true && pOption->state & QStyle::State_MouseOver) {
			if(pOption->state & QStyle::State_Selected) {
				pen.setColor(lighter_color);
				brush.setColor(lighter_color);
			}
			else {
				pen.setColor(light_color);
				brush.setColor(light_color);
			}
			pPainter->setPen(pen);
			pPainter->setBrush(brush);
			pPainter->drawRect(visible_rect);
		}
		else {
			if(pOption->state & QStyle::State_Selected) {
				pen.setColor(lighter_color);
				brush.setColor(lighter_color);
			}
			else {
				pen.setColor(mColor);
				brush.setColor(pen.color());
			}
			pPainter->setPen(pen);
			pPainter->setBrush(brush);
			pPainter->drawRect(visible_rect);
		}

		if(pOption->state & QStyle::State_Selected) pen.setColor(dark_color_border);
		else pen.setColor(light_color_border);
		pPainter->setPen(pen);
		if(exposed_rect.top() <= segment_rect.top()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.topRight());
		if(exposed_rect.left() <= segment_rect.left()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.bottomLeft());
		if(pOption->state & QStyle::State_Selected) pen.setColor(light_color_border);
		else pen.setColor(dark_color_border);
		pPainter->setPen(pen);
		if(exposed_rect.bottom() >= segment_rect.bottom()) pPainter->drawLine(visible_rect.bottomLeft(), visible_rect.bottomRight());
		if(exposed_rect.right() >= segment_rect.right()) pPainter->drawLine(visible_rect.topRight(), visible_rect.bottomRight());

		QTransform transf = pPainter->transform();

		QFontMetricsF font_metrics(pPainter->font());
		QString segment_id(QString("Segment: %1").arg(strip_uuid(GetId())));
		QRectF writable_rect(segment_rect);

		if(writable_rect.intersected(visible_rect).isEmpty() == false && acceptHoverEvents() == true && pOption->state & QStyle::State_MouseOver) {
			pen.setColor(QColor(CPL_FONT_COLOR));
			pPainter->setPen(pen);
			segment_id = font_metrics.elidedText(segment_id, Qt::ElideRight, writable_rect.width() * transf.m11());
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.center().rx() * transf.m11() - font_metrics.boundingRect(segment_id).width() / 2, writable_rect.top() + font_metrics.height())); // We have to use QTransform::translate() because of bug 192573.
			pPainter->drawText(QPointF(0, 0), segment_id);
		}
	}
}

void GraphicsWidgetSegmentIndicator::rSegmentDurationChange(const Duration &rNewDuration) {

	mDuration = rNewDuration;
	updateGeometry();
}

void GraphicsWidgetSegmentIndicator::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	if(isSelected() == false) emit HoverActive(false);
}

void GraphicsWidgetSegmentIndicator::hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	emit HoverActive(true);
}

GraphicsWidgetSegmentIndicator* GraphicsWidgetSegmentIndicator::Clone() const {

	GraphicsWidgetSegmentIndicator* ret = new GraphicsWidgetSegmentIndicator(NULL, mColor, mId);
	ret->rSegmentDurationChange(mDuration);
	ret->resize(size());
	return ret;
}

bool GraphicsWidgetSegmentIndicator::ExtendGrid(QPointF &rPoint, eGridPosition which) const {

		if(which == Vertical) {
			QPointF ret(mapFromScene(rPoint));
			ret.setX(boundingRect().left());
			rPoint = mapToScene(ret);
			return true;
		}
		return false;
}

QVariant GraphicsWidgetSegmentIndicator::itemChange(GraphicsItemChange change, const QVariant &rValue) {

	if(change == QGraphicsItem::ItemSelectedHasChanged) {
		emit HoverActive(rValue.toBool());
	}
	return GraphicsWidgetBase::itemChange(change, rValue);
}
