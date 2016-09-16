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
#include "GraphicsWidgetResources.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetSegment.h"
#include "CompositionPlaylistCommands.h"
#include "GraphicsWidgetComposition.h"
#include "MetadataExtractor.h"
#include <QGraphicsSceneResizeEvent>
#include <QStyleOptionGraphicsItem>
#include <QMenu>
#include <QToolTip>


AbstractGraphicsWidgetResource::AbstractGraphicsWidgetResource(GraphicsWidgetSequence *pParent, cpl::BaseResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset /*= QSharedPointer<AssetMxfTrack>(NULL)*/, const QColor &rColor /*= QColor(Qt::white)*/) :
GraphicsWidgetBase(pParent), mpData(pResource), mAssset(rAsset), mColor(rColor), mOldEntryPoint(), mOldSourceDuration(-1), mpLeftTrimHandle(NULL), mpRightTrimHandle(NULL), mpDurationIndicator(NULL), mpVerticalIndicator(NULL) {

	mpLeftTrimHandle = new TrimHandle(this, Left);
	mpLeftTrimHandle->hide();
	mpRightTrimHandle = new TrimHandle(this, Right);
	mpRightTrimHandle->hide();
	mpDurationIndicator = new GraphicsItemDurationIndicator(this);
	mpDurationIndicator->hide();
	mpVerticalIndicator = new GraphicsObjectVerticalIndicator(1, 0, QColor(CPL_COLOR_DEFAULT_SNAP_INDICATOR), this);
	mpVerticalIndicator->hide();
	setAcceptHoverEvents(true);
	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption | QGraphicsItem::ItemIsSelectable); // enables pOption::exposedRect in GraphicsWidgetTimeline::paint()
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	if(mAssset) connect(mAssset.data(), SIGNAL(AssetModified(Asset*)), this, SLOT(rAssetModified()));
}

void AbstractGraphicsWidgetResource::resizeEvent(QGraphicsSceneResizeEvent *pEvent) {

	mpLeftTrimHandle->SetHeight(pEvent->newSize().height());
	mpRightTrimHandle->setPos(pEvent->newSize().width() / GetRepeatCount(), 0);
	mpRightTrimHandle->SetHeight(pEvent->newSize().height());
	QRectF indicator_rect(mpDurationIndicator->boundingRect());
	indicator_rect.setHeight(pEvent->newSize().height());
	mpDurationIndicator->SetRect(indicator_rect);
}

Duration AbstractGraphicsWidgetResource::GetIntrinsicDuration() const {

	return Duration(mpData->getIntrinsicDuration());
}

Duration AbstractGraphicsWidgetResource::GetSourceDuration() const {

	if(mpData->getSourceDuration().present() == true) {
		return Duration(mpData->getSourceDuration().get());
	}
	return GetIntrinsicDuration() - GetEntryPoint();
}

Duration AbstractGraphicsWidgetResource::GetEntryPoint() const {

	if(mpData->getEntryPoint().present() == true) {
		return Duration(mpData->getEntryPoint().get());
	}
	return Duration();
}

EditRate AbstractGraphicsWidgetResource::GetEditRate() const {

	if(mpData->getEditRate().present() == true) {
		return ImfXmlHelper::Convert(mpData->getEditRate().get());
	}
	return GetCplEditRate();
}

GraphicsWidgetSequence* AbstractGraphicsWidgetResource::GetSequence() const {

	return qobject_cast<GraphicsWidgetSequence*>(parentObject());
}

int AbstractGraphicsWidgetResource::GetRepeatCount() const {

	if(mpData->getRepeatCount().present() == true) {
		return mpData->getRepeatCount().get();
	}
	return 1;
}

UserText AbstractGraphicsWidgetResource::GetAnnotation() const {

	if(mpData->getAnnotation().present() == true) {
		return ImfXmlHelper::Convert(mpData->getAnnotation().get());
	}
	return UserText();
}

void AbstractGraphicsWidgetResource::SetEntryPoint(const Duration &rEntryPoint) {

	double samples_factor = ResourceErPerCompositionEr(GetCplEditRate());
	Duration new_entry_point(rEntryPoint);
	Duration current_source_duration(GetSourceDuration());
	Duration current_entry_point(GetEntryPoint());
	if(rEntryPoint < 0) new_entry_point = 0;
	else if(rEntryPoint > GetEntryPoint() + current_source_duration - samples_factor) new_entry_point = GetEntryPoint() + current_source_duration - samples_factor;
	mpData->setSourceDuration(xml_schema::NonNegativeInteger(current_source_duration.GetCount() - (new_entry_point.GetCount() - GetEntryPoint().GetCount())));
	mpData->setEntryPoint(xml_schema::NonNegativeInteger(new_entry_point.GetCount()));
	if(current_entry_point != new_entry_point) emit EntryPointChanged(current_entry_point, new_entry_point);
	if(current_source_duration != GetSourceDuration()) emit SourceDurationChanged(current_source_duration, GetSourceDuration());
	updateGeometry();
	QRectF rect = boundingRect();
	rect.setWidth(qint64((GetIntrinsicDuration().GetCount()) / samples_factor));
	rect.moveLeft(-qint64(GetEntryPoint().GetCount() / samples_factor));
	mpDurationIndicator->SetRect(rect);
}

void AbstractGraphicsWidgetResource::SetSourceDuration(const Duration &rSourceDuration) {

	double samples_factor = ResourceErPerCompositionEr(GetCplEditRate());
	int rounded_samples_factor = (int)(samples_factor + .5); // TODO: What about samples_factor with fraction.
	Duration new_source_duration(rSourceDuration);
	Duration current_source_duration(GetSourceDuration());
	if(new_source_duration < rounded_samples_factor) new_source_duration = rounded_samples_factor;
	else if(new_source_duration > GetIntrinsicDuration() - GetEntryPoint()) new_source_duration = GetIntrinsicDuration() - GetEntryPoint();
	mpData->setSourceDuration(xml_schema::NonNegativeInteger(new_source_duration.GetCount()));
	if(new_source_duration != current_source_duration) emit SourceDurationChanged(current_source_duration, new_source_duration);
	updateGeometry();
	QRectF rect = boundingRect();
	rect.setWidth(qint64((GetIntrinsicDuration().GetCount()) / samples_factor));
	rect.moveLeft(-qint64(GetEntryPoint().GetCount() / samples_factor));
	mpDurationIndicator->SetRect(rect);
}

void AbstractGraphicsWidgetResource::TrimResource(qint64 pos, qint64 lastPos, eTrimHandlePosition epos) {

	double samples_factor = ResourceErPerCompositionEr(GetCplEditRate());
	int rounded_samples_factor = (int)(samples_factor + .5); // TODO: What about samples_factor with fraction.
	QList<AbstractGraphicsWidgetResource*> resources;
	QList<AbstractGridExtension*> ignore_list;
	if(GetSequence()) {
		resources = GetSequence()->GetResources();
		for(int i = 0; i < resources.count(); i++) ignore_list.push_back(resources.at(i));
	}
	if(epos == Left) {
		QRectF rect(0, 0, 0, 0);
		rect.setWidth(qint64((mOldSourceDuration.GetCount() + mOldEntryPoint.GetCount()) / samples_factor));
		qint64 move_left = qint64(mOldEntryPoint.GetCount() / samples_factor);
		rect.moveLeft(-move_left);
		rect = mapRectToScene(rect);
		QPointF grid_point(rect.right() - (pos - rect.left() - move_left), 0);
		GraphicsSceneComposition *p_scene = qobject_cast<GraphicsSceneComposition*>(scene());
		GraphicsSceneComposition::GridInfo grid_info;
		if(p_scene) {
			QRectF search_rect;
			if(GetSequence() && GetSequence()->GetSegment()) search_rect = GetSequence()->GetSegment()->mapRectToScene(GetSequence()->GetSegment()->boundingRect());
			search_rect.setTop(p_scene->sceneRect().top());
			search_rect.setBottom(p_scene->sceneRect().bottom());
			grid_info = p_scene->SnapToGrid(grid_point, Vertical, mapRectToScene(search_rect), ignore_list);
			if(grid_info.IsVerticalSnap == true) {
				mpVerticalIndicator->SetColor(grid_info.ColorAdvice);
				mpVerticalIndicator->SetHeight(p_scene->height());
				mpVerticalIndicator->setPos(mapFromScene(QPointF(grid_info.SnapPos.x(), p_scene->sceneRect().top())));
				mpVerticalIndicator->show();
			}
			else mpVerticalIndicator->hide();
		}
		Duration new_entry_point = ((-grid_info.SnapPos.x() + rect.right() + rect.left()) - rect.left() + move_left) * samples_factor;
		SetEntryPoint(new_entry_point);
	}
	else if(epos == Right) {
		QPointF grid_point(pos, 0);
		GraphicsSceneComposition *p_scene = qobject_cast<GraphicsSceneComposition*>(scene());
		GraphicsSceneComposition::GridInfo grid_info;
		if(p_scene) {
			QRectF search_rect;
			if(GetSequence() && GetSequence()->GetSegment()) search_rect = GetSequence()->GetSegment()->mapRectToScene(GetSequence()->GetSegment()->boundingRect());
			grid_info = p_scene->SnapToGrid(grid_point, Vertical, search_rect, ignore_list);
			if(grid_info.IsVerticalSnap == true) {
				mpVerticalIndicator->SetColor(grid_info.ColorAdvice);
				mpVerticalIndicator->SetHeight(p_scene->height());
				mpVerticalIndicator->setPos(mapFromScene(QPointF(grid_info.SnapPos.x(), p_scene->sceneRect().top())));
				mpVerticalIndicator->show();
			}
			else mpVerticalIndicator->hide();
		}
		int local_pos = mapFromScene(QPointF(grid_info.SnapPos.x(), 0)).x();
		Duration new_source_duration = ceil(local_pos * samples_factor);			//Why???
		SetSourceDuration(new_source_duration);
	}
}

QUuid AbstractGraphicsWidgetResource::GetId() const {

	return ImfXmlHelper::Convert(mpData->getId());
}

void AbstractGraphicsWidgetResource::TrimHandleInUse(eTrimHandlePosition pos, bool active) {

	if(active == true) {
		if(pos == Left) {
			mOldEntryPoint = GetEntryPoint();
			mOldSourceDuration = GetSourceDuration();
		}
		else if(pos == Right) {
			mOldEntryPoint = GetEntryPoint();
			mOldSourceDuration = GetSourceDuration();
		}
		MaximizeZValue();
	}
	else {
		if(pos == Left) {
			if(mOldEntryPoint != GetEntryPoint()) {
				if(GraphicsSceneComposition* p_scene = qobject_cast<GraphicsSceneComposition*>(scene())) p_scene->DelegateCommand(new SetEntryPointCommand(this, mOldEntryPoint, GetEntryPoint()));
				else qWarning() << "Couldn't delegate trim command.";
			}
		}
		else if(pos == Right) {
			if(mOldSourceDuration != GetSourceDuration()) {
				if(GraphicsSceneComposition* p_scene = qobject_cast<GraphicsSceneComposition*>(scene())) p_scene->DelegateCommand(new SetSourceDurationCommand(this, mOldSourceDuration, GetSourceDuration()));
				else qWarning() << "Couldn't delegate trim command.";
			}
		}
		mOldSourceDuration = Duration(-1);
		mOldEntryPoint = Duration(-1);
		RestoreZValue();
	}
	mpDurationIndicator->setVisible(active);
	mpVerticalIndicator->setVisible(active);
}

void AbstractGraphicsWidgetResource::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF resource_rect(boundingRect());
	resource_rect.setWidth(resource_rect.width() / GetRepeatCount());
	QPen pen;
	pen.setWidth(0); // cosmetic
	QBrush brush(Qt::SolidPattern);
	if(mAssset.isNull() || mAssset->HasAffinity() == false) {
		brush.setStyle(Qt::BDiagPattern);
		brush.setTransform(pPainter->transform().inverted());
	}
	QColor color(mColor);
	QColor light_color(mColor.lighter(115));
	QColor dark_color(mColor.darker(129));
	QColor light_color_border(mColor.lighter(133));
	QColor dark_color_border(mColor.darker(214));

	pPainter->save();
	for(int i = 0; i < GetRepeatCount(); i++) {
		resource_rect.moveLeft(i * resource_rect.width());
		resource_rect = resource_rect.intersected(boundingRect());
		QRectF exposed_rect(pOption->exposedRect);
		if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
		if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
		QRectF visible_rect(resource_rect.intersected(exposed_rect));
		if(visible_rect.isEmpty() == true) continue;
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());

		if(acceptHoverEvents() == true && pOption->state & QStyle::State_MouseOver) {
			if(pOption->state & QStyle::State_Selected) {
				pen.setColor(dark_color);
				brush.setColor(dark_color);
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
				pen.setColor(dark_color);
				brush.setColor(dark_color);
			}
			else {
				pen.setColor(mColor);
				brush.setColor(mColor);
			}
			pPainter->setPen(pen);
			pPainter->setBrush(brush);
			pPainter->drawRect(visible_rect);
		}

		if(pOption->state & QStyle::State_Selected) pen.setColor(dark_color_border);
		else pen.setColor(light_color_border);
		pPainter->setPen(pen);
		if(exposed_rect.top() <= resource_rect.top()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.topRight());
		if(exposed_rect.left() <= resource_rect.left()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.bottomLeft());
		if(pOption->state & QStyle::State_Selected) pen.setColor(light_color_border);
		else pen.setColor(dark_color_border);
		pPainter->setPen(pen);
		if(exposed_rect.bottom() >= resource_rect.bottom()) pPainter->drawLine(visible_rect.bottomLeft(), visible_rect.bottomRight());
		if(exposed_rect.right() >= resource_rect.right()) pPainter->drawLine(visible_rect.topRight(), visible_rect.bottomRight());
	}
	pPainter->restore();
}

QSizeF AbstractGraphicsWidgetResource::sizeHint(Qt::SizeHint which, const QSizeF &rConstraint /*= QSizeF()*/) const {

	QSizeF size;
	qint64 duration_to_width = GetRepeatCount() * (qint64)(GetSourceDuration().GetCount() / ResourceErPerCompositionEr(GetCplEditRate()));
	if(rConstraint.isValid() == false) {
		switch(which) {
			case Qt::MinimumSize:
				size = QSizeF(duration_to_width, -1);
				break;
			case Qt::PreferredSize:
				size = QSizeF(duration_to_width, -1);
				break;
			case Qt::MaximumSize:
				size = QSizeF(duration_to_width, -1);
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

void AbstractGraphicsWidgetResource::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	mpLeftTrimHandle->hide();
	mpRightTrimHandle->hide();
}

void AbstractGraphicsWidgetResource::hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	if(mpLeftTrimHandle->isEnabled() == true) mpLeftTrimHandle->show();
	if(mpRightTrimHandle->isEnabled() == true) mpRightTrimHandle->show();
}

Timecode AbstractGraphicsWidgetResource::MapToCplTimeline(const Timecode &rLocalTimecode) const {

	QPointF result = mapToScene(QPointF((qint64)((rLocalTimecode.GetOverallFrames() - GetEntryPoint().GetCount()) / ResourceErPerCompositionEr(GetCplEditRate())), 0));
	return Timecode(GetCplEditRate(), result.x());
}

Duration AbstractGraphicsWidgetResource::MapToCplTimeline(const Duration &rLocalDuration) const {

	return Duration((qint64)(rLocalDuration.GetCount() / ResourceErPerCompositionEr(GetCplEditRate())));
}

Timecode AbstractGraphicsWidgetResource::MapFromCplTimeline(const Timecode &rCplTimecode) const {

	QPointF result = mapFromScene(QPointF(rCplTimecode.GetOverallFrames(), 0));
	return Timecode(GetEditRate(), (qint64)((result.x() + GetEntryPoint().GetCount()) * ResourceErPerCompositionEr(GetCplEditRate())));
}

Duration AbstractGraphicsWidgetResource::MapFromCplTimeline(const Duration &rCplDuration) const {

	return Duration((qint64)(rCplDuration.GetCount() * ResourceErPerCompositionEr(GetCplEditRate()) + .5));
}

bool AbstractGraphicsWidgetResource::ExtendGrid(QPointF &rPoint, eGridPosition which) const {

	if(which == Vertical) {
		QPointF ret(mapFromScene(rPoint));
		if(ret.x() > boundingRect().center().x()) ret.setX(boundingRect().right());
		else ret.setX(boundingRect().left());
		rPoint = mapToScene(ret);
		return true;
	}
	return false;
}

void AbstractGraphicsWidgetResource::MaximizeZValue() {

	setZValue(1);
	mpDurationIndicator->setZValue(1);
	mpVerticalIndicator->setZValue(1);
	GraphicsWidgetSequence* p_sequence = GetSequence();
	GraphicsWidgetSegment* p_segment = NULL;
	if(p_sequence) {
		p_sequence->setZValue(1);
		p_segment = p_sequence->GetSegment();
		if(p_segment) p_segment->setZValue(1);
	}
}

void AbstractGraphicsWidgetResource::RestoreZValue() {

	setZValue(0);
	mpDurationIndicator->setZValue(0);
	mpVerticalIndicator->setZValue(0);
	GraphicsWidgetSequence* p_sequence = GetSequence();
	GraphicsWidgetSegment* p_segment = NULL;
	if(p_sequence) {
		p_sequence->setZValue(0);
		p_segment = p_sequence->GetSegment();
		if(p_segment) p_segment->setZValue(0);
	}
}

void AbstractGraphicsWidgetResource::EnableTrimHandle(eTrimHandlePosition pos, bool enable) {

	switch(pos) {
		case AbstractGraphicsWidgetResource::Left:
			mpLeftTrimHandle->setEnabled(enable);
			if(enable == false) mpLeftTrimHandle->hide();
			break;
		case AbstractGraphicsWidgetResource::Right:
			mpRightTrimHandle->setEnabled(enable);
			if(enable == false) mpRightTrimHandle->hide();
			break;
	}
}

void AbstractGraphicsWidgetResource::DisableTrimHandle(eTrimHandlePosition pos, bool disable) {

	EnableTrimHandle(pos, !disable);
}

void AbstractGraphicsWidgetResource::hideEvent(QHideEvent *event) {

	mpRightTrimHandle->hide();
	mpLeftTrimHandle->hide();
	mpDurationIndicator->hide();
	mpVerticalIndicator->hide();
}

void AbstractGraphicsWidgetResource::CplEditRateChanged() {

	updateGeometry();
}

void AbstractGraphicsWidgetResource::rAssetModified() {

	update();
}

AbstractGraphicsWidgetResource::TrimHandle::TrimHandle(AbstractGraphicsWidgetResource *pParent, eTrimHandlePosition pos) :
QGraphicsItem(pParent), AbstractViewTransformNotifier(), mPos(pos), mMouseXOffset(0), mMousePressed(false), mRect() {

	setFlags(QGraphicsItem::ItemSendsScenePositionChanges);
	SetDirection(mPos);
	SetWidth(8);
	if(mPos == Right) setTransform(QTransform::fromScale(1 / GetViewTransform().m11(), 1).translate(-boundingRect().width() - 1, 0));
	else setTransform(QTransform::fromScale(1 / GetViewTransform().m11(), 1));
}

void AbstractGraphicsWidgetResource::TrimHandle::SetDirection(eTrimHandlePosition pos) {

	mPos = pos;
	QCursor cursor;
	switch(mPos) {
		case Left:
			cursor = QCursor(QPixmap(":/cursor_crop_right.png"));
			break;
		case Right:
			cursor = QCursor(QPixmap(":/cursor_crop_left.png"));
			break;
		default:
			cursor = QCursor(Qt::SplitHCursor);
			qWarning() << "Unknown cursor type.";
			break;
	}
	setCursor(cursor);
}

void AbstractGraphicsWidgetResource::TrimHandle::mousePressEvent(QGraphicsSceneMouseEvent *event) {

	AbstractGraphicsWidgetResource *p_resource_widget = qobject_cast<AbstractGraphicsWidgetResource *>(parentWidget());
	if(p_resource_widget) {
		if(event->button() == Qt::LeftButton) {
			mMouseXOffset = (int)(event->scenePos().x() + .5);
			mMousePressed = true;
			p_resource_widget->TrimHandleInUse(mPos, true);
			p_resource_widget->TrimResource((qint64)(event->scenePos().x() + .5), (qint64)(event->scenePos().x() + .5), mPos);
		}
	}
}

void AbstractGraphicsWidgetResource::TrimHandle::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {

	if(!(event->buttons() & Qt::LeftButton)) return;
	AbstractGraphicsWidgetResource *p_resource_widget = qobject_cast<AbstractGraphicsWidgetResource *>(parentWidget());
	if(p_resource_widget) {
		p_resource_widget->TrimResource((qint64)(event->scenePos().x() + .5), (qint64)(event->lastScenePos().x() + .5), mPos);
		p_resource_widget->TrimHandleMoved(mPos);
	}
}

void AbstractGraphicsWidgetResource::TrimHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {

	AbstractGraphicsWidgetResource *p_resource_widget = qobject_cast<AbstractGraphicsWidgetResource *>(parentWidget());
	if(p_resource_widget) {
		if(event->button() == Qt::LeftButton) {
			mMousePressed = false;
			p_resource_widget->TrimHandleInUse(mPos, false);
		}
	}
}

void AbstractGraphicsWidgetResource::TrimHandle::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *widget /*= 0*/) {

	QRectF trim_rect(boundingRect());
	trim_rect.adjust(0, 2, 0, -2);
	QColor dark(0, 0, 0, 120);

	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(trim_rect.intersected(exposed_rect));
	if(mPos == Left) visible_rect.setLeft(boundingRect().left() + 3);
	else visible_rect.setRight(boundingRect().right() - 3);
	visible_rect.translate(-.5, -.5);
	if(visible_rect.isEmpty() == false) {
		AbstractGraphicsWidgetResource *p_resource_widget = qobject_cast<AbstractGraphicsWidgetResource *>(parentWidget());
		if(p_resource_widget) {
			if(boundingRect().intersects(pOption->exposedRect) && trim_rect.width() * 2 <= p_resource_widget->boundingRect().width() * GetViewTransform().m11()) {
				pPainter->fillRect(visible_rect, dark);
			}
		}
	}
}

void AbstractGraphicsWidgetResource::TrimHandle::ViewTransformEvent(const QTransform &rTransform) {

	if(mPos == Right) setTransform(QTransform::fromScale(1 / rTransform.m11(), 1).translate(-boundingRect().width() - 1, 0));
	else setTransform(QTransform::fromScale(1 / rTransform.m11(), 1));
}

QRectF AbstractGraphicsWidgetResource::TrimHandle::boundingRect() const {

	return mRect;
}

void AbstractGraphicsWidgetResource::TrimHandle::SetWidth(qreal width) {

	qreal old_width = boundingRect().width();
	prepareGeometryChange();
	mRect.setWidth(width);
	if(mPos == Right) {
		setTransform(transform().translate(old_width, 0));
		setTransform(transform().translate(-boundingRect().width(), 0));
	}
}

QGraphicsView* AbstractGraphicsWidgetResource::TrimHandle::GetObservableView() const {

	if(scene() && scene()->views().empty() == false) {
		return scene()->views().first();
	}
	return NULL;
}

QVariant AbstractGraphicsWidgetResource::TrimHandle::itemChange(GraphicsItemChange change, const QVariant &rValue) {

	if(change == QGraphicsItem::ItemSceneHasChanged) {
		ViewTransformEvent(GetViewTransform());
	}
	return QGraphicsItem::itemChange(change, rValue);
}

AbstractGraphicsWidgetResource::GraphicsItemDurationIndicator::GraphicsItemDurationIndicator(AbstractGraphicsWidgetResource *pParent) :
QGraphicsItem(pParent), mRect() {

	setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
}

QRectF AbstractGraphicsWidgetResource::GraphicsItemDurationIndicator::boundingRect() const {

	return mRect;
}

void AbstractGraphicsWidgetResource::GraphicsItemDurationIndicator::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= 0*/) {

	QRectF exposed_rect(pOption->exposedRect);
	if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
	if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
	QRectF visible_rect(boundingRect().intersected(exposed_rect));
	visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());
	// 	visible_rect.translate(.5 / pPainter->transform().m11(), .5 / pPainter->transform().m22());

	QPen pen;
	pen.setWidth(0); // cosmetic
	pPainter->setPen(pen);

	pen.setColor(QColor(CPL_COLOR_DURATION_INDICATOR));
	pPainter->setPen(pen);
	if(exposed_rect.top() <= boundingRect().top()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.topRight());
	if(exposed_rect.bottom() >= boundingRect().bottom()) pPainter->drawLine(visible_rect.bottomLeft(), visible_rect.bottomRight());
	if(exposed_rect.left() - 1 <= boundingRect().left()) pPainter->drawLine(visible_rect.topLeft(), QPointF(visible_rect.left(), visible_rect.top() + 20));
	if(exposed_rect.left() - 1 <= boundingRect().left()) pPainter->drawLine(visible_rect.bottomLeft(), QPointF(visible_rect.left(), visible_rect.bottom() - 20));
	if(exposed_rect.right() + 1 >= boundingRect().right()) pPainter->drawLine(visible_rect.topRight(), QPointF(visible_rect.right(), visible_rect.top() + 20));
	if(exposed_rect.right() + 1 >= boundingRect().right()) pPainter->drawLine(visible_rect.bottomRight(), QPointF(visible_rect.right(), visible_rect.bottom() - 20));
}

void AbstractGraphicsWidgetResource::GraphicsItemDurationIndicator::SetRect(const QRectF &rRect) {

	prepareGeometryChange();
	mRect = rRect;
}

GraphicsWidgetDummyResource::GraphicsWidgetDummyResource(GraphicsWidgetSequence *pParent, bool stretch /*= false*/) :
GraphicsWidgetFileResource(pParent, QSharedPointer<AssetMxfTrack>(NULL), QColor(CPL_COLOR_DUMMY_RESOURCE)), mStretch(stretch) {

	setEnabled(false);
	setAcceptHoverEvents(false);
	if(mStretch) setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	else setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

QSizeF GraphicsWidgetDummyResource::sizeHint(Qt::SizeHint which, const QSizeF &rConstraint /*= QSizeF()*/) const {

	if(mStretch) return QSizeF(-1, -1);
	else return QSizeF(0, -1);
}

void GraphicsWidgetDummyResource::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	QRectF resource_rect(boundingRect());
	resource_rect.setWidth(resource_rect.width() / GetRepeatCount());
	QPen pen;
	pen.setWidth(0); // cosmetic
	QBrush brush(Qt::SolidPattern);
	QColor color(GetColor());
	QColor light_color(GetColor().lighter(115));
	QColor dark_color(GetColor().darker(129));
	QColor light_color_border(GetColor().lighter(133));
	QColor dark_color_border(GetColor().darker(214));

	for(int i = 0; i < GetRepeatCount(); i++) {
		resource_rect.moveLeft(i * resource_rect.width());
		resource_rect = resource_rect.intersected(boundingRect());
		QRectF exposed_rect(pOption->exposedRect);
		if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
		if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
		QRectF visible_rect(resource_rect.intersected(exposed_rect));
		if(visible_rect.isEmpty() == true) continue;
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());

		if(acceptHoverEvents() == true && pOption->state & QStyle::State_MouseOver) {
			if(pOption->state & QStyle::State_Selected) {
				pen.setColor(dark_color);
				brush.setColor(dark_color);
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
				pen.setColor(dark_color);
				brush.setColor(dark_color);
			}
			else {
				pen.setColor(GetColor());
				brush.setColor(GetColor());
			}
			pPainter->setPen(pen);
			pPainter->setBrush(brush);
			pPainter->drawRect(visible_rect);
		}

		if(pOption->state & QStyle::State_Selected) pen.setColor(dark_color_border);
		else pen.setColor(light_color_border);
		pPainter->setPen(pen);
		if(exposed_rect.top() <= resource_rect.top()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.topRight());
		if(exposed_rect.left() <= resource_rect.left()) pPainter->drawLine(visible_rect.topLeft(), visible_rect.bottomLeft());
		if(pOption->state & QStyle::State_Selected) pen.setColor(light_color_border);
		else pen.setColor(dark_color_border);
		pPainter->setPen(pen);
		if(exposed_rect.bottom() >= resource_rect.bottom()) pPainter->drawLine(visible_rect.bottomLeft(), visible_rect.bottomRight());
		if(exposed_rect.right() >= resource_rect.right()) pPainter->drawLine(visible_rect.topRight(), visible_rect.bottomRight());
	}
}

GraphicsWidgetFileResource::GraphicsWidgetFileResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset /*= QSharedPointer<AssetMxfTrack>(NULL)*/, const QColor &rColor /*= QColor(Qt::white)*/) :
AbstractGraphicsWidgetResource(pParent, pResource, rAsset, rColor) {

}

GraphicsWidgetFileResource::GraphicsWidgetFileResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset, const QColor &rColor /*= QColor(Qt::white)*/) :
AbstractGraphicsWidgetResource(pParent,
new cpl::TrackFileResourceType(ImfXmlHelper::Convert(QUuid::createUuid()), rAsset ? rAsset->GetDuration().GetCount() : 0, ImfXmlHelper::Convert(QUuid::createUuid()) /*not used*/, rAsset ? ImfXmlHelper::Convert(rAsset->GetId()) : ImfXmlHelper::Convert(QUuid())),
rAsset, rColor) {

}

GraphicsWidgetFileResource* GraphicsWidgetFileResource::Clone() const {

	cpl::TrackFileResourceType intermediate_resource(*(static_cast<cpl::TrackFileResourceType*>(mpData)));
	intermediate_resource.setId(ImfXmlHelper::Convert(QUuid::createUuid()));
	return new GraphicsWidgetFileResource(NULL, intermediate_resource._clone(), mAssset, GetColor());
}

std::auto_ptr<cpl::BaseResourceType> GraphicsWidgetFileResource::Write() const {

	return std::auto_ptr<cpl::BaseResourceType>(mpData->_clone());
}


GraphicsWidgetVideoResource::GraphicsWidgetVideoResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset /*= QSharedPointer<AssetMxfTrack>(NULL)*/) :
GraphicsWidgetFileResource(pParent, pResource, rAsset, QColor(CPL_COLOR_VIDEO_RESOURCE)), mLeftProxyImage(":/proxy_film.png"), mRightProxyImage(":/proxy_film.png"), mTrimActive(false) {


	connect(this, SIGNAL(SourceDurationChanged(const Duration&, const Duration&)), this, SLOT(rSourceDurationChanged()));
	connect(this, SIGNAL(EntryPointChanged(const Duration&, const Duration&)), this, SLOT(rEntryPointChanged()));
	RefreshProxy();
}

GraphicsWidgetVideoResource::GraphicsWidgetVideoResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset) :
GraphicsWidgetFileResource(pParent, rAsset, QColor(CPL_COLOR_VIDEO_RESOURCE)), mLeftProxyImage(":/proxy_film.png"), mRightProxyImage(":/proxy_film.png"), mTrimActive(false) {

	connect(this, SIGNAL(SourceDurationChanged(const Duration&, const Duration&)), this, SLOT(rSourceDurationChanged()));
	connect(this, SIGNAL(EntryPointChanged(const Duration&, const Duration&)), this, SLOT(rEntryPointChanged()));
	RefreshProxy();
}

void GraphicsWidgetVideoResource::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	AbstractGraphicsWidgetResource::paint(pPainter, pOption, pWidget);

	const int offset = 20; //
	QRectF resource_rect(boundingRect());
	resource_rect.setWidth(resource_rect.width() / GetRepeatCount());
	QPen pen;
	pen.setWidth(0); // cosmetic
	pen.setColor(QColor(CPL_FONT_COLOR));
	pPainter->setPen(pen);
	pPainter->setFont(QFont());

	for(int i = 0; i < GetRepeatCount(); i++) {
		resource_rect.moveLeft(i * resource_rect.width());
		resource_rect = resource_rect.intersected(boundingRect());
		QRectF exposed_rect(pOption->exposedRect);
		if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
		if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
		QRectF visible_rect(resource_rect.intersected(exposed_rect));
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());
		if(visible_rect.isEmpty() == true) continue;

		QImage left_proxy_image = mLeftProxyImage.scaledToHeight(boundingRect().height() - 3);
		QImage right_proxy_image = mRightProxyImage.scaledToHeight(boundingRect().height() - 3);

		QTransform transf = pPainter->transform();

		QRectF left_proxy_image_rect(QPointF((resource_rect.left() * transf.m11() + offset) * 1 / transf.m11(), resource_rect.topLeft().y() + 1), QSizeF(left_proxy_image.width() * 1 / transf.m11(), left_proxy_image.height()));
		QRectF right_proxy_image_rect(QPointF((resource_rect.right() * transf.m11() - right_proxy_image.width() - offset - 1) * 1 / transf.m11(), resource_rect.topRight().y() + 1), QSizeF(right_proxy_image.width() * 1 / transf.m11(), right_proxy_image.height()));

		if(left_proxy_image_rect.right() <= right_proxy_image_rect.left()) {
			if(visible_rect.intersects(left_proxy_image_rect)) pPainter->drawImage(left_proxy_image_rect, left_proxy_image);
			if(visible_rect.intersects(right_proxy_image_rect))	pPainter->drawImage(right_proxy_image_rect, right_proxy_image);
		}
		else {
			left_proxy_image_rect.setWidth(0);
			right_proxy_image_rect.setLeft(right_proxy_image_rect.right());
		}

		QFontMetricsF font_metrics(pPainter->font());
		QRectF writable_rect(left_proxy_image_rect.topRight(), right_proxy_image_rect.bottomLeft());
		writable_rect.adjust(5 / transf.m11(), 0, -5 / transf.m11(), -2);

		if(writable_rect.isEmpty() == false) {

			QString duration(font_metrics.elidedText(tr("Dur.: %1").arg(MapToCplTimeline(GetSourceDuration()).GetCount()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString file_name;
			if(mAssset && mAssset->HasAffinity()) {
				if(i == 0) file_name = QString(font_metrics.elidedText(mAssset->GetOriginalFileName().first, Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
				else file_name = QString(font_metrics.elidedText(tr("[Duplicate]"), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			else {
				pen.setColor(Qt::white);
				pPainter->setPen(pen);
				file_name = QString(font_metrics.elidedText("Missing Asset", Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			QString cpl_out_point(font_metrics.elidedText(tr("Cpl Out: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i + 1) * GetSourceDuration() - 1)).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString cpl_in_point(font_metrics.elidedText(tr("Cpl In: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i * (GetSourceDuration())))).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			QString resource_out_point(font_metrics.elidedText(tr("Out: %1").arg(Timecode(GetEditRate(), GetEntryPoint() + GetSourceDuration() - 1).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString resource_in_point(font_metrics.elidedText(tr("In: %1").arg(Timecode(GetEditRate(), GetEntryPoint()).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top() + font_metrics.height())); // We have to use QTransform::translate() because of bug 192573.
			pPainter->drawText(QPointF(0, 0), file_name);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(duration)), writable_rect.top() + font_metrics.height()));
			pPainter->drawText(QPointF(0, 0), duration);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(cpl_out_point)), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_out_point);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(resource_out_point)), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_out_point);

			pPainter->setTransform(transf);
		}
	}
}

double GraphicsWidgetVideoResource::ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const {

	return 1;
}

void GraphicsWidgetVideoResource::rShowProxyImage(const QImage &rImage, const QVariant &rIdentifier /*= QVariant()*/) {

	if(rIdentifier.canConvert<Timecode>() == true) {
		Timecode timecode = qvariant_cast<Timecode>(rIdentifier);
		if(timecode == GetFirstVisibleFrame()) {
			mLeftProxyImage = rImage;
			update();
		}
		else if(timecode == GetLastVisibleFrame()) {
			mRightProxyImage = rImage;
			update();
		}
	}
}

GraphicsWidgetVideoResource* GraphicsWidgetVideoResource::Clone() const {

	cpl::TrackFileResourceType intermediate_resource(*(static_cast<cpl::TrackFileResourceType*>(mpData)));
	intermediate_resource.setId(ImfXmlHelper::Convert(QUuid::createUuid()));
	GraphicsWidgetVideoResource *p_resource = new GraphicsWidgetVideoResource(NULL, intermediate_resource._clone(), mAssset);
	p_resource->mLeftProxyImage = mLeftProxyImage;
	p_resource->mRightProxyImage = mRightProxyImage;
	p_resource->update();
	return p_resource;
}

void GraphicsWidgetVideoResource::rSourceDurationChanged() {

	if(mTrimActive == false) {
		RefreshSecondProxy();
	}
}

void GraphicsWidgetVideoResource::TrimHandleInUse(eTrimHandlePosition pos, bool active) {

	GraphicsWidgetFileResource::TrimHandleInUse(pos, active);
	mTrimActive = active;
	if(active == false) {
		switch(pos) {
			case AbstractGraphicsWidgetResource::Left:
				rEntryPointChanged();
				break;
			case AbstractGraphicsWidgetResource::Right:
				rSourceDurationChanged();
				break;
		}
	}
}

void GraphicsWidgetVideoResource::rEntryPointChanged() {

	if(mTrimActive == false) {
		RefreshFirstProxy();
	}
}

void GraphicsWidgetVideoResource::RefreshProxy() {

	RefreshFirstProxy();
	RefreshSecondProxy();
}

void GraphicsWidgetVideoResource::RefreshFirstProxy() {

}

void GraphicsWidgetVideoResource::RefreshSecondProxy() {

}

GraphicsWidgetAudioResource::GraphicsWidgetAudioResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset /*= QSharedPointer<AssetMxfTrack>(NULL)*/) :
GraphicsWidgetFileResource(pParent, pResource, rAsset, QColor(CPL_COLOR_AUDIO_RESOURCE)) {

}

GraphicsWidgetAudioResource::GraphicsWidgetAudioResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset) :
GraphicsWidgetFileResource(pParent, rAsset, QColor(CPL_COLOR_AUDIO_RESOURCE)) {

	if(mAssset && mAssset->GetEditRate().IsValid()) mpData->setEditRate(ImfXmlHelper::Convert(mAssset->GetEditRate()));
}

void GraphicsWidgetAudioResource::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	AbstractGraphicsWidgetResource::paint(pPainter, pOption, pWidget);

	const int offset = 20;
	QRectF resource_rect(boundingRect());
	resource_rect.setWidth(resource_rect.width() / GetRepeatCount());
	QPen pen;
	pen.setWidth(0); // cosmetic
	pen.setColor(QColor(CPL_FONT_COLOR));
	pPainter->setPen(pen);
	pPainter->setFont(QFont());

	for(int i = 0; i < GetRepeatCount(); i++) {
		resource_rect.moveLeft(i * resource_rect.width());
		resource_rect = resource_rect.intersected(boundingRect());
		QRectF exposed_rect(pOption->exposedRect);
		if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
		if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
		QRectF visible_rect(resource_rect.intersected(exposed_rect));
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());
		if(visible_rect.isEmpty() == true) continue;

		QTransform transf = pPainter->transform();

		QFontMetricsF font_metrics(pPainter->font());
		QRectF writable_rect(QPointF((resource_rect.left() * transf.m11() + offset) * 1 / transf.m11(), resource_rect.topLeft().y() + 1), QPointF((resource_rect.right() * transf.m11() - offset - 1) * 1 / transf.m11(), resource_rect.bottomRight().y() - 2));
		writable_rect.adjust(5 / transf.m11(), 0, -5 / transf.m11(), -2);

		if(writable_rect.isEmpty() == false) {

			QString duration(font_metrics.elidedText(tr("Dur.: %1").arg(MapToCplTimeline(GetSourceDuration()).GetCount()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString file_name;
			if(mAssset && mAssset->HasAffinity()) {
				if(i == 0) file_name = QString(font_metrics.elidedText(mAssset->GetOriginalFileName().first, Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
				else file_name = QString(font_metrics.elidedText(tr("[Duplicate]"), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			else {
				pen.setColor(Qt::white);
				pPainter->setPen(pen);
				file_name = QString(font_metrics.elidedText("Missing Asset", Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			QString cpl_out_point(font_metrics.elidedText(tr("Cpl Out: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i + 1) * (MapToCplTimeline(GetSourceDuration()).GetCount()*GetEditRate().GetQuotient()/GetCplEditRate().GetQuotient()/*this is necessary to prevent rounding error*/) - 1)).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString cpl_in_point(font_metrics.elidedText(tr("Cpl In: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i * (GetSourceDuration())))).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			QString resource_out_point(font_metrics.elidedText(tr("Out: %1").arg(Timecode(GetCplEditRate(), Duration(ceil(GetEntryPoint().GetCount() * GetCplEditRate().GetQuotient() / GetEditRate().GetQuotient())) + (GetSourceDuration().GetCount() * GetCplEditRate().GetQuotient() / GetEditRate().GetQuotient()) - 1).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString resource_in_point(font_metrics.elidedText(tr("In: %1").arg(Timecode(GetCplEditRate(), Duration(ceil(GetEntryPoint().GetCount() * GetCplEditRate().GetQuotient() / GetEditRate().GetQuotient()))).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top() + font_metrics.height())); // We have to use QTransform::translate() because of bug 192573.
			pPainter->drawText(QPointF(0, 0), file_name);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(duration)), writable_rect.top() + font_metrics.height()));
			pPainter->drawText(QPointF(0, 0), duration);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(cpl_out_point)), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_out_point);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(resource_out_point)), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_out_point);

			pPainter->setTransform(transf);
		}
	}
}

double GraphicsWidgetAudioResource::ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const {

	return GetEditRate().GetNumerator() * rCompositionEditRate.GetDenominator() / double(rCompositionEditRate.GetNumerator() * GetEditRate().GetDenominator());
}

GraphicsWidgetAudioResource* GraphicsWidgetAudioResource::Clone() const {

	cpl::TrackFileResourceType intermediate_resource(*(static_cast<cpl::TrackFileResourceType*>(mpData)));
	intermediate_resource.setId(ImfXmlHelper::Convert(QUuid::createUuid()));
	return new GraphicsWidgetAudioResource(NULL, intermediate_resource._clone(), mAssset);
}

SoundfieldGroup GraphicsWidgetAudioResource::GetSoundfieldGroup() const {

	if(mAssset) return mAssset->GetSoundfieldGroup();
	return SoundfieldGroup::SoundFieldGroupNone;
}

GraphicsWidgetTimedTextResource::GraphicsWidgetTimedTextResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset /*= QSharedPointer<AssetMxfTrack>(NULL)*/) :
GraphicsWidgetFileResource(pParent, pResource, rAsset, QColor(CPL_COLOR_TIMED_TEXT_RESOURCE)) {

}

GraphicsWidgetTimedTextResource::GraphicsWidgetTimedTextResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset) :
GraphicsWidgetFileResource(pParent, rAsset, QColor(CPL_COLOR_TIMED_TEXT_RESOURCE)) {

	if(mAssset) mpData->setEditRate(ImfXmlHelper::Convert(mAssset->GetEditRate()));

}

void GraphicsWidgetTimedTextResource::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	AbstractGraphicsWidgetResource::paint(pPainter, pOption, pWidget);

	const int offset = 20;
	QRectF resource_rect(boundingRect());
	resource_rect.setWidth(resource_rect.width() / GetRepeatCount());
	QPen pen;
	pen.setWidth(0); // cosmetic
	pen.setColor(QColor(CPL_FONT_COLOR));
	pPainter->setPen(pen);
	pPainter->setFont(QFont());

	for(int i = 0; i < GetRepeatCount(); i++) {
		resource_rect.moveLeft(i * resource_rect.width());
		resource_rect = resource_rect.intersected(boundingRect());
		QRectF exposed_rect(pOption->exposedRect);
		if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
		if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
		QRectF visible_rect(resource_rect.intersected(exposed_rect));
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());
		if(visible_rect.isEmpty() == true) continue;

		QTransform transf = pPainter->transform();

		QFontMetricsF font_metrics(pPainter->font());
		QRectF writable_rect(QPointF((resource_rect.left() * transf.m11() + offset) * 1 / transf.m11(), resource_rect.topLeft().y() + 1), QPointF((resource_rect.right() * transf.m11() - offset - 1) * 1 / transf.m11(), resource_rect.bottomRight().y() - 2));
		writable_rect.adjust(5 / transf.m11(), 0, -5 / transf.m11(), -2);

		if(writable_rect.isEmpty() == false) {

			QString duration(font_metrics.elidedText(tr("Dur.: %1").arg(MapToCplTimeline(GetSourceDuration()).GetCount()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString file_name;
			if(mAssset && mAssset->HasAffinity()) {
				if(i == 0) file_name = QString(font_metrics.elidedText(mAssset->GetOriginalFileName().first, Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
				else file_name = QString(font_metrics.elidedText(tr("[Duplicate]"), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			else {
				pen.setColor(Qt::white);
				pPainter->setPen(pen);
				file_name = QString(font_metrics.elidedText("Missing Asset", Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			QString cpl_out_point(font_metrics.elidedText(tr("Cpl Out: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i + 1) * (MapToCplTimeline(GetSourceDuration()).GetCount()*GetEditRate().GetQuotient()/GetCplEditRate().GetQuotient()/*this is necessary to prevent rounding error*/) - 1)).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString cpl_in_point(font_metrics.elidedText(tr("Cpl In: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i * (GetSourceDuration())))).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			QString resource_out_point(font_metrics.elidedText(tr("Out: %1").arg(Timecode(GetCplEditRate(), Duration(ceil(GetEntryPoint().GetCount() * GetCplEditRate().GetQuotient() / GetEditRate().GetQuotient())) + (GetSourceDuration().GetCount() * GetCplEditRate().GetQuotient() / GetEditRate().GetQuotient()) - 1).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString resource_in_point(font_metrics.elidedText(tr("In: %1").arg(Timecode(GetCplEditRate(), Duration(ceil(GetEntryPoint().GetCount() * GetCplEditRate().GetQuotient() / GetEditRate().GetQuotient()))).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top() + font_metrics.height())); // We have to use QTransform::translate() because of bug 192573.
			pPainter->drawText(QPointF(0, 0), file_name);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(duration)), writable_rect.top() + font_metrics.height()));
			pPainter->drawText(QPointF(0, 0), duration);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(cpl_out_point)), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_out_point);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(resource_out_point)), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_out_point);

			pPainter->setTransform(transf);
		}
	}
}

GraphicsWidgetTimedTextResource* GraphicsWidgetTimedTextResource::Clone() const {

	cpl::TrackFileResourceType intermediate_resource(*(static_cast<cpl::TrackFileResourceType*>(mpData)));
	intermediate_resource.setId(ImfXmlHelper::Convert(QUuid::createUuid()));
	return new GraphicsWidgetTimedTextResource(NULL, intermediate_resource._clone(), mAssset);
}

double GraphicsWidgetTimedTextResource::ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const {

	return GetEditRate().GetNumerator() * rCompositionEditRate.GetDenominator() / double(rCompositionEditRate.GetNumerator() * GetEditRate().GetDenominator());
}

GraphicsWidgetAncillaryDataResource::GraphicsWidgetAncillaryDataResource(GraphicsWidgetSequence *pParent, cpl::TrackFileResourceType *pResource, const QSharedPointer<AssetMxfTrack> &rAsset /*= QSharedPointer<AssetMxfTrack>(NULL)*/) :
GraphicsWidgetFileResource(pParent, pResource, rAsset, QColor(CPL_COLOR_ANC_RESOURCE)) {

}

GraphicsWidgetAncillaryDataResource::GraphicsWidgetAncillaryDataResource(GraphicsWidgetSequence *pParent, const QSharedPointer<AssetMxfTrack> &rAsset) :
GraphicsWidgetFileResource(pParent, rAsset, QColor(CPL_COLOR_ANC_RESOURCE)) {

	if(mAssset && mAssset->GetEditRate().IsValid()) mpData->setEditRate(ImfXmlHelper::Convert(mAssset->GetEditRate()));
}

void GraphicsWidgetAncillaryDataResource::paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget /*= NULL*/) {

	AbstractGraphicsWidgetResource::paint(pPainter, pOption, pWidget);

	const int offset = 20;
	QRectF resource_rect(boundingRect());
	resource_rect.setWidth(resource_rect.width() / GetRepeatCount());
	QPen pen;
	pen.setWidth(0); // cosmetic
	pen.setColor(QColor(CPL_FONT_COLOR));
	pPainter->setPen(pen);
	pPainter->setFont(QFont());

	for(int i = 0; i < GetRepeatCount(); i++) {
		resource_rect.moveLeft(i * resource_rect.width());
		resource_rect = resource_rect.intersected(boundingRect());
		QRectF exposed_rect(pOption->exposedRect);
		if(exposed_rect.left() - 1 >= boundingRect().left()) exposed_rect.adjust(-1, 0, 0, 0);
		if(exposed_rect.right() + 1 <= boundingRect().right()) exposed_rect.adjust(0, 0, 1, 0);
		QRectF visible_rect(resource_rect.intersected(exposed_rect));
		visible_rect.adjust(0, 0, -1. / pPainter->transform().m11(), -1. / pPainter->transform().m22());
		if(visible_rect.isEmpty() == true) continue;

		QTransform transf = pPainter->transform();

		QFontMetricsF font_metrics(pPainter->font());
		QRectF writable_rect(QPointF((resource_rect.left() * transf.m11() + offset) * 1 / transf.m11(), resource_rect.topLeft().y() + 1), QPointF((resource_rect.right() * transf.m11() - offset - 1) * 1 / transf.m11(), resource_rect.bottomRight().y() - 2));
		writable_rect.adjust(5 / transf.m11(), 0, -5 / transf.m11(), -2);

		if(writable_rect.isEmpty() == false) {

			QString duration(font_metrics.elidedText(tr("Dur.: %1").arg(MapToCplTimeline(GetSourceDuration()).GetCount()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString file_name;
			if(mAssset && mAssset->HasAffinity()) {
				if(i == 0) file_name = QString(font_metrics.elidedText(mAssset->GetOriginalFileName().first, Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
				else file_name = QString(font_metrics.elidedText(tr("[Duplicate]"), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			else {
				pen.setColor(Qt::white);
				pPainter->setPen(pen);
				file_name = QString(font_metrics.elidedText("Missing Asset", Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(duration)));
			}
			QString cpl_out_point(font_metrics.elidedText(tr("Cpl Out: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i + 1) * GetSourceDuration() - 1)).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString cpl_in_point(font_metrics.elidedText(tr("Cpl In: %1").arg(MapToCplTimeline(Timecode(GetEditRate(), GetEntryPoint() + (i * (GetSourceDuration())))).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			QString resource_out_point(font_metrics.elidedText(tr("Out: %1").arg(Timecode(GetEditRate(), GetEntryPoint() +  GetSourceDuration() - 1).GetAsString()), Qt::ElideLeft, writable_rect.width() * transf.m11()));
			QString resource_in_point(font_metrics.elidedText(tr("In: %1").arg(Timecode(GetEditRate(), GetEntryPoint()).GetAsString()), Qt::ElideRight, writable_rect.width() * transf.m11() - font_metrics.width(cpl_out_point)));

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top() + font_metrics.height())); // We have to use QTransform::translate() because of bug 192573.
			pPainter->drawText(QPointF(0, 0), file_name);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(duration)), writable_rect.top() + font_metrics.height()));
			pPainter->drawText(QPointF(0, 0), duration);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(cpl_out_point)), writable_rect.bottom()));
			pPainter->drawText(QPointF(0, 0), cpl_out_point);

			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate(writable_rect.left() * transf.m11(), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_in_point);
			pPainter->setTransform(QTransform(transf).scale(1 / transf.m11(), 1).translate((writable_rect.right() * transf.m11() - font_metrics.width(resource_out_point)), writable_rect.top()));
			pPainter->drawText(QPointF(0, 35), resource_out_point);

			pPainter->setTransform(transf);
		}
	}
}

GraphicsWidgetAncillaryDataResource* GraphicsWidgetAncillaryDataResource::Clone() const {

	cpl::TrackFileResourceType intermediate_resource(*(static_cast<cpl::TrackFileResourceType*>(mpData)));
	intermediate_resource.setId(ImfXmlHelper::Convert(QUuid::createUuid()));
	return new GraphicsWidgetAncillaryDataResource(NULL, intermediate_resource._clone(), mAssset);
}

double GraphicsWidgetAncillaryDataResource::ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const {

	return GetEditRate().GetNumerator() * rCompositionEditRate.GetDenominator() / double(rCompositionEditRate.GetNumerator() * GetEditRate().GetDenominator());
}

GraphicsWidgetMarkerResource::GraphicsWidgetMarkerResource(GraphicsWidgetSequence *pParent, cpl::MarkerResourceType *pResource) :
AbstractGraphicsWidgetResource(pParent, pResource, QSharedPointer<AssetMxfTrack>(NULL), QColor(CPL_COLOR_MARKER_RESOURCE)), mActiveMarkerOldPosition(-1, -1),
mOldSourceDuration(-1), mOldIntrinsicDuration(-1) {

	setFlag(QGraphicsItem::ItemClipsChildrenToShape);
	DisableTrimHandle(AbstractGraphicsWidgetResource::Left, true);
	DisableTrimHandle(AbstractGraphicsWidgetResource::Right, true);
	setAcceptHoverEvents(false);
	InitMarker();
}

GraphicsWidgetMarkerResource::GraphicsWidgetMarkerResource(GraphicsWidgetSequence *pParent) :
AbstractGraphicsWidgetResource(pParent,
new cpl::MarkerResourceType(ImfXmlHelper::Convert(QUuid::createUuid()), pParent && pParent->GetSegment() ? pParent->GetSegment()->GetDuration().GetCount() : 1),
QSharedPointer<AssetMxfTrack>(NULL), QColor(CPL_COLOR_MARKER_RESOURCE)), mActiveMarkerOldPosition(-1, -1),
mOldSourceDuration(-1), mOldIntrinsicDuration(-1) {

	setFlag(QGraphicsItem::ItemClipsChildrenToShape);
	DisableTrimHandle(AbstractGraphicsWidgetResource::Left, true);
	DisableTrimHandle(AbstractGraphicsWidgetResource::Right, true);
	setAcceptHoverEvents(false);
}

std::auto_ptr<cpl::BaseResourceType> GraphicsWidgetMarkerResource::Write() const {

	cpl::MarkerResourceType *p_marker_resource = static_cast<cpl::MarkerResourceType*>(mpData->_clone());
	cpl::MarkerResourceType::MarkerSequence marker_sequence;
	QList<QGraphicsItem*> child_items = childItems();
	for(int i = 0; i < child_items.size(); i++) {
		GraphicsWidgetMarker *p_marker = dynamic_cast<GraphicsWidgetMarker*>(child_items.at(i));
		if(p_marker) {
			cpl::MarkerType marker(ImfXmlHelper::Convert(p_marker->GetMarkerLabel()), p_marker->pos().x() * ResourceErPerCompositionEr(GetCplEditRate()));
			if(p_marker->GetAnnotation().IsEmpty() == false) marker.setAnnotation(ImfXmlHelper::Convert(p_marker->GetAnnotation()));
			marker_sequence.push_back(marker);
		}
	}
	p_marker_resource->setMarker(marker_sequence);
	return std::auto_ptr<cpl::BaseResourceType>(p_marker_resource);
}

GraphicsWidgetMarkerResource* GraphicsWidgetMarkerResource::Clone() const {

	std::auto_ptr<cpl::BaseResourceType> intermediate = Write();
	intermediate->setId(ImfXmlHelper::Convert(QUuid::createUuid()));
	return new GraphicsWidgetMarkerResource(NULL, static_cast<cpl::MarkerResourceType*>(intermediate.release()));
}

double GraphicsWidgetMarkerResource::ResourceErPerCompositionEr(const EditRate &rCompositionEditRate) const {

	return GetEditRate().GetNumerator() * rCompositionEditRate.GetDenominator() / double(rCompositionEditRate.GetNumerator() * GetEditRate().GetDenominator());
}

void GraphicsWidgetMarkerResource::InitMarker() {

	cpl::MarkerResourceType *p_marker_resource = static_cast<cpl::MarkerResourceType*>(mpData);
	const cpl::MarkerResourceType::MarkerSequence &r_marker_sequence = p_marker_resource->getMarker();
	for(unsigned int i = 0; i < r_marker_sequence.size(); i++) {
		GraphicsWidgetMarker *p_marker = new GraphicsWidgetMarker(this, 1, boundingRect().height(), ImfXmlHelper::Convert(r_marker_sequence.at(i).getLabel()), QColor(CPL_COLOR_DEFAULT_MARKER));
		p_marker->setPos((qint64)(r_marker_sequence.at(i).getOffset() / ResourceErPerCompositionEr(GetCplEditRate())), 1);
	}
}

void GraphicsWidgetMarkerResource::contextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent) {

	QMenu menu;
	QMenu sub_menu;
	sub_menu.setIcon(QIcon(":/marker.png"));
	sub_menu.setTitle(tr("&Add Marker"));
	QAction *p_sub_menu_action = menu.addMenu(&sub_menu);
	QStringList marker = MarkerLabel::GetMarkerLabels();
	for(int i = 0; i < marker.size(); i++) {
		QAction *p_add_marker_action = sub_menu.addAction(marker.at(i));
		p_add_marker_action->setToolTip(MarkerLabel::GetMarker(marker.at(i)).GetDescription());
	}
	GraphicsObjectVerticalIndicator *p_marker = NULL;
	QList<QGraphicsItem*> items = scene()->items(pEvent->scenePos());
	for(int i = 0; i < items.size(); i++) {
		p_marker = dynamic_cast<GraphicsObjectVerticalIndicator*>(items.at(i));
		if(p_marker != NULL) break;
	}
	QAction *p_delete_marker_action = NULL;
	if(p_marker != NULL) p_delete_marker_action = menu.addAction(QIcon(":/delete.png"), tr("&Remove Marker"));
	QAction *p_selected_action = menu.exec(pEvent->screenPos());

	if(p_selected_action) {
		if(p_delete_marker_action && p_selected_action == p_delete_marker_action) {
			if(GraphicsSceneComposition* p_scene = qobject_cast<GraphicsSceneComposition*>(scene())) p_scene->DelegateCommand(new RemoveMarkerCommand(p_marker, this));
			else qWarning() << "Couldn't delegate remove marker command.";
		}
		else {
			MarkerLabel label = MarkerLabel::GetMarker(p_selected_action->text());
			if(label.IsWellKnown()) {
				GraphicsWidgetMarker *p_marker = new GraphicsWidgetMarker(this, 1, boundingRect().height(), label, QColor(CPL_COLOR_DEFAULT_MARKER));
				if(GraphicsSceneComposition* p_scene = qobject_cast<GraphicsSceneComposition*>(scene())) p_scene->DelegateCommand(new AddMarkerCommand(p_marker, QPointF((qint64)(pEvent->pos().x()), 1), this));
				else qWarning() << "Couldn't delegate add marker command.";
			}
		}
	}
}

void GraphicsWidgetMarkerResource::MoveMarker(GraphicsWidgetMarker *pMarker, qint64 pos, qint64 lastPos) {

	double samples_factor = ResourceErPerCompositionEr(GetCplEditRate());
	if(pMarker) {
		QPointF local_pos = mapFromScene(pos, 0);
		if(local_pos.x() < boundingRect().left()) local_pos.setX(boundingRect().left());

		//WR
		// We don't allow to change the intrinsic duration of marker segments
		if(local_pos.x() > boundingRect().right()) local_pos.setX(boundingRect().right());
		//WR
		qint64 max_offset = local_pos.x();
		QList<QGraphicsItem*> items = childItems();
		for(int i = 0; i < items.size(); i++) {
			GraphicsWidgetMarker *p_marker = dynamic_cast<GraphicsWidgetMarker*>(items.at(i));
			if(p_marker && p_marker != pMarker) {
				qint64 offset = p_marker->pos().x();
				if(offset > max_offset) max_offset = offset;
			}
		}
		Duration new_source_duration = max_offset * samples_factor;
		//WR
		// Commented out, because the else block has reduced the duration of the marker sequence,
		// which in turn created holes in the virtual marker track timeline
		// As a consequence, also the first block has been commented out
		// TODO Create event when the total duration of a video/audio/tt segment changes and adjust marker segment duration accordingly
		/*if(new_source_duration > GetSourceDuration()) {
			SetIntrinsicDuaration(new_source_duration + GetEntryPoint());
			SetSourceDuration(new_source_duration);
		}
		else {
			//SetSourceDuration(new_source_duration);
			//SetIntrinsicDuaration(new_source_duration + GetEntryPoint());
		}*/
		//WR
		pMarker->setPos(local_pos.x(), 1);
	}
}

void GraphicsWidgetMarkerResource::CplEditRateChanged() {

	AbstractGraphicsWidgetResource::CplEditRateChanged();
	QList<QGraphicsItem*> items = childItems();
	for(int i = 0; i < items.size(); i++) {
		GraphicsWidgetMarker *p_marker = dynamic_cast<GraphicsWidgetMarker*>(items.at(i));
		if(p_marker) {
			p_marker->deleteLater();
		}
	}
	InitMarker();
}

void GraphicsWidgetMarkerResource::MarkerInUse(GraphicsWidgetMarker *pMarker, bool active) {

	if(pMarker) {
		if(active == true) {
			mActiveMarkerOldPosition = pMarker->pos();
			mOldSourceDuration = GetSourceDuration();
			mOldIntrinsicDuration = GetIntrinsicDuration();
		}
		else {
			if(mActiveMarkerOldPosition != pMarker->pos()) {
				QUndoCommand *p_root = new QUndoCommand(NULL);
				new MoveMarkerCommand(pMarker, pMarker->pos(), mActiveMarkerOldPosition, p_root);
				if(mOldSourceDuration > GetSourceDuration()) {
					new SetSourceDurationCommand(this, mOldSourceDuration, GetSourceDuration(), p_root);
					new SetIntrinsicDurationCommand(this, mOldIntrinsicDuration, GetIntrinsicDuration(), p_root);
				}
				else {
					new SetIntrinsicDurationCommand(this, mOldIntrinsicDuration, GetIntrinsicDuration(), p_root);
					new SetSourceDurationCommand(this, mOldSourceDuration, GetSourceDuration(), p_root);
				}
				if(GraphicsSceneComposition* p_scene = qobject_cast<GraphicsSceneComposition*>(scene())) p_scene->DelegateCommand(p_root);
				else {
					qWarning() << "Couldn't delegate move marker command.";
					delete p_root;
				}
			}
			mActiveMarkerOldPosition = QPointF(-1, -1);
			mOldSourceDuration = Duration(-1);
			mOldIntrinsicDuration = Duration(-1);
		}
	}
}

void GraphicsWidgetMarkerResource::SetIntrinsicDuaration(const Duration &rIntrinsicDuration) {

	Duration new_intrinsic_duration(rIntrinsicDuration);
	Duration current_intrinsic_duration(GetIntrinsicDuration());
	if(new_intrinsic_duration < GetEntryPoint() + GetSourceDuration()) new_intrinsic_duration = GetEntryPoint() + GetSourceDuration();
	mpData->setIntrinsicDuration(xml_schema::NonNegativeInteger(new_intrinsic_duration.GetCount()));
}

void GraphicsWidgetMarkerResource::resizeEvent(QGraphicsSceneResizeEvent *pEvent) {


}

GraphicsWidgetMarkerResource::GraphicsWidgetMarker::GraphicsWidgetMarker(GraphicsWidgetMarkerResource *pParent, qreal width, qreal height, const MarkerLabel &rLabel, const QColor &rColor) :
GraphicsObjectVerticalIndicator(width, height, rColor, pParent), mAnnotation(), mLabel(rLabel) {

	ShowHead();
	HideLine();
	setAcceptHoverEvents(true);
	EnableGridExtension(true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void GraphicsWidgetMarkerResource::GraphicsWidgetMarker::mousePressEvent(QGraphicsSceneMouseEvent *pEvent) {

	setZValue(1);
	GraphicsObjectVerticalIndicator::mousePressEvent(pEvent);
	GraphicsWidgetMarkerResource *p_parent = qobject_cast<GraphicsWidgetMarkerResource*>(parentWidget());
	if(p_parent) {
		if(pEvent->button() == Qt::LeftButton) {
			QToolTip::showText(pEvent->screenPos(), "");
			p_parent->MarkerInUse(this, true);
			p_parent->MoveMarker(this, (qint64)(pEvent->scenePos().x() + .5), (qint64)(pEvent->lastScenePos().x() + .5));
		}
	}
}

void GraphicsWidgetMarkerResource::GraphicsWidgetMarker::mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsObjectVerticalIndicator::mouseMoveEvent(pEvent);
	if(!(pEvent->buttons() & Qt::LeftButton)) return;
	GraphicsWidgetMarkerResource *p_parent = qobject_cast<GraphicsWidgetMarkerResource*>(parentWidget());
	if(p_parent) {
		p_parent->MoveMarker(this, (qint64)(pEvent->scenePos().x() + .5), (qint64)(pEvent->lastScenePos().x() + .5));
	}
}

void GraphicsWidgetMarkerResource::GraphicsWidgetMarker::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsObjectVerticalIndicator::mouseReleaseEvent(pEvent);
	GraphicsWidgetMarkerResource *p_parent = qobject_cast<GraphicsWidgetMarkerResource*>(parentWidget());
	if(p_parent) {
		if(pEvent->button() == Qt::LeftButton) {
			p_parent->MarkerInUse(this, false);
		}
	}
	setZValue(0);
}

void GraphicsWidgetMarkerResource::GraphicsWidgetMarker::hoverEnterEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	QString tool_tip(QString("[%1]").arg(mLabel.GetLabel()));
	if(mLabel.IsWellKnown()) tool_tip.append(QString("\n%1").arg(mLabel.GetDescription()));
	QToolTip::showText(pEvent->screenPos(), tool_tip);
}

void GraphicsWidgetMarkerResource::GraphicsWidgetMarker::hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent) {

	update();
	QToolTip::showText(pEvent->screenPos(), "");
}
