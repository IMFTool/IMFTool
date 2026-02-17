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
#include "ImfCommon.h"
#include "GraphicsCommon.h"
#include <QGraphicsWidget>
#include <QUuid>
#include <QGraphicsLinearLayout>


class QButtonGroup;
class QAbstractButton;
class GraphicsWidgetSegment;
class GraphicsWidgetSegmentIndicator;
class AbstractGraphicsWidgetResource;

class GraphicsWidgetTimeline : public GraphicsWidgetBase {

	Q_OBJECT
		
private:
	class GraphicsWidgetDrawnTimeline : public GraphicsWidgetBase {

	public:
		GraphicsWidgetDrawnTimeline(GraphicsWidgetTimeline *pParent);
		virtual ~GraphicsWidgetDrawnTimeline() {}
		virtual int type() const { return GraphicsWidgetDrawnTimelineType; }
		virtual void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = NULL);

	protected:
		virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
	};

public:
	GraphicsWidgetTimeline(QGraphicsItem *pParent = NULL);
	virtual ~GraphicsWidgetTimeline() {}
	virtual int type() const { return GraphicsWidgetTimelineType; }
	void AddSegmentIndicator(GraphicsWidgetSegmentIndicator *pSegmentIndicator, int SegmentIndicatorIndex);
	void MoveSegmentIndicator(GraphicsWidgetSegmentIndicator *pSegmentIndicator, int NewSegmentIndicatorIndex);
	//! Doesn't delete pSegment. pSegment is still a child of this.
	void RemoveSegmentIndicator(GraphicsWidgetSegmentIndicator *pSegmentIndicator);
	GraphicsWidgetSegmentIndicator* GetSegmentIndicator(int SegmentIndicatorIndex) const;
	GraphicsWidgetSegmentIndicator* GetSegmentIndicator(const QUuid &rId) const;
	int GetSegmentIndicatorCount() const;
	int GetSegmentIndicatorIndex(GraphicsWidgetSegmentIndicator *pSegmentIndicator) const;

signals:
	void NewSegmentRequest(int SegmentIndex);
	void DeleteSegmentRequest(const QUuid &rId);

	public slots:
	void SetHeight(qint32 height);

private slots:
void rButtonClicked(QAbstractButton *pButton);

protected:
	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
	virtual void resizeEvent(QGraphicsSceneResizeEvent *pEvent);
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent);

private:
	Q_DISABLE_COPY(GraphicsWidgetTimeline);
	void InitLayout();

	QGraphicsLinearLayout *mpTimelineLayout;
	QGraphicsLinearLayout *mpSegmentLayout;
	GraphicsWidgetDrawnTimeline *mpTimeline;
	QButtonGroup *mpButtonGroup;
	qint32 mHeight;
};
