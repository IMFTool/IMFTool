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
#include "GraphicsCommon.h"
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QFontMetricsF>
#include <QPropertyAnimation>
#include <QFont>
#include <QTimer>
#include <limits>


GraphicsWidgetBase::GraphicsWidgetBase(QGraphicsItem *pParent /*= NULL*/) :
QGraphicsWidget(pParent), AbstractGridExtension() {

}

EditRate GraphicsWidgetBase::GetCplEditRate() const {

	GraphicsSceneBase *p_scene = qobject_cast<GraphicsSceneBase*>(scene());
	if(p_scene) return p_scene->GetCplEditRate();
	return EditRate();
}

QVariant GraphicsWidgetBase::itemChange(GraphicsItemChange change, const QVariant &rValue) {

	if(change == QGraphicsItem::ItemSceneHasChanged) {
		CplEditRateChanged();
	}
	return QGraphicsWidget::itemChange(change, rValue);
}

void GraphicsWidgetBase::customEvent(QEvent *pEvent) {

	if(pEvent && pEvent->type() == UserEventCplEditRateChange) {
		CplEditRateChanged();
	}
}

GraphicsObjectBase::GraphicsObjectBase(QGraphicsItem *pParent /*= NULL*/) :
QGraphicsObject(pParent), AbstractGridExtension() {

}

EditRate GraphicsObjectBase::GetCplEditRate() const {

	GraphicsSceneBase *p_scene = qobject_cast<GraphicsSceneBase*>(scene());
	if(p_scene) return p_scene->GetCplEditRate();
	return EditRate();
}

QVariant GraphicsObjectBase::itemChange(GraphicsItemChange change, const QVariant &rValue) {

	if(change == QGraphicsItem::ItemSceneHasChanged) {
		CplEditRateChanged();
	}
	return QGraphicsObject::itemChange(change, rValue);
}

void GraphicsObjectBase::customEvent(QEvent *pEvent) {

	if(pEvent && pEvent->type() == UserEventCplEditRateChange) {
		CplEditRateChanged();
	}
}


GraphicsObjectVerticalIndicator::GraphicsObjectVerticalIndicator(qreal width, qreal height, const QColor &rColor, QGraphicsItem *pParent /*= NULL*/) :
GraphicsObjectBase(pParent), AbstractViewTransformNotifier(), mColor(rColor), mpLine(NULL), mHeadImage(), mText(), mHeadSize(15, 20), mExtendGrid(false) {

	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption | QGraphicsItem::ItemSendsGeometryChanges);
	mpLine = new GraphicsItemLine(width, height, this, rColor);
	HideHead();
	setTransform(QTransform::fromScale(1 / GetViewTransform().m11(), 1));
}

void GraphicsObjectVerticalIndicator::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF head_rect(boundingRect());
	QPen pen;
	pen.setWidth(0); // cosmetic
	QBrush brush(Qt::SolidPattern);

	QColor color(mColor);
	QColor light_color(mColor.lighter());
	QColor dark_color(mColor.darker());

	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(head_rect.intersected(exposed_rect));
	head_rect.adjust(0, 0, -1, -1);

	const QPointF points[5] = {
		QPointF(head_rect.center().x(), head_rect.bottom()),
		QPointF(head_rect.left(), head_rect.bottom() - (head_rect.center().x() - head_rect.left())),
		head_rect.topLeft(),
		head_rect.topRight(),
		QPointF(head_rect.right(), head_rect.bottom() - (head_rect.center().x() - head_rect.left())),
	};

	QFont font;
	font.setPixelSize(boundingRect().width());
	QFontMetricsF font_metrics(font);
	QString text = font_metrics.elidedText(mText, Qt::ElideRight, points[1].y() - points[2].y());

	pPainter->save();
	if(visible_rect.isEmpty() == false) {
		if(acceptHoverEvents() == true && pOption->state & QStyle::State_MouseOver) {
			if(pOption->state & QStyle::State_Selected) {
				pen.setColor(dark_color);
				brush.setColor(dark_color);
			}
			else {
				pen.setColor(light_color);
				brush.setColor(light_color);
			}
			if(mHeadImage.isNull() == true) {
				pPainter->setFont(font);
				pPainter->setPen(pen);
				pPainter->setBrush(brush);
				pPainter->drawPolygon(points, 5);
				if(mText.isEmpty() == false) {
					pen.setColor(QColor(CPL_FONT_COLOR));
					QTransform transf(pPainter->transform());
					transf.rotate(-90).translate(-points[4].y() - points[3].y(), font_metrics.height() / 2 - 1.5);
					pPainter->setTransform(transf);
					pPainter->setPen(pen);
					pPainter->drawText(0, 0, text);
				}
			}
			else {
				QPointF point(boundingRect().topLeft().x() - .5, boundingRect().topLeft().y() - .5);
				pPainter->drawPixmap(point, mHeadImage);
			}
		}
		else {
			if(pOption->state & QStyle::State_Selected) {
				pen.setColor(dark_color);
				brush.setColor(dark_color);
			}
			else {
				pen.setColor(color);
				brush.setColor(color);
			}
			if(mHeadImage.isNull() == true) {
				pPainter->setFont(font);
				pPainter->setPen(pen);
				pPainter->setBrush(brush);
				pPainter->drawPolygon(points, 5);
				if(mText.isEmpty() == false) {
					pen.setColor(QColor(CPL_FONT_COLOR));
					QTransform transf(pPainter->transform());
					transf.rotate(-90).translate(-points[4].y() - points[3].y(), font_metrics.height() / 2 - 1.5);
					pPainter->setTransform(transf);
					pPainter->setPen(pen);
					pPainter->drawText(0, 0, text);
				}
			}
			else {
				QPointF point(boundingRect().topLeft().x() - .5, boundingRect().topLeft().y() - .5);
				pPainter->drawPixmap(point, mHeadImage);
			}
		}
	}
	pPainter->restore();
}

void GraphicsObjectVerticalIndicator::ViewTransformEvent(const QTransform &rViewTransform) {

	setTransform(QTransform::fromScale(1 / GetViewTransform().m11(), 1));
}

QVariant GraphicsObjectVerticalIndicator::itemChange(GraphicsItemChange change, const QVariant &rValue) {

	if(change == ItemPositionChange) {
		QPointF new_pos = rValue.toPointF();
		if(new_pos.x() != pos().x()) emit XPosChanged(new_pos.x());
	}
	return GraphicsObjectBase::itemChange(change, rValue);
}

void GraphicsObjectVerticalIndicator::SetXPos(qreal xPos) {

	setX(xPos);
}

bool GraphicsObjectVerticalIndicator::ExtendGrid(QPointF &rPoint, eGridPosition which) const {

	if(which == Vertical) {
		QPointF ret(mapFromScene(rPoint));
		ret.setX(0);
		rPoint = mapToScene(ret);
		return mExtendGrid;
	}
	return false;
}

QRectF GraphicsObjectVerticalIndicator::boundingRect() const {

	QRectF rect(QPointF(0, 0), QSizeF(mHeadSize));
	if(mHeadImage.isNull() == false) rect.setSize(mHeadImage.size());
	rect.moveLeft((int)(-rect.width() / 2));
	return rect;
}

QGraphicsView* GraphicsObjectVerticalIndicator::GetObservableView() const {

	if(scene() && scene()->views().empty() == false) {
		return scene()->views().first();
	}
	return NULL;
}

QRectF GraphicsObjectVerticalIndicator::GraphicsItemLine::boundingRect() const {

	QRectF rect(QPointF(0, 0), QSizeF(mLineSize));
	return rect;
}

void GraphicsObjectVerticalIndicator::GraphicsItemLine::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF indicator_rect(boundingRect());
	QPen pen;
	pen.setWidth(0); // cosmetic
	QBrush brush(Qt::SolidPattern);
	brush.setColor(mColor);
	pen.setColor(mColor);

	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(indicator_rect.intersected(exposed_rect));
	if(visible_rect.isEmpty() == false) {
		pPainter->setPen(pen);
		pPainter->setBrush(brush);
		visible_rect.adjust(0, 0, -1., -1.);
		// 		visible_rect.translate(.5, .5);
		if(visible_rect.width() >= 1) pPainter->drawRect(visible_rect);
		else pPainter->drawLine(visible_rect.topLeft(), visible_rect.bottomLeft());
	}
}

GraphicsObjectVerticalIndicator::GraphicsItemLine::GraphicsItemLine(qreal width, qreal height, GraphicsObjectVerticalIndicator *pParent, const QColor &rColor) :
QGraphicsItem(pParent), mColor(rColor), mLineSize(width, height) {

	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption | QGraphicsItem::ItemStacksBehindParent); // enables pOption::exposedRect in GraphicsWidgetTimeline::paint()
}

qreal GraphicsHelper::GetDefaultFontHeight() {

	QFontMetricsF font_metrics = QFontMetricsF(QFont());
	return font_metrics.height();
}

QColor GraphicsHelper::GetSegmentColor(int SegmentIndex, bool transparent /*= false*/) {

	return QColor::fromHsv(SegmentIndex * 25 % 359, 82, 100, transparent ? 60 : 255);
}

GraphicsWidgetHollowProxyWidget::GraphicsWidgetHollowProxyWidget(QGraphicsItem *pParent /*= NULL*/) :
QGraphicsWidget(pParent), AbstractViewTransformNotifier(), mpHover(NULL), mpProxyWidget(NULL), mpTimerHideProxy(NULL), mpTimerShowProxy(NULL) {

	mpHover = new GraphicsItemHover(this);
	mpProxyWidget = new GraphicsProxyWidget(this);
	mpProxyWidget->hide();
	setFlags(QGraphicsItem::ItemHasNoContents | QGraphicsItem::ItemSendsGeometryChanges);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	setTransform(QTransform::fromScale(1 / GetViewTransform().m11(), 1));
	mpTimerHideProxy = new QTimer(this);
	mpTimerHideProxy->setSingleShot(true);
	mpTimerHideProxy->setInterval(1000);
	mpTimerShowProxy = new QTimer(this);
	mpTimerShowProxy->setSingleShot(true);
	mpTimerShowProxy->setInterval(100);

	connect(mpTimerHideProxy, SIGNAL(timeout()), this, SLOT(HideProxyWidget()));
	connect(mpTimerShowProxy, SIGNAL(timeout()), this, SLOT(ShowProxyWidget()));
}

QSizeF GraphicsWidgetHollowProxyWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint /*= QSizeF()*/) const {

	QSizeF size;
	if(constraint.isValid() == false) {
		switch(which) {
			case Qt::MinimumSize:
				size = QSizeF(0, 0);
				break;
			case Qt::PreferredSize:
				size = QSizeF(0, 0);
				break;
			case Qt::MaximumSize:
				size = QSizeF(0, std::numeric_limits<qint32>::max());
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

void GraphicsWidgetHollowProxyWidget::ViewTransformEvent(const QTransform &rViewTransform) {

	setTransform(QTransform::fromScale(1 / rViewTransform.m11(), 1));
}

QGraphicsView* GraphicsWidgetHollowProxyWidget::GetObservableView() const {

	if(scene() && scene()->views().empty() == false) {
		return scene()->views().first();
	}
	return NULL;
}

void GraphicsWidgetHollowProxyWidget::resizeEvent(QGraphicsSceneResizeEvent *pEvent) {

	mpHover->SetHeight(pEvent->newSize().height());
}

void GraphicsWidgetHollowProxyWidget::HoverItemActive(bool active) {

	if(active) {
		mpProxyWidget->setPos(0 - mpProxyWidget->boundingRect().width() / 2, boundingRect().bottom());
		mpTimerHideProxy->stop();
		mpTimerShowProxy->start();
	}
	else {
		mpTimerShowProxy->stop();
		mpTimerHideProxy->start();
	}
}

void GraphicsWidgetHollowProxyWidget::HoverProxyActive(bool active) {

	if(active) {
		mpTimerHideProxy->stop();
	}
	else {
		mpTimerHideProxy->start();
	}
}

void GraphicsWidgetHollowProxyWidget::SetWidget(QWidget *pWidget) {

	mpProxyWidget->setWidget(pWidget);
	mpProxyWidget->hide();
}

QWidget* GraphicsWidgetHollowProxyWidget::GetWidget() const {

	return mpProxyWidget->widget();
}

GraphicsWidgetHollowProxyWidget::GraphicsItemHover::GraphicsItemHover(QGraphicsItem *pParent) :
QGraphicsItem(pParent) {

	setCursor(Qt::UpArrowCursor);
	setFlags(QGraphicsItem::ItemHasNoContents);
	setAcceptHoverEvents(true);
}

QRectF GraphicsWidgetHollowProxyWidget::GraphicsItemHover::boundingRect() const {

	return QRectF(-6, 0, 12, mHeight);
}

void GraphicsWidgetHollowProxyWidget::GraphicsItemHover::hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	GraphicsWidgetHollowProxyWidget *p_parent = qobject_cast<GraphicsWidgetHollowProxyWidget*>(parentObject());
	if(p_parent) {
		p_parent->HoverItemActive(true);
	}
}

void GraphicsWidgetHollowProxyWidget::GraphicsItemHover::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	GraphicsWidgetHollowProxyWidget *p_parent = qobject_cast<GraphicsWidgetHollowProxyWidget*>(parentObject());
	if(p_parent) {
		p_parent->HoverItemActive(false);
	}
}

void GraphicsWidgetHollowProxyWidget::GraphicsItemHover::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF hover_rect(boundingRect());
	QColor dark(0, 0, 0, 120);

	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(hover_rect.intersected(exposed_rect));
	visible_rect.translate(-.5, -.5);
	if(visible_rect.isEmpty() == false) {
		if(acceptHoverEvents() == true && pOption->state & QStyle::State_MouseOver) {
			pPainter->fillRect(visible_rect, dark);
		}
	}
}

void GraphicsWidgetHollowProxyWidget::GraphicsProxyWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent) {

	QGraphicsProxyWidget::hoverEnterEvent(pEvent);
	GraphicsWidgetHollowProxyWidget *p_parent = qobject_cast<GraphicsWidgetHollowProxyWidget*>(parentObject());
	if(p_parent) {
		p_parent->HoverProxyActive(true);
	}
}

void GraphicsWidgetHollowProxyWidget::GraphicsProxyWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent) {

	QGraphicsProxyWidget::hoverLeaveEvent(pEvent);
	GraphicsWidgetHollowProxyWidget *p_parent = qobject_cast<GraphicsWidgetHollowProxyWidget*>(parentObject());
	if(p_parent) {
		p_parent->HoverProxyActive(false);
	}
}
