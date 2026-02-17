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
#include "GraphicsWidgetTimeline.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetComposition.h"
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QPushButton>
#include <QButtonGroup>
#include <QMenu>


GraphicsWidgetTimeline::GraphicsWidgetTimeline(QGraphicsItem *pParent /*= NULL*/) :
GraphicsWidgetBase(pParent), mpTimelineLayout(NULL), mpSegmentLayout(NULL), mpTimeline(NULL), mpButtonGroup(NULL), mHeight(60) {

	setFlags(QGraphicsItem::ItemHasNoContents);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	InitLayout();
}

QSizeF GraphicsWidgetTimeline::sizeHint(Qt::SizeHint which, const QSizeF &constraint /*= QSizeF()*/) const {

	QSizeF size;
	if(constraint.isValid() == false) {
		switch(which) {
			case Qt::MinimumSize:
				size = QSizeF(std::numeric_limits<qint32>::max(), mHeight);
				break;
			case Qt::PreferredSize:
				size = QSizeF(std::numeric_limits<qint32>::max(), mHeight);
				break;
			case Qt::MaximumSize:
				size = QSizeF(std::numeric_limits<qint32>::max(), mHeight);
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
		size = constraint;
	}
	return size;
}

void GraphicsWidgetTimeline::SetHeight(qint32 height) {

	mHeight = height;
	updateGeometry();
}

void GraphicsWidgetTimeline::InitLayout() {

	mpTimeline = new GraphicsWidgetDrawnTimeline(this);
	mpButtonGroup = new QButtonGroup(this);

	QGraphicsLinearLayout *p_outer_layout = new QGraphicsLinearLayout(Qt::Vertical);
	p_outer_layout->setContentsMargins(0, 0, 0, 0);
	p_outer_layout->setSpacing(0);

	mpTimelineLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	mpTimelineLayout->setContentsMargins(0, 0, 0, 0);
	mpTimelineLayout->setSpacing(0);
	mpTimelineLayout->addItem(mpTimeline);

	mpSegmentLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	mpSegmentLayout->setContentsMargins(0, 0, 0, 0);
	mpSegmentLayout->setSpacing(0);

	QPushButton *p_button = new QPushButton(QIcon(":/add.png"), NULL);
	p_button->setFlat(true);
	p_button->setAttribute(Qt::WA_NoSystemBackground);
	mpButtonGroup->addButton(p_button);
	GraphicsWidgetHollowProxyWidget *p_proxy_widget = new GraphicsWidgetHollowProxyWidget(this);
	p_proxy_widget->SetWidget(p_button);
	p_proxy_widget->setZValue(2);
	mpSegmentLayout->addItem(p_proxy_widget);

	p_outer_layout->addItem(mpSegmentLayout);
	p_outer_layout->addItem(mpTimelineLayout);
	setLayout(p_outer_layout);

	connect(mpButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rButtonClicked(QAbstractButton*)));
}

void GraphicsWidgetTimeline::resizeEvent(QGraphicsSceneResizeEvent *pEvent) {

	QGraphicsScene *p_scene = scene();
	if(p_scene) {
		QRectF scene_rect = boundingRect();
		scene()->setSceneRect(scene_rect);
	}
}

int GraphicsWidgetTimeline::GetSegmentIndicatorIndex(GraphicsWidgetSegmentIndicator *pSegmentIndicator) const {

	for(int i = 0; i < GetSegmentIndicatorCount(); i++) {
		if(GetSegmentIndicator(i) == pSegmentIndicator) return i;
	}
	return -1;
}

void GraphicsWidgetTimeline::AddSegmentIndicator(GraphicsWidgetSegmentIndicator *pSegmentIndicator, int SegmentIndicatorIndex) {

	QPushButton *p_button = new QPushButton(QIcon(":/add.png"), NULL);
	p_button->setFlat(true);
	p_button->setAttribute(Qt::WA_NoSystemBackground);
	mpButtonGroup->addButton(p_button);
	GraphicsWidgetHollowProxyWidget *p_proxy_widget = new GraphicsWidgetHollowProxyWidget(this);
	p_proxy_widget->SetWidget(p_button);
	p_proxy_widget->setZValue(2);

	if(SegmentIndicatorIndex <= 0) {
		mpSegmentLayout->insertItem(1, pSegmentIndicator);
		mpSegmentLayout->insertItem(2, p_proxy_widget);
	}
	else if(SegmentIndicatorIndex >= GetSegmentIndicatorCount()) {
		mpSegmentLayout->insertItem(mpSegmentLayout->count(), pSegmentIndicator);
		mpSegmentLayout->insertItem(mpSegmentLayout->count(), p_proxy_widget);
	}
	else {
		mpSegmentLayout->insertItem(SegmentIndicatorIndex * 2 + 1, pSegmentIndicator);
		mpSegmentLayout->insertItem(SegmentIndicatorIndex * 2 + 2, p_proxy_widget);
	}
}

void GraphicsWidgetTimeline::MoveSegmentIndicator(GraphicsWidgetSegmentIndicator *pSegmentIndicator, int NewSegmentIndicatorIndex) {

	RemoveSegmentIndicator(pSegmentIndicator);
	AddSegmentIndicator(pSegmentIndicator, NewSegmentIndicatorIndex);
}

void GraphicsWidgetTimeline::RemoveSegmentIndicator(GraphicsWidgetSegmentIndicator *pSegmentIndicator) {

	for(int i = 0; i < mpSegmentLayout->count(); i++) {
		if(mpSegmentLayout->itemAt(i) == pSegmentIndicator) {
			QGraphicsLayoutItem *p_item = mpSegmentLayout->itemAt(i + 1);
			if(GraphicsWidgetHollowProxyWidget *p_hollow_proxy = dynamic_cast<GraphicsWidgetHollowProxyWidget*>(p_item)) {
				mpButtonGroup->removeButton(dynamic_cast<QAbstractButton*>(p_hollow_proxy->GetWidget()));
			}
			mpSegmentLayout->removeAt(i);
			mpSegmentLayout->removeItem(p_item);
			delete p_item;
			break;
		}
	}
}

GraphicsWidgetSegmentIndicator* GraphicsWidgetTimeline::GetSegmentIndicator(int SegmentIndicatorIndex) const {

	GraphicsWidgetSegmentIndicator *p_seqment_indicator = NULL;
	if(SegmentIndicatorIndex <= 0) {
		p_seqment_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(mpSegmentLayout->itemAt(1));
	}
	else if(SegmentIndicatorIndex >= mpSegmentLayout->count()) {
		p_seqment_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(mpSegmentLayout->itemAt(mpSegmentLayout->count() - 2));
	}
	else {
		p_seqment_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(mpSegmentLayout->itemAt(SegmentIndicatorIndex * 2 + 1));
	}
	return p_seqment_indicator;
}

GraphicsWidgetSegmentIndicator* GraphicsWidgetTimeline::GetSegmentIndicator(const QUuid &rId) const {

	for(int i = 0; i < mpSegmentLayout->count(); i++) {
		GraphicsWidgetSegmentIndicator *p_seqment_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(mpSegmentLayout->itemAt(i));
		if(p_seqment_indicator && p_seqment_indicator->GetId() == rId) {
			return p_seqment_indicator;
		}
	}
	return NULL;
}

int GraphicsWidgetTimeline::GetSegmentIndicatorCount() const {

	return (mpSegmentLayout->count() - 1) / 2;
}

void GraphicsWidgetTimeline::rButtonClicked(QAbstractButton *pButton) {

	// Find the index pButton belongs to
	for(int i = 0; i < mpSegmentLayout->count(); i++) {
		GraphicsWidgetHollowProxyWidget *p_hollow_proxy = dynamic_cast<GraphicsWidgetHollowProxyWidget*>(mpSegmentLayout->itemAt(i));
		if(p_hollow_proxy) {
			if(p_hollow_proxy->GetWidget() == pButton) {
				emit NewSegmentRequest(i / 2);
				break;
			}
		}
	}
}

void GraphicsWidgetTimeline::contextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent) {

	GraphicsSceneTimeline *p_scene = qobject_cast<GraphicsSceneTimeline*>(scene());
	GraphicsWidgetSegmentIndicator *p_indicator = NULL;
	if(p_scene) {
		QList<QGraphicsItem*> items = p_scene->items(pEvent->scenePos());
		for(int i = 0; i < items.size(); i++) {
			p_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(items.at(i));
			if(p_indicator) break;
		}
	}
	if(p_indicator) {
		QMenu menu;
		QAction *p_delete_action = menu.addAction(QIcon(":/delete.png"), tr("&Remove Segment"));
		if(GetSegmentIndicatorCount() <= 1) p_delete_action->setDisabled(true);
		QAction *p_add_left_action = menu.addAction(QIcon(":/add.png"), tr("&Insert Segment to the left"));
		QAction *p_add_right_action = menu.addAction(QIcon(":/add.png"), tr("&Insert Segment to the right"));
		QAction *p_selected_action = menu.exec(pEvent->screenPos());
		if(p_selected_action == p_delete_action) {
			emit DeleteSegmentRequest(p_indicator->GetId());
		}
		int segment_offset = 0;
		if((p_selected_action == p_add_left_action) || (p_selected_action == p_add_right_action)) {
			if (p_selected_action == p_add_right_action) segment_offset = 1;
			for(int i = 0; i < mpSegmentLayout->count(); i++) {
				GraphicsWidgetSegmentIndicator *p_seqment_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(mpSegmentLayout->itemAt(i));
				if (p_seqment_indicator) {
					if (p_indicator->GetId() == p_seqment_indicator->GetId()) {
						emit NewSegmentRequest(i / 2 + segment_offset);
						break;
					}
				}
			}
		}
	}
}

GraphicsWidgetTimeline::GraphicsWidgetDrawnTimeline::GraphicsWidgetDrawnTimeline(GraphicsWidgetTimeline *pParent) :
GraphicsWidgetBase(pParent) {

	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void GraphicsWidgetTimeline::GraphicsWidgetDrawnTimeline::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	const qreal lod = pOption->levelOfDetailFromTransform(pPainter->worldTransform());
	const qreal sq_lod = lod * lod;
	const qint32 rounded_editrate = GetCplEditRate().GetRoundendQuotient();
	const QRectF exposed_rect = pOption->exposedRect;
	const QRectF track_rect(boundingRect());
	const qreal track_height = boundingRect().height();
	const quint32 exposed_left = (quint32)exposed_rect.left();
	const quint32 exposed_right = (quint32)exposed_rect.right();
	const quint32 pixel_distance_frames = 1;
	const quint32 pixel_distance_seconds = rounded_editrate;
	const quint32 pixel_distance_10_seconds = pixel_distance_seconds * 10;
	const quint32 pixel_distance_30_seconds = pixel_distance_seconds * 30;
	const quint32 pixel_distance_minutes = pixel_distance_seconds * 60;
	const quint32 pixel_distance_10_minutes = pixel_distance_minutes * 10;
	const quint32 pixel_distance_30_minutes = pixel_distance_minutes * 30;
	const quint32 pixel_distance_hours = pixel_distance_minutes * 60;
	const quint32 shown_pixel_distance_frames = pixel_distance_frames * sq_lod;
	const quint32 shown_pixel_distance_seconds = rounded_editrate * sq_lod;
	const quint32 shown_pixel_distance_10_seconds = pixel_distance_10_seconds * sq_lod;
	const quint32 shown_pixel_distance_30_seconds = pixel_distance_30_seconds * sq_lod;
	const quint32 shown_pixel_distance_minutes = pixel_distance_minutes * sq_lod;
	const quint32 shown_pixel_distance_10_minutes = pixel_distance_10_minutes * sq_lod;
	const quint32 shown_pixel_distance_30_minutes = pixel_distance_30_minutes * sq_lod;
	const quint32 shown_pixel_distance_hours = pixel_distance_hours * sq_lod;

	QLinearGradient gradient(QPointF(0, track_rect.top()), QPointF(0, track_rect.bottom()));
	gradient.setColorAt(0, QColor(CPL_COLOR_TIMELINE_TOP));
	gradient.setColorAt(1, QColor(CPL_COLOR_TIMELINE_BOTTOM));
	QPen pen;
	pen.setCosmetic(true); // no scaling
	pen.setColor(QColor(CPL_BORDER_COLOR));
	pen.setWidth(0);
	QFont font("Arial");
	font.setPixelSize(8);
	pPainter->setFont(font);
	pPainter->setPen(pen);
	pPainter->fillRect(track_rect.intersected(exposed_rect), gradient);
	QRectF border_rect = track_rect.intersected(exposed_rect);
	border_rect.moveTop(0);
	// When rendering with a one pixel wide pen the QRectF's boundary line will be rendered to the right and below the mathematical rectangle's boundary line. 
	// So we need to substract 1. When using an anti-aliased painter other rules apply. See QRect documentation.
	border_rect.setHeight(track_rect.height() - 1);
	pPainter->drawLine(border_rect.bottomLeft(), border_rect.bottomRight());
	pen.setColor(QColor(CPL_COLOR_TIMELINE_TOP));
	pPainter->setPen(pen);
	pPainter->drawLine(border_rect.topLeft(), border_rect.topRight());
	pen.setColor(QColor(CPL_COLOR_TIMELINE_TEXT_MARK));
	pPainter->setPen(pen);

	// 	QTransform transf = pPainter->transform(); // TODO: Is there a better way?
	// 	pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
	// 	pPainter->drawText(exposed_rect.left() * transf.m11() - 6, 0.17 * track_height, 500, 20, Qt::AlignRight, QString("this: %9 exposed: %1,%2 %3x%4 geometry: %5,%6 %7x%8").arg(exposed_rect.left()).arg(exposed_rect.top()).arg(exposed_rect.width()).arg(exposed_rect.height()).arg(geometry().left()).arg(geometry().top()).arg(geometry().width()).arg(geometry().height()).arg((quint32)this, 8, 16));
	// 	qDebug() << "paint calll: " << this << "exposed: " << exposed_rect;
	// 	pPainter->setTransform(transf);


	if(shown_pixel_distance_frames >= 4) {
		for(quint32 i = exposed_left; i <= exposed_right; i++) {
			pPainter->drawLine(i, 0, i, 0.14 * track_height); // frames
			if(shown_pixel_distance_frames >= 10) {
				QTransform transf = pPainter->transform(); // TODO: Is there a better way?
				pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
				pPainter->drawText(i * transf.m11() - 6, 0.17 * track_height, 12, 8, Qt::AlignCenter, QString("%1").arg(i % rounded_editrate, 2, 10, QChar('0')));
				pPainter->setTransform(transf);
			}
		}
	}
	if(shown_pixel_distance_seconds >= 4) {
		font.setPixelSize(10);
		pPainter->setFont(font);
		for(quint32 i = exposed_left - (exposed_left % rounded_editrate); i <= exposed_right; i += pixel_distance_seconds) {
			pPainter->drawLine(i, 0.83 * track_height, i, track_height); // seconds
			if(shown_pixel_distance_seconds >= 55) {
				QTransform transf = pPainter->transform(); // TODO: Is there a better way?
				pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
				pPainter->drawText(i * transf.m11() - 22, 0, 44, track_height, Qt::AlignCenter, Timecode(GetCplEditRate(), i).GetAsString("%1:%2:%3"));
				pPainter->setTransform(transf);
			}
		}
	}
	if(shown_pixel_distance_10_seconds >= 4) {
		font.setPixelSize(10);
		pPainter->setFont(font);
		for(quint32 i = exposed_left - (exposed_left % pixel_distance_10_seconds); i <= exposed_right; i += pixel_distance_10_seconds) {
			pPainter->drawLine(i, 0.75 * track_height, i, track_height); // 10 seconds
			if(shown_pixel_distance_10_seconds >= 55 && shown_pixel_distance_seconds < 55) {
				QTransform transf = pPainter->transform(); // TODO: Is there a better way?
				pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
				pPainter->drawText(i * transf.m11() - 22, 0, 44, track_height, Qt::AlignCenter, Timecode(GetCplEditRate(), i).GetAsString("%1:%2:%3"));
				pPainter->setTransform(transf);
			}
		}
	}
	if(shown_pixel_distance_30_seconds >= 6) {
		font.setPixelSize(10);
		pPainter->setFont(font);
		for(quint32 i = exposed_left - (exposed_left % pixel_distance_30_seconds); i <= exposed_right; i += pixel_distance_30_seconds) {
			pPainter->drawLine(i, 0.7 * track_height, i, track_height); // 30 seconds
			if(shown_pixel_distance_30_seconds >= 55 && shown_pixel_distance_10_seconds < 55) {
				QTransform transf = pPainter->transform(); // TODO: Is there a better way?
				pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
				pPainter->drawText(i * transf.m11() - 22, 0, 44, track_height, Qt::AlignCenter, Timecode(GetCplEditRate(), i).GetAsString("%1:%2:%3"));
				pPainter->setTransform(transf);
			}
		}
	}
	if(shown_pixel_distance_minutes >= 6) {
		font.setPixelSize(10);
		pPainter->setFont(font);
		if(shown_pixel_distance_30_seconds >= 6) {
			pen.setWidth(2);
			pPainter->setPen(pen);
		}
		else {
			pen.setWidth(1);
			pPainter->setPen(pen);
		}
		for(quint32 i = exposed_left - (exposed_left % pixel_distance_minutes); i <= exposed_right; i += pixel_distance_minutes) {
			pPainter->drawLine(i, 0.7 * track_height, i, track_height); // minutes
			if(shown_pixel_distance_minutes >= 40 && shown_pixel_distance_30_seconds < 55) {
				QTransform transf = pPainter->transform(); // TODO: Is there a better way?
				pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
				pPainter->drawText(i * transf.m11() - 22, 0, 44, track_height, Qt::AlignCenter, Timecode(GetCplEditRate(), i).GetAsString("%1:%2"));
				pPainter->setTransform(transf);
			}
		}
	}
	if(shown_pixel_distance_30_minutes >= 7) {
		font.setPixelSize(10);
		pPainter->setFont(font);
		pen.setWidth(2);
		pPainter->setPen(pen);
		for(quint32 i = exposed_left - (exposed_left % pixel_distance_30_minutes); i <= exposed_right; i += pixel_distance_30_minutes) {
			pPainter->drawLine(i, 0.7 * track_height, i, track_height); // 30 minutes
			pPainter->drawLine(i, 0, i, 0.14 * track_height); // 30 minutes
			if(shown_pixel_distance_30_minutes >= 40 && shown_pixel_distance_minutes < 40) {
				QTransform transf = pPainter->transform(); // TODO: Is there a better way?
				pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1));
				pPainter->drawText(i * transf.m11() - 22, 0, 44, track_height, Qt::AlignCenter, Timecode(GetCplEditRate(), i).GetAsString("%1:%2"));
				pPainter->setTransform(transf);
			}
		}
	}
	if(shown_pixel_distance_hours >= 7 && shown_pixel_distance_frames < 4) {
		font.setPixelSize(10);
		pPainter->setFont(font);
		pen.setWidth(2);
		pPainter->setPen(pen);
		for(quint32 i = exposed_left - (exposed_left % pixel_distance_hours); i <= exposed_right; i += pixel_distance_hours) {
			pPainter->drawLine(i, 0, i, 0.3 * track_height); // hours
			pPainter->drawLine(i, 0.7 * track_height, i, track_height); // 30 minutes
		}
	}
}

QSizeF GraphicsWidgetTimeline::GraphicsWidgetDrawnTimeline::sizeHint(Qt::SizeHint which, const QSizeF &constraint /*= QSizeF()*/) const {

	QSizeF size;
	if(constraint.isValid() == false) {
		switch(which) {
			case Qt::MinimumSize:
				size = QSizeF(0, 40);
				break;
			case Qt::PreferredSize:
				size = QSizeF(0, 40);
				break;
			case Qt::MaximumSize:
				size = QSizeF(std::numeric_limits<qint32>::max(), std::numeric_limits<qint32>::max());
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
		size = constraint;
	}
	return size;
}
