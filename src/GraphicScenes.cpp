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
#include "GraphicScenes.h"
#include "GraphicsCommon.h"
#include "CompositionPlaylistCommands.h"
#include "GraphicsWidgetTimeline.h"
#include "GraphicsWidgetResources.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetSequence.h"
#include "Events.h"
#include "ImfMimeData.h"
#include "ImfPackage.h"
#include <limits>
#include <QPair>
#include <QStatusBar>
#include <QGraphicsSceneMouseEvent>


GraphicsSceneBase::GraphicsSceneBase(const EditRate &rCplEditRate, QObject *pParent /*= NULL*/) :
QGraphicsScene(pParent), mSnapWidth(16), mCplEditRate(rCplEditRate) {

	connect(this, SIGNAL(sceneRectChanged(const QRectF&)), this, SLOT(rSceneRectChanged(const QRectF&)));
}

GraphicsSceneBase::GridInfo GraphicsSceneBase::SnapToGrid(const QPointF &rPoint, GridPosition which, const QRectF &rSearchRect /*= QRectF()*/, QList<AbstractGridExtension*> ignoreItems /*= QList<AbstractGridExtension*>()*/) const {

	QGraphicsView *p_view(NULL);
	QRectF search_rect(rSearchRect);
	if(views().isEmpty() == false) p_view = views().first();
	QRectF snap_rect(0, 0, mSnapWidth, sceneRect().height());
	if(p_view) snap_rect.setWidth(snap_rect.width() / p_view->transform().m11());
	snap_rect.moveCenter(QPointF(rPoint.x(), snap_rect.center().y()));
	if(search_rect.isEmpty() == true) search_rect = snap_rect;
	QList<AbstractGridExtension*> grid_items(AddPermanentSnapItems());
	QList<QGraphicsItem*> graphics_items = items(search_rect.intersected(snap_rect), Qt::IntersectsItemBoundingRect);
	for(int i = 0; i < graphics_items.size(); i++) grid_items.push_back(dynamic_cast<AbstractGridExtension*>(graphics_items.at(i)));
	GridInfo ret;
	ret.SnapPos = rPoint;
	ret.IsHoizontalSnap = false;
	ret.IsVerticalSnap = false;
	ret.HorizontalOrigin = NULL;
	ret.VerticalOrigin = NULL;
	ret.HeightAdvice = -1;
	ret.ColorAdvice = QColor();
	QList<QPair<qreal, AbstractGridExtension*> > vertical_sigularities;
	QList<QPair<qreal, AbstractGridExtension*> > horizontal_sigularities;
	for(int i = 0; i < grid_items.size(); i++) {
		AbstractGridExtension *p_base_graphics_widget = grid_items.at(i);
		if(p_base_graphics_widget) {
			if(ignoreItems.contains(p_base_graphics_widget) == true) continue;
			bool use = false;
			if(which & Vertical) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, Vertical);
				if(use && snap_rect.contains(proposed_point)) vertical_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.x(), p_base_graphics_widget));
			}
			if(which & VideoHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, VideoHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & AudioHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, AudioHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & TimedTextHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, TimedTextHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & DataHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, DataHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & ISXDHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, ISXDHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & IABHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, IABHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & SADMHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, SADMHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
			if(which & ADMHorizontal) {
				QPointF proposed_point(rPoint);
				use = p_base_graphics_widget->ExtendGrid(proposed_point, ADMHorizontal);
				if(use && snap_rect.contains(proposed_point)) horizontal_sigularities.push_back(QPair<qreal, AbstractGridExtension*>(proposed_point.y(), p_base_graphics_widget));
			}
		}
	}
	AbstractGridExtension *p_final_snap_item_vertical = NULL;
	qreal min_distance_vertical = std::numeric_limits<qreal>::max();
	for(int i = 0; i < vertical_sigularities.size(); i++) {
		qreal distance = std::abs((int)(rPoint.x() - vertical_sigularities.at(i).first));
		if(distance < min_distance_vertical) {
			min_distance_vertical = distance;
			ret.SnapPos.setX(vertical_sigularities.at(i).first);
			ret.IsVerticalSnap = true;
			p_final_snap_item_vertical = vertical_sigularities.at(i).second;
		}
		else if(distance == min_distance_vertical) {
			if(QGraphicsItem *p_item = dynamic_cast<QGraphicsItem*>(vertical_sigularities.at(i).second)) {
				QPointF probe = p_item->mapFromScene(rPoint.x(), 0);
				if(p_item->contains(QPointF(probe.x(), p_item->boundingRect().center().y()))) {
					min_distance_vertical = distance;
					ret.SnapPos.setX(vertical_sigularities.at(i).first);
					ret.IsVerticalSnap = true;
					p_final_snap_item_vertical = vertical_sigularities.at(i).second;
				}
			}
		}
	}
	AbstractGridExtension *p_final_snap_item_horizontal = NULL;
	qreal min_distance_horizontal = std::numeric_limits<qreal>::max();
	for(int i = 0; i < horizontal_sigularities.size(); i++) {
		qreal distance = std::abs((int)(rPoint.y() - horizontal_sigularities.at(i).first));
		if(distance < min_distance_horizontal) {
			min_distance_horizontal = distance;
			ret.SnapPos.setY(horizontal_sigularities.at(i).first);
			ret.IsHoizontalSnap = true;
			p_final_snap_item_horizontal = horizontal_sigularities.at(i).second;
		}
		else if(distance == min_distance_horizontal) {
			if(QGraphicsItem *p_item = dynamic_cast<QGraphicsItem*>(horizontal_sigularities.at(i).second)) {
				QPointF probe = p_item->mapFromScene(0, rPoint.y());
				if(p_item->contains(QPointF(p_item->boundingRect().center().x(), probe.y()))) {
					min_distance_horizontal = distance;
					ret.SnapPos.setY(horizontal_sigularities.at(i).first);
					ret.IsHoizontalSnap = true;
					p_final_snap_item_horizontal = horizontal_sigularities.at(i).second;
				}
			}
		}
	}
	if(p_final_snap_item_vertical) {
		ret.ColorAdvice = p_final_snap_item_vertical->ColorAdviceForGrid();
		ret.VerticalOrigin = p_final_snap_item_vertical;
	}
	if(p_final_snap_item_horizontal) {
		ret.HeightAdvice = p_final_snap_item_horizontal->HeightAdviceForHorizontalGrid();
		ret.HorizontalOrigin = p_final_snap_item_horizontal;
	}
	return ret;
}

GraphicsSceneBase::GridInfo GraphicsSceneBase::SnapToGrid(const QPointF &rPoint, GridPosition which, const QRectF &rSearchRect /*= QRectF()*/, AbstractGridExtension *pIgnoreItem /*= NULL*/) const {

	QList<AbstractGridExtension*> ignore_list;
	ignore_list << pIgnoreItem;
	return SnapToGrid(rPoint, which, rSearchRect, ignore_list);
}

void GraphicsSceneBase::SetCplEditRate(const EditRate &rCplEditRate) {

	QList<QGraphicsItem*> items_list = items();
	for(int i = 0; i < items_list.size(); i++) {
		QGraphicsObject *p_object = dynamic_cast<QGraphicsObject*>(items_list.at(i));
		if(p_object) {
			EventCplEditRateChange cpl_change(mCplEditRate, rCplEditRate);
			QCoreApplication::sendEvent(p_object, &cpl_change);
		}
	}
	mCplEditRate = rCplEditRate;
	update();
}

void GraphicsSceneBase::mousePressEvent(QGraphicsSceneMouseEvent *pEvent) {

	QGraphicsScene::mousePressEvent(pEvent);
	emit ClearSelectionRequest();
}

GraphicsSceneComposition::GraphicsSceneComposition(const EditRate &rCplEditRate /*= EditRate::EditRate24*/, QObject *pParent /*= NULL*/) :
GraphicsSceneBase(rCplEditRate, pParent), mpComposition(NULL), mpGhost(NULL), mpSnapIndicator(NULL), mpInsertIndicatorTop(NULL), mpInsertIndicatorBottom(NULL), mpCurrentFrameIndicator(NULL), mDropInfo(), mDragActive(false) {

	mpComposition = new GraphicsWidgetComposition();
	addItem(mpComposition);
	mpSnapIndicator = new GraphicsObjectVerticalIndicator(1, sceneRect().height(), QColor(CPL_COLOR_DEFAULT_SNAP_INDICATOR), NULL);
	addItem(mpSnapIndicator);
	mpSnapIndicator->setZValue(2);
	mpSnapIndicator->hide();
	mpInsertIndicatorTop = new GraphicsObjectVerticalIndicator(1, 12, QColor(CPL_COLOR_DEFAULT_SNAP_INDICATOR), NULL);
	mpInsertIndicatorTop->HideLine();
	addItem(mpInsertIndicatorTop);
	mpInsertIndicatorTop->setZValue(2);
	mpInsertIndicatorTop->hide();
	QPolygonF polygon;
	polygon << QPointF(0, 0.5) << QPointF(6, 6.5) << QPointF(0, 12.5);
	QGraphicsPolygonItem *p_polygon_item = new QGraphicsPolygonItem(polygon, mpInsertIndicatorTop);
	p_polygon_item->setPen(QPen(QColor(Qt::white)));
	p_polygon_item->setBrush(QBrush(QColor(Qt::white)));
	mpInsertIndicatorBottom = new GraphicsObjectVerticalIndicator(1, 12, QColor(CPL_COLOR_DEFAULT_SNAP_INDICATOR), NULL);
	mpInsertIndicatorBottom->HideLine();
	addItem(mpInsertIndicatorBottom);
	mpInsertIndicatorBottom->setZValue(2);
	mpInsertIndicatorBottom->hide();
	QGraphicsPolygonItem *p_polygon_item_second = new QGraphicsPolygonItem(polygon, mpInsertIndicatorBottom);
	p_polygon_item_second->setPen(QPen(QColor(Qt::white)));
	p_polygon_item_second->setBrush(QBrush(QColor(Qt::white)));
	mpCurrentFrameIndicator = new GraphicsObjectVerticalIndicator(2, sceneRect().height(), QColor(CPL_COLOR_CURRENT_FRAME_INDICATOR), NULL);
	addItem(mpCurrentFrameIndicator);
	mpCurrentFrameIndicator->EnableGridExtension(true);
	mpCurrentFrameIndicator->setZValue(2);

	connect(mpComposition, SIGNAL(geometryChanged()), this, SLOT(rCompositionGeometryChanged()));
}

void GraphicsSceneComposition::mousePressEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsSceneBase::mousePressEvent(pEvent);
	if(pEvent->button() == Qt::LeftButton) {
		AbstractGraphicsWidgetResource *p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(mouseGrabberItem());
		if(p_resource) {
			int offset_left = p_resource->mapFromScene(pEvent->scenePos().toPoint()).x();
			int offset_top = p_resource->mapFromScene(pEvent->scenePos().toPoint()).y();
			ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(offset_left, offset_top));
		}
	}
	else {
		mDragActive = false;
		ProcessCleanUp(mDropInfo);
	}
}

void GraphicsSceneComposition::mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsSceneBase::mouseMoveEvent(pEvent);
	if(!(pEvent->buttons() == Qt::LeftButton)) return;
	if((pEvent->screenPos() - pEvent->buttonDownScreenPos(Qt::LeftButton)).manhattanLength() >= QApplication::startDragDistance()) mDragActive = true;
	if(mDragActive) {
		ProcessDragMove(mDropInfo, pEvent->scenePos().toPoint(), !(pEvent->modifiers() & Qt::ShiftModifier), (pEvent->modifiers() & Qt::ControlModifier ? InsertMode : OverwriteMode));
	}
}

void GraphicsSceneComposition::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsSceneBase::mouseReleaseEvent(pEvent);
	if(pEvent->button() == Qt::LeftButton) {
		mDragActive = false;
		ProcessDrop(mDropInfo);
		ProcessCleanUp(mDropInfo);
	}
}

void GraphicsSceneComposition::ProcessInitDrag(DragDropInfo &rInfo, AbstractGraphicsWidgetResource *pOriginResource, const QPoint &rScenePos, const QPoint &rHotspot) {

	rInfo.state = DragDropInfo::InitDrag;
	rInfo.isDropable = false;
	rInfo.mode = OverwriteMode;
	rInfo.isSnapToGridMode = false;
	rInfo.dropSucceeded = false;
	rInfo.lastScenePos = rScenePos;
	rInfo.hotspot = rHotspot;
	rInfo.pOriginResource = pOriginResource;
	rInfo.pSegment = NULL;
	if(rInfo.pOriginResource && rInfo.state == DragDropInfo::InitDrag) {
		delete mpGhost;
		mpGhost = rInfo.pOriginResource->Clone();
		mpGhost->hide();
		mpGhost->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
		addItem(mpGhost);
		mpGhost->setOpacity(.9);
		mpGhost->SetColor(rInfo.pOriginResource->GetColor().darker(150));
		mpGhost->setEnabled(false);
		mpGhost->setZValue(1);
		QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
		rInfo.state = DragDropInfo::DragMove;
	}
}

void GraphicsSceneComposition::ProcessDragMove(DragDropInfo &rInfo, const QPoint &rScenePos, bool snapToGrid, eDragMode mode) {

	if(rInfo.state == DragDropInfo::DragMove) {
		rInfo.lastScenePos = rScenePos;
		rInfo.isDropable = false;
		rInfo.mode = mode;
		rInfo.isSnapToGridMode = snapToGrid;
		rInfo.pSegment = NULL;
		mpGhost->show();
		AbstractGraphicsWidgetResource *p_origin_resource = rInfo.pOriginResource;
		if(p_origin_resource->type() != GraphicsWidgetDummyResourceType && p_origin_resource->type() != GraphicsWidgetMarkerResourceType && p_origin_resource->type() != GraphicsWidgetFileResourceType) {
			QPoint scene_pos(rScenePos);
			qint64 offset_left = rInfo.hotspot.x();
			qint64 offset_right = mpGhost->boundingRect().width() - offset_left;
			qreal offset_top = rInfo.hotspot.y();
			if(scene_pos.x() - offset_left < mpComposition->boundingRect().left()) scene_pos.setX(mpComposition->boundingRect().left() + offset_left);
			else if(scene_pos.x() - offset_left > mpComposition->boundingRect().right()) scene_pos.setX(mpComposition->boundingRect().right() + offset_left);
			int grid_type = Vertical;
			switch(p_origin_resource->type()) {
				case GraphicsWidgetVideoResourceType:
					grid_type |= VideoHorizontal;
					break;
				case GraphicsWidgetAudioResourceType:
					grid_type |= AudioHorizontal;
					break;
				case GraphicsWidgetTimedTextResourceType:
					grid_type |= TimedTextHorizontal;
					break;
				case GraphicsWidgetISXDResourceType:
					grid_type |= ISXDHorizontal;
					break;
				case GraphicsWidgetIABResourceType:
					grid_type |= IABHorizontal;
					break;
				case GraphicsWidgetSADMResourceType:
					grid_type |= SADMHorizontal;
					break;
				case GraphicsWidgetADMResourceType:
					grid_type |= ADMHorizontal;
					break;
			}
			GraphicsSceneComposition::GridInfo grid_info_left = SnapToGrid(QPointF(scene_pos.x() - offset_left, rScenePos.y()), grid_type, QRectF(), mpGhost);
			GraphicsSceneComposition::GridInfo grid_info_right = SnapToGrid(QPointF(scene_pos.x() + offset_right, rScenePos.y()), grid_type, QRectF(), mpGhost);

			QPointF snapped_pos(scene_pos);
			qreal height_advice(p_origin_resource->boundingRect().height());

			// Process vertical snap
			if(snapToGrid) {
				if(grid_info_left.IsVerticalSnap) {
					snapped_pos.setX(grid_info_left.SnapPos.x() + offset_left);
					mpSnapIndicator->SetColor(grid_info_left.ColorAdvice);
					mpSnapIndicator->SetHeight(height());
					mpSnapIndicator->setPos((QPointF(grid_info_left.SnapPos.x(), sceneRect().top())));
					mpSnapIndicator->show();
				}
				else if(grid_info_right.IsVerticalSnap) {
					snapped_pos.setX(grid_info_right.SnapPos.x() - offset_right);
					mpSnapIndicator->SetColor(grid_info_right.ColorAdvice);
					mpSnapIndicator->SetHeight(height());
					mpSnapIndicator->setPos((QPointF(grid_info_right.SnapPos.x(), sceneRect().top())));
					mpSnapIndicator->show();
				}
				else mpSnapIndicator->hide();
			}
			else mpSnapIndicator->hide();

			// Process horizontal snap
			if(grid_info_right.IsHoizontalSnap) {
				snapped_pos.setY(grid_info_right.SnapPos.y());
				if(grid_info_right.HeightAdvice >= 0) height_advice = grid_info_right.HeightAdvice;
			}
			else if(grid_info_left.IsHoizontalSnap) {
				snapped_pos.setY(grid_info_left.SnapPos.y());
				if(grid_info_left.HeightAdvice >= 0) height_advice = grid_info_left.HeightAdvice;
			}

			// Reset segment opacity
			for(int i = 0; i < mpComposition->GetSegmentCount(); i++) {
				mpComposition->GetSegment(i)->setOpacity(0.2);
			}

			QList<QGraphicsItem*> items_list_left = items(snapped_pos.x() - offset_left - 1, snapped_pos.y() - height_advice / 2, 1, height_advice, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
			QList<QGraphicsItem*> items_list_right = items(snapped_pos.x() - offset_left, snapped_pos.y() - height_advice / 2, 1, height_advice, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);

			GraphicsWidgetDummyResource *dummy_left = NULL;
			GraphicsWidgetDummyResource *dummy_right = NULL;
			GraphicsWidgetSequence *p_sequence = NULL;
			// Iterate over items_list_left
			for(int i = 0; i < items_list_left.size(); i++) {
				// Check border case
				if(GraphicsWidgetSegment *seg = dynamic_cast<GraphicsWidgetSegment*>(items_list_left.at(i))) {
					QPointF probe = seg->mapFromScene(scene_pos.x() - offset_left, 0);
					if(seg->contains(QPointF(probe.x(), seg->boundingRect().center().y()))) {
						rInfo.pSegment = seg;
						seg->setOpacity(1);
					}
				}
				GraphicsWidgetDummyResource *p_dummy = dynamic_cast<GraphicsWidgetDummyResource*>(items_list_left.at(i));
				if(p_dummy && p_dummy->size().width() >= 1) dummy_left = p_dummy;
			}

			// Iterate over items_list_right
			for(int i = 0; i < items_list_right.size(); i++) {
				// Check border case
				if(GraphicsWidgetSegment *seg = dynamic_cast<GraphicsWidgetSegment*>(items_list_right.at(i))) {
					QPointF probe = seg->mapFromScene(scene_pos.x() - offset_left, 0);
					if(seg->contains(QPointF(probe.x(), seg->boundingRect().center().y()))) {
						rInfo.pSegment = seg;
						seg->setOpacity(1);
					}
				}
				GraphicsWidgetDummyResource *p_dummy = dynamic_cast<GraphicsWidgetDummyResource*>(items_list_right.at(i));
				if(p_dummy && p_dummy->size().width() >= 1) dummy_right = p_dummy;
			}

			QString warning_text, error_text;
			if(rInfo.pSegment) {
				// Check if drop for audio resource is allowed
				if(p_origin_resource->type() == GraphicsWidgetAudioResourceType) {
					SoundfieldGroup proposed_soundfield_group;
					bool track_is_empty = true;
					if(GraphicsWidgetSequence *p_sequence = dynamic_cast<GraphicsWidgetSequence*>(grid_info_left.HorizontalOrigin)) {
						if(p_sequence->GetType() == MainAudioSequence) {
							QUuid track_id = p_sequence->GetTrackId();
							bool descriptor_found = false;
							for(int i = 0; i < mpComposition->GetSegmentCount(); i++) {
								GraphicsWidgetSegment *p_segment = mpComposition->GetSegment(i);
								if(p_segment) {
									GraphicsWidgetSequence *p_sequence = p_segment->GetSequenceWithTrackId(track_id);
									if(p_sequence && p_sequence->IsEmpty() == false) {
										track_is_empty = false;
										GraphicsWidgetAudioResource *p_resource_other = NULL;
										for (int i=0; i < p_sequence->GetResourceCount(); i++) {
											p_resource_other = dynamic_cast<GraphicsWidgetAudioResource*>(p_sequence->GetResource(i));
											if (p_resource_other && p_resource_other->GetAsset()) {
												descriptor_found = true;
												break;
											}
										}
										if(descriptor_found && p_resource_other->GetSoundfieldGroup() != static_cast<GraphicsWidgetAudioResource*>(p_origin_resource)->GetSoundfieldGroup()) {
											error_text = tr("Sound field group mismatch: %1 expected.").arg(p_resource_other->GetSoundfieldGroup().GetName());
											break;
										}
										else if (p_resource_other && p_resource_other->GetAsset()) {
											proposed_soundfield_group = p_resource_other->GetSoundfieldGroup();
											if (p_resource_other->GetAsset()->GetLanguageTag() != p_origin_resource->GetAsset()->GetLanguageTag()
												|| p_resource_other->GetAsset()->GetMCAAudioContentKind() != p_origin_resource->GetAsset()->GetMCAAudioContentKind()
												|| p_resource_other->GetAsset()->GetMCAAudioElementKind() != p_origin_resource->GetAsset()->GetMCAAudioElementKind()
												|| p_resource_other->GetAsset()->GetMCATitle() != p_origin_resource->GetAsset()->GetMCATitle()
												|| p_resource_other->GetAsset()->GetMCATitleVersion() != p_origin_resource->GetAsset()->GetMCATitleVersion()
												) {
												//get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:yellow}");
												//get_main_window()->statusBar()->showMessage("Warning: Essence Descriptor mismatch", 5000);
												warning_text = "Warning: Essence Descriptor mismatch";

											}
										}

									}
								}
							}
							if (!descriptor_found) {
								warning_text =  "Warning: Cannot determine audio characteristics";
							}
						}
					}
					if(track_is_empty == true) rInfo.isDropable = true;
					else if (!error_text.isEmpty()) {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(error_text, 5000);
					}
					else if (!warning_text.isEmpty()) {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:yellow}");
						get_main_window()->statusBar()->showMessage(warning_text, 2000);
						rInfo.isDropable = true;
					}
					else {
						rInfo.isDropable = true;
					}
				}
				else if(p_origin_resource->type() == GraphicsWidgetVideoResourceType) {
					bool track_is_empty = true;
					if(GraphicsWidgetSequence *p_sequence = dynamic_cast<GraphicsWidgetSequence*>(grid_info_left.HorizontalOrigin)) {
						if(p_sequence->GetType() == MainImageSequence) {
							QUuid track_id = p_sequence->GetTrackId();
							for(int i = 0; i < mpComposition->GetSegmentCount(); i++) {
								GraphicsWidgetSegment *p_segment = mpComposition->GetSegment(i);
								if(p_segment) {
									GraphicsWidgetSequence *p_sequence = p_segment->GetSequenceWithTrackId(track_id);
									if(!(p_origin_resource->GetAsset() && p_origin_resource->GetAsset()->GetEditRate() == GetCplEditRate()))
										error_text = tr("Image edit rate mismatch: %1 expected.").arg(GetCplEditRate().GetQuotient(), 0, 'f', 2);
									if(p_sequence && p_sequence->IsEmpty() == false) {
										track_is_empty = false;
										GraphicsWidgetVideoResource *p_resource_other = NULL;
										for (int i=0; i < p_sequence->GetResourceCount(); i++) {
											p_resource_other = dynamic_cast<GraphicsWidgetVideoResource*>(p_sequence->GetResource(i));
											if (p_resource_other && p_resource_other->GetAsset()) break;
										}
										if(p_resource_other && p_resource_other->GetAsset()) {
											if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().storedWidth != p_origin_resource->GetAsset()->GetMetadata().storedWidth)
													&& (p_resource_other->GetAsset()->GetMetadata().storedHeight != p_origin_resource->GetAsset()->GetMetadata().storedHeight)) {
												error_text = tr("Resolution mismatch: %1 x %2 expected.").arg(p_resource_other->GetAsset()->GetMetadata().storedWidth).arg(p_resource_other->GetAsset()->GetMetadata().storedHeight);

											}
											else if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().colorEncoding != p_origin_resource->GetAsset()->GetMetadata().colorEncoding)) {
												QString color;
												switch (p_resource_other->GetAsset()->GetMetadata().colorEncoding) {
												case Metadata::eColorEncoding::Unknown_Color_Encoding:
													color = "Unknown";
													break;
												case Metadata::eColorEncoding::RGBA:
													color = "RGB";
													break;
												case Metadata::eColorEncoding::CDCI:
													color = "YCbCr";
													break;
												}
												error_text = tr("Color encoding mismatch: %1 expected.").arg(color);
											}
											else if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().horizontalSubsampling != p_origin_resource->GetAsset()->GetMetadata().horizontalSubsampling)) {
												QString subsampling;
												switch (p_resource_other->GetAsset()->GetMetadata().horizontalSubsampling) {
												case 1:
													subsampling = "4:4:4";
													break;
												case 2:
													subsampling = "4:2:2";
													break;
												default:
													subsampling = "Unknown";
													break;
												}
												error_text = tr("Color subsampling mismatch: %1 expected.").arg(subsampling);

											}
											else if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().colorPrimaries != p_origin_resource->GetAsset()->GetMetadata().colorPrimaries)) {
												error_text = tr("Color primaries mismatch.");

											}
											else if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().transferCharcteristics != p_origin_resource->GetAsset()->GetMetadata().transferCharcteristics)) {
												error_text = tr("Transfer characteristics mismatch.");

											}
											else if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().pictureEssenceCoding != p_origin_resource->GetAsset()->GetMetadata().pictureEssenceCoding)) {
												error_text = tr("Picture Essence Coding mismatch.");

											}
											else if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().componentDepth != p_origin_resource->GetAsset()->GetMetadata().componentDepth)) {
												if (p_resource_other->GetAsset()->GetMetadata().componentDepth) {
													error_text = tr("Component Bit Depth mismatch: %1 expected.").arg(p_resource_other->GetAsset()->GetMetadata().componentDepth);
												} else {
													get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:yellow}");
													get_main_window()->statusBar()->showMessage("Warning: Cannot determine component bit depth of virtaul track", 5000);
												}

											}
										} else {
											get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:yellow}");
											get_main_window()->statusBar()->showMessage("Warning: Cannot determine image characteristics", 5000);

										}
									}
									if (error_text.isEmpty()) {
										rInfo.isDropable = true;
									} else {
										get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
										get_main_window()->statusBar()->showMessage(error_text, 5000);
									}
								}
							}
						}
					}
				}

				else if(p_origin_resource->type() == GraphicsWidgetTimedTextResourceType) {

					if(p_origin_resource->GetAsset() && p_origin_resource->GetAsset()->GetEditRate() == GetCplEditRate()) {
						rInfo.isDropable = true;
					}
					else {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(tr("Timed Text sample rate mismatch: %1 expected.").arg(GetCplEditRate().GetQuotient(), 0, 'f', 2), 5000);
					}
				}

				else if(p_origin_resource->type() == GraphicsWidgetISXDResourceType) {
					if(p_origin_resource->GetAsset() && p_origin_resource->GetAsset()->GetEditRate() == GetCplEditRate()) {
						rInfo.isDropable = true;
					}
					else {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(tr("ISXD sample rate mismatch: %1 expected.").arg(GetCplEditRate().GetQuotient(), 0, 'f', 2), 5000);
					}
				}

				else if(p_origin_resource->type() == GraphicsWidgetIABResourceType) {
					// TODO Per 2067-201, the IAB Edit Rate shall be an Integer multiple of the image Edit Rate
					if(p_origin_resource->GetAsset() && p_origin_resource->GetAsset()->GetEditRate() == GetCplEditRate()) {
						rInfo.isDropable = true;
					}
					else {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(tr("IAB sample rate mismatch: %1 expected.").arg(GetCplEditRate().GetQuotient(), 0, 'f', 2), 5000);
					}
				}

				else if(p_origin_resource->type() == GraphicsWidgetSADMResourceType) {
					// Per 2067-203, The Edit Rate shall be an integer multiple of the Edit Rate of the Main Image Virtual Track
					if(p_origin_resource->GetAsset().isNull()) {
						error_text = tr("Asset is NULL");
					} else if(GraphicsWidgetSequence *p_sequence = dynamic_cast<GraphicsWidgetSequence*>(grid_info_left.HorizontalOrigin)) {
						if(p_sequence->GetType() == MGASADMSignalSequence) {
							QUuid track_id = p_sequence->GetTrackId();
							for(int i = 0; i < mpComposition->GetSegmentCount(); i++) {
								GraphicsWidgetSegment *p_segment = mpComposition->GetSegment(i);
								if(p_segment) {
									GraphicsWidgetSequence *p_sequence = p_segment->GetSequenceWithTrackId(track_id);
									if(p_sequence && p_sequence->IsEmpty() == false) {
										GraphicsWidgetSADMResource *p_resource_other = NULL;
										for (int i=0; i < p_sequence->GetResourceCount(); i++) {
											p_resource_other = dynamic_cast<GraphicsWidgetSADMResource*>(p_sequence->GetResource(i));
											if (p_resource_other && p_resource_other->GetAsset()) break;
										}
										if(p_resource_other && p_resource_other->GetAsset()) {
											if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().audioSamplingRate != p_origin_resource->GetAsset()->GetMetadata().audioSamplingRate)) {
												error_text = tr("Audio sampling rate mismatch: %1 but %2 expected.").arg(p_origin_resource->GetAsset()->GetMetadata().audioSamplingRate.GetQuotient()).arg(p_resource_other->GetAsset()->GetMetadata().audioSamplingRate.GetName());

											}
											if (error_text.isEmpty() && (p_resource_other->GetAsset()->GetMetadata().editRate != p_origin_resource->GetAsset()->GetMetadata().editRate)) {
												error_text = tr("MGA Edit rate mismatch: %1 fps but %2 fps expected.").arg(p_origin_resource->GetAsset()->GetMetadata().editRate.GetQuotient()).arg(p_resource_other->GetAsset()->GetMetadata().editRate.GetName());

											}
										}
									}
								}
							}
						}
					}
					if (((p_origin_resource->GetAsset()->GetEditRate().GetNumerator() % GetCplEditRate().GetNumerator()) != 0)
							|| (p_origin_resource->GetAsset()->GetEditRate().GetDenominator() != GetCplEditRate().GetDenominator())) {
						error_text = tr("S-ADM edit rate mismatch: %1 or a multiple expected.").arg(GetCplEditRate().GetQuotient(), 0, 'f', 2);
					}  else if ((p_origin_resource->GetAsset()->GetAudioSamplingRate().GetNumerator()  != 48000)
						&& (p_origin_resource->GetAsset()->GetAudioSamplingRate().GetNumerator()  != 96000)) {
						error_text = tr("MGA S-ADM sampling rate mismatch: %1 Hz, 48000 or 96000 is expected.").arg(p_origin_resource->GetAsset()->GetAudioSamplingRate().GetQuotient(), 0, 'f', 0);
					}

					if (error_text.isEmpty()) {
						rInfo.isDropable = true;
					} else {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(error_text, 5000);
					}
				}

				else if(p_origin_resource->type() == GraphicsWidgetADMResourceType) {
					// TODO 2067-204 edit rate
					rInfo.isDropable = true;
				}

				if((dummy_right != NULL && dummy_right == dummy_left) || (rInfo.pSegment && dummy_left != NULL && rInfo.pSegment->isAncestorOf(dummy_left))) {
					if(rInfo.isDropable == true) {
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(tr("Resource cannot be dropped here."), 5000);
					}
					rInfo.isDropable = false;
				}
			}
			else {
				get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
				get_main_window()->statusBar()->showMessage(tr("Resource cannot be dropped here."), 5000);
			}

			// Overwrite cursor and ghost opacity depending on drop state
			if(rInfo.isDropable == true) {
				//get_main_window()->statusBar()->clearMessage();
				mpGhost->SetColor(p_origin_resource->GetColor().darker(150));
				QCursor *p_cursor = QGuiApplication::overrideCursor();
				if(p_cursor && p_cursor->shape() == Qt::ForbiddenCursor) QGuiApplication::restoreOverrideCursor();
			}
			else {
				mpGhost->SetColor(QColor(CPL_COLOR_RESOURCE_NOT_DROPPABLE));
				QCursor *p_cursor = QGuiApplication::overrideCursor();
				if(p_cursor && p_cursor->shape() == Qt::ClosedHandCursor) QGuiApplication::setOverrideCursor(Qt::ForbiddenCursor);
			}

			if(mode == InsertMode) {
				mpInsertIndicatorTop->setPos(snapped_pos.x() - offset_left, snapped_pos.y() - height_advice / 2);
				mpInsertIndicatorTop->show();
				mpInsertIndicatorBottom->setPos(snapped_pos.x() - offset_left, snapped_pos.y() + height_advice / 2 - 12);
				mpInsertIndicatorBottom->show();
			}
			else if(mode == OverwriteMode) {
				mpInsertIndicatorTop->hide();
				mpInsertIndicatorBottom->hide();
			}

			mpGhost->setGeometry(snapped_pos.x() - offset_left, snapped_pos.y() - height_advice / 2, mpGhost->boundingRect().width(), height_advice);
		}
	}
}

void GraphicsSceneComposition::ProcessDrop(DragDropInfo &rInfo, bool addResource /*= false*/) {

	if(rInfo.state == DragDropInfo::DragMove) {
		rInfo.dropSucceeded = false;
		AbstractGraphicsWidgetResource *p_origin_resource = rInfo.pOriginResource;
		p_origin_resource->SetSourceDuration(p_origin_resource->GetSourceDuration());  //For Audio: Ensures that SourceDuration is an integer multiple of samples per frame
		if(rInfo.isDropable == true) {
			// TODO: rinfo segment is null
			Timecode edit_left = mpGhost->MapToCplTimeline(mpGhost->GetFirstVisibleFrame());
			Timecode edit_right = mpGhost->MapToCplTimeline(mpGhost->GetLastVisibleFrame());
			GraphicsWidgetSegment *p_segment = rInfo.pSegment;
			GraphicsWidgetSequence *p_sequence = NULL;
			AbstractGraphicsWidgetResource *pEditLeftResource = NULL;
			AbstractGraphicsWidgetResource *pEditRightResource = NULL;
			QList<QGraphicsItem*> items_list = items(mpGhost->pos().x(), mpGhost->pos().y(), 1, mpGhost->boundingRect().height(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
			for(int i = 0; i < items_list.size(); i++) {
				if(AbstractGraphicsWidgetResource *res = dynamic_cast<AbstractGraphicsWidgetResource*>(items_list.at(i))) {
					if(res && res != mpGhost && res != p_origin_resource && res->type() != GraphicsWidgetDummyResourceType && res->type() != GraphicsWidgetMarkerResourceType) {
						if(p_segment->isAncestorOf(res)) pEditLeftResource = res;
					}
				}
				if(GraphicsWidgetSequence *seq = dynamic_cast<GraphicsWidgetSequence*>(items_list.at(i))) {
					if(seq) {
						if(p_segment->isAncestorOf(seq)) p_sequence = seq;
					}
				}
			}
			items_list = items(mpGhost->pos().x() + mpGhost->boundingRect().width() - 1, mpGhost->pos().y(), 1, mpGhost->boundingRect().height(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
			for(int i = 0; i < items_list.size(); i++) {
				if(AbstractGraphicsWidgetResource *res = dynamic_cast<AbstractGraphicsWidgetResource*>(items_list.at(i))) {
					if(res && res != mpGhost && res != p_origin_resource && res->type() != GraphicsWidgetDummyResourceType && res->type() != GraphicsWidgetMarkerResourceType) {
						if(p_segment->isAncestorOf(res)) pEditRightResource = res;
					}
				}
			}

			if(p_sequence) {
				QUndoCommand *p_root = new QUndoCommand(NULL);
				int new_index = -1;
				if(p_sequence->isAncestorOf(p_origin_resource)) new_index = p_sequence->GetResourceIndex(p_origin_resource);
				// insert mode
				if(rInfo.mode == InsertMode) {
					if(pEditLeftResource) {
						AppendEditCommand(edit_left, pEditLeftResource, p_root);
						new_index = p_sequence->GetResourceIndex(pEditLeftResource);
					}
					if(new_index < 0) new_index = p_sequence->GetResourceCount();
					if(p_root->childCount() > 0) new_index++;
					if(p_sequence->isAncestorOf(p_origin_resource) && p_sequence->GetResourceIndex(p_origin_resource) < new_index) new_index--;
				}
				// overwrite mode
				else if(rInfo.mode == OverwriteMode) {
					AbstractGraphicsWidgetResource *p_replace_right_resource = NULL;
					if(pEditLeftResource && pEditLeftResource == pEditRightResource) {
						p_replace_right_resource = pEditLeftResource->Clone();
						p_replace_right_resource->hide();
						addItem(p_replace_right_resource);
						new AddResourceCommand(p_replace_right_resource, p_sequence->GetResourceIndex(pEditLeftResource) + 1, p_sequence, p_root);
					}
					if(pEditLeftResource) {
						Timecode first_visible_frame = pEditLeftResource->MapToCplTimeline(pEditLeftResource->GetFirstVisibleFrame());
						if((edit_left - first_visible_frame).AsDuration() > 0) {
							new_index = p_sequence->GetResourceIndex(pEditLeftResource) + 1;
							Duration old_source_duration = pEditLeftResource->GetSourceDuration();
							Duration new_source_duration = pEditLeftResource->MapFromCplTimeline((edit_left - first_visible_frame).AsPositiveDuration());
							new SetSourceDurationCommand(pEditLeftResource, old_source_duration, new_source_duration, p_root);
						}
						else {
							new_index = p_sequence->GetResourceIndex(pEditLeftResource);
							new RemoveResourceCommand(pEditLeftResource, p_sequence, p_root);
						}
					}
					if(pEditRightResource) {
						Timecode first_visible_frame = pEditRightResource->MapToCplTimeline(pEditRightResource->GetFirstVisibleFrame());
						Timecode last_visible_frame = pEditRightResource->MapToCplTimeline(pEditRightResource->GetLastVisibleFrame());
						if((last_visible_frame - edit_right).AsDuration() > 0) {
							Duration old_entry_point = pEditRightResource->GetEntryPoint();
							Duration new_entry_point = pEditRightResource->MapFromCplTimeline((edit_right - first_visible_frame).AsPositiveDuration() + 1) + pEditRightResource->GetEntryPoint();
							if(p_replace_right_resource != NULL) pEditRightResource = p_replace_right_resource;
							new SetEntryPointCommand(pEditRightResource, old_entry_point, new_entry_point, p_root);
						}
						else {
							if(p_replace_right_resource != NULL) pEditRightResource = p_replace_right_resource;
							new RemoveResourceCommand(pEditRightResource, p_sequence, p_root);
						}
					}
					QList<QGraphicsItem*> obsolete_resources = items(mpGhost->geometry(), Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
					for(int i = 0; i < obsolete_resources.size(); i++) {
						AbstractGraphicsWidgetResource* p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(obsolete_resources.at(i));
						if(p_resource && p_resource->type() != GraphicsWidgetDummyResourceType && p_segment->isAncestorOf(p_resource) && p_resource != mpGhost && p_resource != pEditLeftResource && p_resource != pEditRightResource) {
							new RemoveResourceCommand(p_resource, p_resource->GetSequence(), p_root);
						}
					}
					if(new_index < 0) new_index = p_sequence->GetResourceCount();
					if(p_sequence->isAncestorOf(p_origin_resource) && p_sequence->GetResourceIndex(p_origin_resource) < new_index) new_index--;
				}
				if(addResource) new AddResourceCommand(p_origin_resource, new_index, p_sequence, p_root);
				else new MoveResourceCommand(p_origin_resource, new_index, p_sequence, p_origin_resource->GetSequence(), p_root);
				if(p_root->childCount() > 0) {
					PushCommand(p_root);
					rInfo.dropSucceeded = true;
				}
				else delete p_root;
			}
			// Border case segment transition
			else {
				items_list = items(mpGhost->pos().x() - 1, mpGhost->pos().y(), 1, mpGhost->boundingRect().height(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
				for(int i = 0; i < items_list.size(); i++) {
					if(GraphicsWidgetSequence *seq = dynamic_cast<GraphicsWidgetSequence*>(items_list.at(i))) {
						if(seq && p_segment->isAncestorOf(seq)) {
							if(addResource) {
								rInfo.dropSucceeded = true;
								PushCommand(new AddResourceCommand(p_origin_resource, seq->GetResourceCount(), seq));
							}
							else {
								rInfo.dropSucceeded = true;
								PushCommand(new MoveResourceCommand(p_origin_resource, seq->GetResourceCount(), seq, p_origin_resource->GetSequence()));
							}
						}
					}
				}
			}
		}
	}
}

void GraphicsSceneComposition::ProcessCleanUp(DragDropInfo &rInfo) {

	get_main_window()->statusBar()->clearMessage();
	if(mpGhost) mpGhost->hide();
	delete mpGhost;
	mpGhost = NULL;
	// Reset segment opacity.
	for(int i = 0; i < mpComposition->GetSegmentCount(); i++) {
		mpComposition->GetSegment(i)->setOpacity(1);
	}
	while(QGuiApplication::overrideCursor() != NULL) QGuiApplication::restoreOverrideCursor();
	mpSnapIndicator->hide();
	mpInsertIndicatorBottom->hide();
	mpInsertIndicatorTop->hide();
	rInfo.state = DragDropInfo::InitDrag;
}

void GraphicsSceneComposition::keyPressEvent(QKeyEvent *pEvent) {

	if(pEvent->key() == Qt::Key_Delete && mouseGrabberItem() == NULL) {
		QList<QGraphicsItem*> items = selectedItems();
		if(items.isEmpty() == false) {
			QGraphicsItem *p_item = items.first();

			switch(p_item->type()) {
				case 	GraphicsWidgetFileResourceType:
				case	GraphicsWidgetVideoResourceType:
				case	GraphicsWidgetAudioResourceType:
				case	GraphicsWidgetMarkerResourceType:
				case	GraphicsWidgetTimedTextResourceType:
				case 	GraphicsWidgetISXDResourceType:
				case 	GraphicsWidgetIABResourceType:
				case 	GraphicsWidgetSADMResourceType:
				case 	GraphicsWidgetADMResourceType:
					emit PushCommand(new RemoveResourceCommand(static_cast<AbstractGraphicsWidgetResource*>(p_item), static_cast<AbstractGraphicsWidgetResource*>(p_item)->GetSequence()));
					break;
				case GraphicsWidgetMarkerType:
					emit PushCommand(new RemoveMarkerCommand(static_cast<GraphicsObjectVerticalIndicator*>(p_item), static_cast<GraphicsWidgetMarkerResource*>(p_item->parentItem())));
					break;
			}
		}
	}
	if(pEvent->key() == Qt::Key_Shift) {
		if(mDragActive) {
			ProcessDragMove(mDropInfo, mDropInfo.lastScenePos, false, mDropInfo.mode);
		}
	}
	if(pEvent->key() == Qt::Key_Control) {
		if(mDragActive) {
			ProcessDragMove(mDropInfo, mDropInfo.lastScenePos, mDropInfo.isSnapToGridMode, InsertMode);
		}
	}
	if(pEvent->key() == Qt::Key_Escape) {
		if(mDragActive) {
			mDragActive = false;
			ProcessCleanUp(mDropInfo);
		}
	}
	QGraphicsScene::keyPressEvent(pEvent);
}

void GraphicsSceneComposition::keyReleaseEvent(QKeyEvent *pEvent) {

	if(pEvent->key() == Qt::Key_Shift) {
		if(mDragActive) {
			ProcessDragMove(mDropInfo, mDropInfo.lastScenePos, true, mDropInfo.mode);
		}
	}
	if(pEvent->key() == Qt::Key_Control) {
		if(mDragActive) {
			ProcessDragMove(mDropInfo, mDropInfo.lastScenePos, mDropInfo.isSnapToGridMode, OverwriteMode);
		}
	}
	QGraphicsScene::keyReleaseEvent(pEvent);
}

void GraphicsSceneComposition::rCompositionGeometryChanged() {

	if(mpCurrentFrameIndicator) mpCurrentFrameIndicator->SetHeight(mpComposition->boundingRect().height());
}

QList<AbstractGridExtension*> GraphicsSceneComposition::AddPermanentSnapItems() const {

	QList<AbstractGridExtension*> ret;
	ret << mpCurrentFrameIndicator;
	return ret;
}

void GraphicsSceneComposition::SetEditRequest() {

	QList<QGraphicsItem*> items_list = items(mpCurrentFrameIndicator->pos().x(), 0, 1, sceneRect().height(), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
	QList<AbstractGraphicsWidgetResource*>resources;
	for(int i = 0; i < items_list.size(); i++) {
		AbstractGraphicsWidgetResource *p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(items_list.at(i));
		if(p_resource && p_resource->type() != GraphicsWidgetDummyResourceType && p_resource->type() != GraphicsWidgetMarkerResourceType) {
			if(p_resource->isSelected() == true) {
				resources.clear();
				resources.push_back(p_resource);
				break;
			}
			else resources.push_back(p_resource);
		}
	}
	SetEditRequest(Timecode(GetCplEditRate(), mpCurrentFrameIndicator->pos().x()), resources);
}

void GraphicsSceneComposition::SetEditRequest(const Timecode &rCplTimecode, QList<AbstractGraphicsWidgetResource*>resources) {

	QUndoCommand *p_root = new QUndoCommand(NULL);
	AppendEditCommand(rCplTimecode, resources, p_root);
	if(p_root && p_root->childCount() > 0) PushCommand(p_root);
	else delete p_root;
}

bool GraphicsSceneComposition::AppendEditCommand(const Timecode &rCplTimecode, QList<AbstractGraphicsWidgetResource*>resources, QUndoCommand *pRootCommand) {

	if(pRootCommand) {
		for(int i = 0; i < resources.size(); i++) {
			AbstractGraphicsWidgetResource *p_resource = resources.at(i);
			GraphicsWidgetAudioResource *p_audio_resource = qobject_cast<GraphicsWidgetAudioResource *>(p_resource);

			if (p_audio_resource) {
				GraphicsWidgetSequence *p_sequence = p_audio_resource->GetSequence();
				Duration old_duration = p_audio_resource->GetSourceDuration();
				Duration entry_point = p_audio_resource->GetEntryPoint();
				Duration position_duration = rCplTimecode.GetOverallFrames() * p_audio_resource->GetEditRate().GetNumerator() / p_audio_resource->GetEditRate().GetDenominator()  * p_audio_resource->GetCplEditRate().GetDenominator() / p_audio_resource->GetCplEditRate().GetNumerator() + .5;
				//Duration position_duration = p_audio_resource->ResourceErPerCompositionEr(p_audio_resource->GetEditRate()) * rCplTimecode.GetOverallFrames();
				Duration new_duration = (position_duration - entry_point).GetCount() % old_duration.GetCount();
				if(new_duration == 0) continue;
				GraphicsWidgetAudioResource *p_clone = p_audio_resource->Clone();
				addItem(p_clone);
				new EditCommand(p_audio_resource, p_audio_resource->GetSourceDuration(), new_duration,
												p_clone, p_audio_resource->GetSourceDuration(), old_duration - new_duration,
												p_clone->GetEntryPoint(), p_audio_resource->GetEntryPoint() + new_duration,
												p_sequence->GetResourceIndex(p_audio_resource) + 1, p_sequence, pRootCommand);
			} else if(p_resource) {
				GraphicsWidgetSequence *p_sequence = p_resource->GetSequence();
				Duration old_duration = p_resource->MapToCplTimeline(p_resource->GetSourceDuration());
				Timecode current_frame(rCplTimecode);
				Timecode first_visible_frame = p_resource->MapToCplTimeline(p_resource->GetFirstVisibleFrame());
				Duration new_duration = (current_frame - first_visible_frame).AsPositiveDuration().GetCount() % old_duration.GetCount(); // The modulo is for repeat count != 1
				if(new_duration == 0) continue;
				AbstractGraphicsWidgetResource *p_clone = p_resource->Clone();
				addItem(p_clone);
				new EditCommand(p_resource, p_resource->GetSourceDuration(), p_resource->MapFromCplTimeline(new_duration),
												p_clone, p_resource->GetSourceDuration(), p_resource->MapFromCplTimeline(old_duration - new_duration),
												p_clone->GetEntryPoint(), p_resource->GetEntryPoint() + p_resource->MapFromCplTimeline(new_duration),
												p_sequence->GetResourceIndex(p_resource) + 1, p_sequence, pRootCommand);
			}
		}
	}
	if(pRootCommand && pRootCommand->childCount() > 0) return true;
	return false;
}

bool GraphicsSceneComposition::AppendEditCommand(const Timecode &rCplTimecode, AbstractGraphicsWidgetResource *pResource, QUndoCommand *pRootCommand) {

	QList<AbstractGraphicsWidgetResource*>resources;
	resources << pResource;
	return AppendEditCommand(rCplTimecode, resources, pRootCommand);
}

void GraphicsSceneComposition::dragEnterEvent(QGraphicsSceneDragDropEvent *pEvent) {

	bool accept = false;
	const ImfMimeData *p_mime = dynamic_cast<const ImfMimeData*>(pEvent->mimeData());
	if(p_mime) {
		QSharedPointer<AssetMxfTrack> asset = p_mime->GetAsset().objectCast<AssetMxfTrack>();
		if(asset) {
			double samples_per_cpl_edit_rate = asset->GetEditRate().GetNumerator() * GetCplEditRate().GetDenominator() / double(GetCplEditRate().GetNumerator() * asset->GetEditRate().GetDenominator());
			if(asset->GetDuration().GetCount() / samples_per_cpl_edit_rate >= 1) {
				AbstractGraphicsWidgetResource *p_resource = NULL;
				switch(asset->GetEssenceType()) {
					case Metadata::Jpeg2000:
					case Metadata::ProRes:
#ifdef APP5_ACES
					case Metadata::Aces:
#endif
#ifdef CODEC_HTJ2K
					case Metadata::HTJ2K:
#endif
						p_resource = new GraphicsWidgetVideoResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;
					case Metadata::Pcm:
						p_resource = new GraphicsWidgetAudioResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;


							/* -----Denis Manthey Beg----- */
					case Metadata::TimedText:
						p_resource = new GraphicsWidgetTimedTextResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;
							/* -----Denis Manthey End----- */

					case Metadata::ISXD:
						p_resource = new GraphicsWidgetISXDResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;

					case Metadata::IAB:
						p_resource = new GraphicsWidgetIABResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;

					case Metadata::SADM:
						p_resource = new GraphicsWidgetSADMResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;

					case Metadata::ADM:
						p_resource = new GraphicsWidgetADMResource(NULL, asset);
						p_resource->hide();
						addItem(p_resource);
						ProcessInitDrag(mDropInfo, p_resource, pEvent->scenePos().toPoint(), QPoint(p_resource->boundingRect().center().toPoint()));
						accept = true;
						break;

					default:
						get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
						get_main_window()->statusBar()->showMessage(tr("Unsupported MXF asset"), 5000);
						break;
				}
			}
			else {
				get_main_window()->statusBar()->setStyleSheet("QStatusBar{color:red}");
				get_main_window()->statusBar()->showMessage(tr("MXF asset duration < one composition edit unit"), 5000);
			}
		}
	}
	pEvent->setAccepted(accept);
}

void GraphicsSceneComposition::dragMoveEvent(QGraphicsSceneDragDropEvent *pEvent) {

	ProcessDragMove(mDropInfo, pEvent->scenePos().toPoint(), !(pEvent->modifiers() & Qt::ShiftModifier), (pEvent->modifiers() & Qt::ControlModifier ? InsertMode : OverwriteMode));
	pEvent->setAccepted(mDropInfo.isDropable);
}

void GraphicsSceneComposition::dragLeaveEvent(QGraphicsSceneDragDropEvent *pEvent) {

	if(mDropInfo.pOriginResource) mDropInfo.pOriginResource->deleteLater();
	ProcessCleanUp(mDropInfo);
}

void GraphicsSceneComposition::dropEvent(QGraphicsSceneDragDropEvent *pEvent) {

	ProcessDrop(mDropInfo, true);
	if(mDropInfo.pOriginResource && mDropInfo.dropSucceeded == false) mDropInfo.pOriginResource->deleteLater();
	ProcessCleanUp(mDropInfo);
}

GraphicsWidgetSegment* GraphicsSceneComposition::GetSegmentAt(const Timecode &rCplTimecode) const {

	QList<QGraphicsItem*> items_list = items(QPointF(rCplTimecode.GetOverallFrames(), mpComposition->boundingRect().center().y()), Qt::IntersectsItemBoundingRect, Qt::AscendingOrder);
	for(int i = 0; i < items_list.size(); i++) {
		GraphicsWidgetSegment *p_segment = dynamic_cast<GraphicsWidgetSegment*>(items_list.at(i));
		if(p_segment) return p_segment;
	}
	return NULL;
}

QList<AbstractGraphicsWidgetResource*> GraphicsSceneComposition::GetResourcesAt(const Timecode &rCplTimecode, SequenceTypes filter /*= Unknown*/) const {

	QList<AbstractGraphicsWidgetResource*> ret;
	QList<QGraphicsItem*> items_list = items(QRectF(rCplTimecode.GetOverallFrames(), 0, 1, sceneRect().height()), Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
	for(int i = 0; i < items_list.size(); i++) {
		AbstractGraphicsWidgetResource *p_resource = dynamic_cast<AbstractGraphicsWidgetResource*>(items_list.at(i));
		if(p_resource) {
			GraphicsWidgetSequence *p_sequence = p_resource->GetSequence();
			if(p_sequence) {
				if(filter & Unknown) ret << p_resource;
				else if(p_sequence->GetType() & filter) {
					ret << p_resource;
				}
			}
		}
	}
	return ret;
}

GraphicsSceneTimeline::GraphicsSceneTimeline(const EditRate &rCplEditRate /*= EditRate::EditRate24*/, QObject *pParent /*= NULL*/) :
GraphicsSceneBase(rCplEditRate, pParent), mpCurrentFrameIndicator(NULL), mpSnapIndicator(NULL), mpSegmentGhost(NULL), mExecuteDrop(QPair<bool, QUuid>(false, QUuid())), mDragActive(false) {

	mpTimeline = new GraphicsWidgetTimeline();
	addItem(mpTimeline);
	mpSnapIndicator = new GraphicsObjectVerticalIndicator(1, sceneRect().height(), QColor(CPL_COLOR_DEFAULT_SNAP_INDICATOR), NULL);
	addItem(mpSnapIndicator);
	mpSnapIndicator->setZValue(2);
	mpSnapIndicator->hide();

	// create current frame indicator
	QPixmap indicator_head(":/indicator.png");
	mpCurrentFrameIndicator = new GraphicsObjectVerticalIndicator(2, indicator_head.height(), QColor(CPL_COLOR_CURRENT_FRAME_INDICATOR), NULL);
	mpCurrentFrameIndicator->SetHeadImage(indicator_head);
	mpCurrentFrameIndicator->ShowHead();
	mpCurrentFrameIndicator->HideLine();
	mpCurrentFrameIndicator->setPos(0, 0);
	mpCurrentFrameIndicator->setZValue(2);
	addItem(mpCurrentFrameIndicator);

	connect(mpCurrentFrameIndicator, SIGNAL(XPosChanged(qreal)), this, SLOT(rXPosChanged(qreal)));
	connect(mpTimeline, SIGNAL(geometryChanged()), this, SLOT(rTimelineGeometryChanged()));
}

void GraphicsSceneTimeline::mousePressEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsSceneBase::mousePressEvent(pEvent);
	if(mouseGrabberItem() == NULL) {
		emit FrameInicatorActive(true);
		mpCurrentFrameIndicator->setX((qint64)(pEvent->scenePos().x() + .5)); // (k)
	}
}

void GraphicsSceneTimeline::mouseMoveEvent(QGraphicsSceneMouseEvent *pEvent) {

	GraphicsSceneBase::mouseMoveEvent(pEvent);
	if(!(pEvent->buttons() == Qt::LeftButton)) return;
	if(mouseGrabberItem() == NULL) {
		if(pEvent->scenePos().x() < mpTimeline->boundingRect().left()) mpCurrentFrameIndicator->setX(mpTimeline->boundingRect().left());
		else if(pEvent->scenePos().x() > mpTimeline->boundingRect().right()) mpCurrentFrameIndicator->setX(mpTimeline->boundingRect().right());
		else mpCurrentFrameIndicator->setX((qint64)(pEvent->scenePos().x() + .5));
	}
	else {
		if((pEvent->screenPos() - pEvent->buttonDownScreenPos(Qt::LeftButton)).manhattanLength() >= QApplication::startDragDistance()) mDragActive = true;
		if(mDragActive) {
			GraphicsWidgetSegmentIndicator *p_indicator = dynamic_cast<GraphicsWidgetSegmentIndicator*>(mouseGrabberItem());
			if(p_indicator) {
				if(mpSegmentGhost == NULL) {
					mpSegmentGhost = p_indicator->Clone();
					addItem(mpSegmentGhost);
					mpSegmentGhost->setOpacity(.9);
					mpSegmentGhost->setEnabled(false);
					mpSegmentGhost->setZValue(1);
					QGuiApplication::setOverrideCursor(Qt::ForbiddenCursor);
				}
				else {
					QPoint scene_pos(pEvent->scenePos().toPoint());
					QPoint button_down_scene_pos(pEvent->buttonDownScenePos(Qt::LeftButton).toPoint());
					qint64 offset_left = p_indicator->mapFromScene(button_down_scene_pos).x();
					qint64 offset_right = mpSegmentGhost->boundingRect().width() - offset_left;
					if(scene_pos.x() - offset_left < mpTimeline->boundingRect().left()) scene_pos.setX(mpTimeline->boundingRect().left() + offset_left);
					else if(scene_pos.x() + offset_right > mpTimeline->boundingRect().right()) scene_pos.setX(mpTimeline->boundingRect().right() - offset_right);
					GraphicsSceneComposition::GridInfo grid_info_left = SnapToGrid(QPointF(scene_pos.x() - offset_left, 0), Vertical, QRectF(), mpSegmentGhost);

					QPointF tmp(scene_pos);
					GraphicsWidgetSegmentIndicator* p_snap_origin = dynamic_cast<GraphicsWidgetSegmentIndicator*>(grid_info_left.VerticalOrigin);
					if(grid_info_left.IsVerticalSnap && p_snap_origin && p_snap_origin->GetId() != p_indicator->GetId()) {
						QCursor *p_cursor = QGuiApplication::overrideCursor();
						if(p_cursor && p_cursor->shape() == Qt::ForbiddenCursor) QGuiApplication::setOverrideCursor(Qt::ClosedHandCursor);
						tmp.setX(grid_info_left.SnapPos.x() + offset_left);
						mpSnapIndicator->SetColor(grid_info_left.ColorAdvice);
						mpSnapIndicator->SetHeight(height());
						mpSnapIndicator->setPos((QPointF(grid_info_left.SnapPos.x(), sceneRect().top())));
						mpSnapIndicator->show();
						mExecuteDrop = QPair<bool, QUuid>(true, p_snap_origin->GetId());
					}
					else {
						QCursor *p_cursor = QGuiApplication::overrideCursor();
						if(p_cursor && p_cursor->shape() == Qt::ClosedHandCursor) QGuiApplication::restoreOverrideCursor();
						mpSnapIndicator->hide();
						mExecuteDrop = QPair<bool, QUuid>(false, QUuid());
					}
					QPointF grid_point = p_indicator->mapFromScene(tmp);
					mpSegmentGhost->setPos(tmp.x() - offset_left, p_indicator->pos().y());
				}
			}
		}
	}
}

void GraphicsSceneTimeline::mouseReleaseEvent(QGraphicsSceneMouseEvent *pEvent) {

	if(pEvent->button() == Qt::LeftButton) {
		if(mpSegmentGhost) {
			if(mExecuteDrop.first == true) {
				emit MoveSegmentRequest(mpSegmentGhost->GetId(), mExecuteDrop.second);
			}
			mpSegmentGhost->deleteLater();
		}
		mpSegmentGhost = NULL;
		mExecuteDrop = QPair<bool, QUuid>(false, QUuid());
		while(QGuiApplication::overrideCursor() != NULL) QGuiApplication::restoreOverrideCursor();
		mDragActive = false;
		mpSnapIndicator->hide();
	}
	if(mouseGrabberItem() == NULL) {
		emit FrameInicatorActive(false);
	}
	GraphicsSceneBase::mouseReleaseEvent(pEvent);
}

void GraphicsSceneTimeline::rTimelineGeometryChanged() {

	if(mpCurrentFrameIndicator) mpCurrentFrameIndicator->setPos(mpCurrentFrameIndicator->pos().x(), mpTimeline->boundingRect().bottom() - mpCurrentFrameIndicator->boundingRect().height());
}
