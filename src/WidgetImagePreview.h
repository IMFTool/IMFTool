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
#include <QOpenGLWidget>
#include <QThread>
#include "ImfCommon.h"
#include <QPainter>

class QLabel;
class QOpenGLFunctions;

class WidgetImagePreview : public QOpenGLWidget {

	Q_OBJECT

public:
	WidgetImagePreview();
	~WidgetImagePreview() {}
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
	virtual bool hasHeightForWidth() const;
	virtual int heightForWidth(int w) const;
	virtual void resizeGL(int w, int h);
	virtual void paintGL();
	void Clear();
	void setSmoothing(bool);
	void setScaling(bool);
	void setExtract(int);
	void saveImage();

	QVector<TTMLRegion> ttml_regions; // list of TTML regions

public slots:

	void ShowImage(const QImage&);
	void paintRegions(QPainter &painter, const QRect rect_viewport, const QSize frame_size);
	void InitLayout();
	void regionOptionsChanged(int);
	void toggleFullScreen();

signals:
	void keyPressed(QKeyEvent *pEvent);

protected:

	virtual void initializeGL();
	virtual void mouseDoubleClickEvent(QMouseEvent *pEvent);
	virtual void keyPressEvent(QKeyEvent *pEvent);

private:
	Q_DISABLE_COPY(WidgetImagePreview);

	QOpenGLFunctions *f;
	QPainter painter;
	QRect rect_viewport;
	QSize frame_size;
	QRect draw_rect;

	QImage mImage;
	QFileInfo mLastFile;
	QImage nullimage;

	int px_ratio = 1;
	int viewport_width;
	int viewport_height;
	int frame_width;
	int frame_height;

	bool smooth = true;
	bool scaling = true;
	bool show_ttml_regions = true; // default
	int extract_area = 0; // default (top-left)
};
