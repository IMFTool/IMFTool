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
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetResources.h"
#include "GraphicsWidgetComposition.h"
#include <QStyleOptionGraphicsItem>


GraphicsWidgetSequence::GraphicsWidgetSequence(GraphicsWidgetSegment *pParent, eSequenceType type, const QUuid &rTrackId /*= QUuid::createUuid()*/, const QUuid &rId /*= QUuid::createUuid()*/) :
GraphicsWidgetBase(pParent), mId(rId), mTrackId(rTrackId), mType(type), mHeight(80), mpLayout(NULL), mpFillerResource(NULL) {

	if(mType == MarkerSequence) mHeight = 32;
	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption | QGraphicsItem::ItemStacksBehindParent); // enables pOption::exposedRect in GraphicsWidgetTimeline::paint()
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	InitLayout();
}

void GraphicsWidgetSequence::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

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
		brush.setColor(QColor(CPL_COLOR_BACKGROUND));
		pen.setColor(QColor(CPL_COLOR_BACKGROUND));
		pPainter->setPen(pen);
		pPainter->setBrush(brush);
		pPainter->drawRect(visible_rect);
		pen.setColor(QColor(CPL_BORDER_COLOR));
		pPainter->setPen(pen);
		if(exposed_rect.bottom() >= segment_rect.bottom()) pPainter->drawLine(visible_rect.bottomLeft(), visible_rect.bottomRight());
	}
}

QSizeF GraphicsWidgetSequence::sizeHint(Qt::SizeHint which, const QSizeF &rConstraint /*= QSizeF()*/) const {

	return QSizeF(-1, mHeight);
}

void GraphicsWidgetSequence::AddResource(AbstractGraphicsWidgetResource *pResource, int ResourceIndex) {

	if(ResourceIndex <= 0) {
		mpLayout->insertItem(0, pResource);
	}
	else if(ResourceIndex >= GetResourceCount()) {
		mpLayout->insertItem(GetResourceCount(), pResource);
	}
	else {
		mpLayout->insertItem(ResourceIndex, pResource);
	}
	connect(pResource, SIGNAL(SourceDurationChanged(const Duration&, const Duration&)), this, SLOT(rResourceSourceDurationChanged(const Duration&, const Duration&)));
	emit EffectiveDurationChanged(this, GetEffectiveDuration());
}

void GraphicsWidgetSequence::MoveResource(AbstractGraphicsWidgetResource *pResource, int NewResourceIndex) {

	RemoveResource(pResource);
	AddResource(pResource, NewResourceIndex);
}

void GraphicsWidgetSequence::RemoveResource(AbstractGraphicsWidgetResource *pResource) {
	for(int i = 0; i < GetResourceCount(); i++) {
		if(mpLayout->itemAt(i) == pResource) {
			mpLayout->removeAt(i);
			disconnect(pResource, NULL, this, NULL);
			emit EffectiveDurationChanged(this, GetEffectiveDuration());
			break;
		}
	}
}

AbstractGraphicsWidgetResource* GraphicsWidgetSequence::GetResource(int ResourceIndex) const {

	AbstractGraphicsWidgetResource *p_resource = NULL;
	if(ResourceIndex <= 0) {
		p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(mpLayout->itemAt(0));
	}
	else if(ResourceIndex >= GetResourceCount()) {
		p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(mpLayout->itemAt(GetResourceCount() - 1));
	}
	else {
		p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(mpLayout->itemAt(ResourceIndex));
	}
	return p_resource;
}

AbstractGraphicsWidgetResource* GraphicsWidgetSequence::GetResource(const QUuid &rId) const {

	for(int i = 0; i < GetResourceCount(); i++) {
		AbstractGraphicsWidgetResource *p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(mpLayout->itemAt(i));
		if(p_resource && p_resource->GetId() == rId) {
			return p_resource;
		}
	}
	return NULL;
}

int GraphicsWidgetSequence::GetResourceCount() const {

	return (mpLayout->count() - 1);
}

int GraphicsWidgetSequence::GetResourceIndex(AbstractGraphicsWidgetResource *pResource) const {

	for(int i = 0; i < GetResourceCount(); i++) {
		if(GetResource(i) == pResource) return i;
	}
	return -1;
}

void GraphicsWidgetSequence::InitLayout() {

	mpFillerResource = new GraphicsWidgetDummyResource(this, true);
	mpLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	mpLayout->setContentsMargins(0, 5, 0, 5);
	mpLayout->setSpacing(0);
	mpLayout->addItem(mpFillerResource);
	setLayout(mpLayout);
}

void GraphicsWidgetSequence::SetHeight(int height) {

	mHeight = height;
	updateGeometry();
}

void GraphicsWidgetSequence::rResourceSourceDurationChanged(const Duration &rOldSourceDuration, const Duration &rNewSourceDuration) {

	emit EffectiveDurationChanged(this, GetEffectiveDuration());
}

Duration GraphicsWidgetSequence::GetEffectiveDuration() const {

	Duration dur;
	for(int i = 0; i < GetResourceCount(); i++) {
		AbstractGraphicsWidgetResource *p_resource = GetResource(i);
		if(p_resource) {
			dur += p_resource->MapToCplTimeline(p_resource->GetSourceDuration()) * p_resource->GetRepeatCount();
		}
	}
	if(dur.IsNull()) return Duration(1);
	return dur;
}

QList<AbstractGraphicsWidgetResource*> GraphicsWidgetSequence::GetResources() const {

	QList<AbstractGraphicsWidgetResource*> resources;
	for(int i = 0; i < GetResourceCount(); i++) {
		resources.push_back(GetResource(i));
	}
	return resources;
}

bool GraphicsWidgetSequence::ExtendGrid(QPointF &rPoint, eGridPosition which) const {

	QPointF ret(mapFromScene(rPoint));
	switch(mType) {
		case MainImageSequence:
			if(which == VideoHorizontal) {
				ret.setY(boundingRect().center().y());
			}
			else return false;
			break;
		case MainAudioSequence:
			if(which == AudioHorizontal) {
				ret.setY(boundingRect().center().y());
			}
			else return false;
			break;
		case SubtitlesSequence:
		case KaraokeSequence:
		case CommentarySequence:
		case HearingImpairedCaptionsSequence:
		case VisuallyImpairedTextSequence: 
			if(which == TimedTextHorizontal) {
				ret.setY(boundingRect().center().y());
			}
			else return false;
			break;
		case AncillaryDataSequence:
			if(which == DataHorizontal) {
				ret.setY(boundingRect().center().y());
			}
			else return false;
			break;
		case MarkerSequence:
			if(which == MarkerHorizontal) {
				ret.setY(boundingRect().center().y());
			}
			else return false;
			break;
		case Unknown:
		default:
			return false;
			break;
	}
	rPoint = mapToScene(ret);
	return true;
}

qreal GraphicsWidgetSequence::HeightAdviceForHorizontalGrid() const {

	return mpLayout->contentsRect().size().height();
}
