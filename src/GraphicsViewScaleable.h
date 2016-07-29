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
#pragma once
#include <QGraphicsView>


class GraphicsViewScaleable : public QGraphicsView {

	Q_OBJECT

public:
	GraphicsViewScaleable(QWidget *pParent = NULL);
	virtual ~GraphicsViewScaleable() {}

	public slots:
	void ZoomIn();
	void ZoomOut();
	void ScaleView(qreal scaleFactor);

private:
	Q_DISABLE_COPY(GraphicsViewScaleable);

	QGraphicsScene	*mpGraphicsScene;
};


/*! Graphics items that want to ignore the view transformation should initialize their own transformation (QGraphicsItem::setTransform) in their ctor.
E.g.: setTransform(ViewTransformNotifier::GetViewTransform().inverted());
The ViewTransformNotifier::ViewTransformEvent() is only invoced if the view transform changes.
*/
class AbstractViewTransformNotifier {

	friend void GraphicsViewScaleable::ScaleView(qreal scaleFactor);

public:
	QTransform GetViewTransform() const { return (GetObservableView() ? GetObservableView()->transform() : QTransform()); }

protected:
	virtual void ViewTransformEvent(const QTransform &rViewTransform) {}
	virtual QGraphicsView* GetObservableView() const = 0;
};
