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
#include "GraphicsViewScaleable.h"
#include <QKeyEvent>
#include <QGraphicsItem>
#include <cmath>


GraphicsViewScaleable::GraphicsViewScaleable(QWidget *pParent /*= NULL*/) :
QGraphicsView(pParent) {

	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate); // Work around: Disable funny behavior of exposedrect.
	// 	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

void GraphicsViewScaleable::ZoomIn() {

	ScaleView(qreal(1.1));
}

void GraphicsViewScaleable::ZoomOut() {

	ScaleView(1 / qreal(1.1));
}

void GraphicsViewScaleable::ScaleView(qreal scaleFactor) {

	qreal factor = transform().scale(scaleFactor, 1).mapRect(QRectF(0, 0, 1, 1)).width();
	if(factor < 0.001 || factor > 100) return;
	scale(scaleFactor, 1);
	QList<QGraphicsItem*> items_list(items());
	for(int i = 0; i < items_list.size(); i++) {
		AbstractViewTransformNotifier* pItem = dynamic_cast<AbstractViewTransformNotifier*>(items_list.at(i));
		if(pItem) {
			pItem->ViewTransformEvent(transform());
		}
	}
}
