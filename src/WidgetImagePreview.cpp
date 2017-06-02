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
#include "WidgetImagePreview.h"
#include "global.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QApplication>
#include <QDesktopWidget>
#include "openjpeg.h"
#include <QOpenGLTexture>
#include <QFileDialog>
#include <QKeyEvent>
#include <QWindow>


WidgetImagePreview::WidgetImagePreview() {

	InitLayout();
	Clear();
}

void WidgetImagePreview::saveImage() {

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QString("export.bmp"),
		tr("Images (*.png *.bmp *.jpg)"));

	mImage.save(fileName);
}

void WidgetImagePreview::InitLayout() {

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setAutoFillBackground(false);
	setFocusPolicy(Qt::ClickFocus);
}

QSize WidgetImagePreview::sizeHint() const {

	QRect size = QApplication::desktop()->availableGeometry();
	return QSize(size.width() * 0.10, size.width()* 0.10 * 9. / 16.);
}

QSize WidgetImagePreview::minimumSizeHint() const {

	return QSize(320, 180);
}

int WidgetImagePreview::heightForWidth(int w) const {

	if (mImage.isNull() == false) {
		return w * mImage.height() / mImage.width();
	}
	return w * 9 / 16;
}

bool WidgetImagePreview::hasHeightForWidth() const {

	return true; // TODO doesn't work
}


void WidgetImagePreview::resizeGL(int w, int h) {

	px_ratio = QApplication::desktop()->devicePixelRatio();

	f = QOpenGLContext::currentContext()->functions();
	f->glViewport(0, 0, (GLint)w, (GLint)h);
	repaint();
}

void WidgetImagePreview::paintGL() {

	f = QOpenGLContext::currentContext()->functions();
	f->glClear(GL_COLOR_BUFFER_BIT);

	
	painter.begin(this);

	if(smooth) painter.setRenderHint(QPainter::SmoothPixmapTransform);
	
	// scale frame & viewport according to clients dpi settings
	frame_size = QSize((int)(mImage.width() ), (int)(mImage.height() ));
	rect_viewport = QRect(0, 0, (int)(painter.viewport().width()/px_ratio ), (int)(painter.viewport().height()/px_ratio ));

	if (scaling) {
		frame_size.scale(rect_viewport.size(), Qt::KeepAspectRatio); // scale image to fit preview-window
		draw_rect = QRect(QPoint((rect_viewport.width() - frame_size.width()) / 2, (rect_viewport.height() - frame_size.height()) / 2), frame_size);
	}
	else {
		// don't scale image to preview-window's size
		
		// check which part to extract (in case of overflowing)
		switch (extract_area) {
		case 0: // top left (default)
			draw_rect = QRect(QPoint(0, 0), mImage.size()); 
			break;
		case 1: // top center
			draw_rect = QRect(QPoint(-((frame_size.width() - rect_viewport.width()) / 2), 0), mImage.size());
			break;
		case 2: // top right
			draw_rect = QRect(QPoint(-(frame_size.width() - rect_viewport.width()), 0), mImage.size());
			break;
		case 3: // center left
			draw_rect = QRect(QPoint(0, -((frame_size.height() - rect_viewport.height()) / 2)), mImage.size());
			break;
		case 4: // center center
			draw_rect = QRect(QPoint(-((frame_size.width() - rect_viewport.width()) / 2), -((frame_size.height() - rect_viewport.height()) / 2)), mImage.size());
			break;
		case 5: // center right
			draw_rect = QRect(QPoint(-(frame_size.width() - rect_viewport.width()), -((frame_size.height() - rect_viewport.height()) / 2)), mImage.size());
			break;
		case 6: // bottom left
			draw_rect = QRect(QPoint(0, -(frame_size.height() - rect_viewport.height())), mImage.size());
			break;
		case 7: // bottom center
			draw_rect = QRect(QPoint(-((frame_size.width() - rect_viewport.width()) / 2), -(frame_size.height() - rect_viewport.height())), mImage.size());
			break;
		case 8: // bottom right
			draw_rect = QRect(QPoint(-(frame_size.width() - rect_viewport.width()), -(frame_size.height() - rect_viewport.height())), mImage.size());
			break;
		}
	}

	// draw current image
	painter.drawImage(draw_rect, mImage);

	// draw regions
	paintRegions(painter, rect_viewport, frame_size);

	painter.end();
}

void WidgetImagePreview::paintRegions(QPainter &painter, const QRect rect_viewport, const QSize frame_size) {

	if (ttml_regions.length() > 0) {

		float left = (rect_viewport.width() - (float)frame_size.width()) / 2;
		float top = (rect_viewport.height() - (float)frame_size.height()) / 2;

		// loop regions
		for (int i = 0; i < ttml_regions.length(); i++) {

			int region_left = qRound(ttml_regions[i].origin[0] * ((float)frame_size.width() / 100) + left);
			int region_top = qRound(ttml_regions[i].origin[1] * ((float)frame_size.height() / 100) + top);
			int region_width = qRound(ttml_regions[i].extent[0] * ((float)frame_size.width() / 100));
			int region_height = qRound(ttml_regions[i].extent[1] * ((float)frame_size.height() / 100));

			if (!ttml_regions[i].bgImage.isNull()) {
				// set background image
				ttml_regions[i].bgImageScaled = ttml_regions[i].bgImage.scaled(QSize(region_width, region_height),Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				painter.drawImage(QPoint(region_left, region_top), ttml_regions[i].bgImageScaled);
			}
			else if(show_ttml_regions){
				painter.setBrush(QBrush(ttml_regions[i].bgColor));
				QRect rect(region_left, region_top, region_width, region_height);
				painter.drawRect(rect);
			}
		}
	}
}

void WidgetImagePreview::toggleFullScreen() {

	if (isFullScreen() == false) {
		setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
		auto const desktop(QApplication::desktop());
		setGeometry(desktop->screenGeometry(desktop->numScreens() - 1));
		windowHandle()->setScreen(qApp->screens()[desktop->numScreens() - 1]);
		showFullScreen();
	}
	else {
		setWindowFlags(Qt::Widget);
		showNormal();
	}
}

void WidgetImagePreview::initializeGL() {

	QOpenGLFunctions *f = context()->functions();
	f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
	f->glClear(GL_COLOR_BUFFER_BIT);
}

void WidgetImagePreview::mouseDoubleClickEvent(QMouseEvent *pEvent) {
	toggleFullScreen();
	return;
}

void WidgetImagePreview::ShowImage(const QImage &rImage) {
	//qDebug() << "image received -> show it!" << rImage.byteCount();
	mImage = rImage;
	repaint();
}

void WidgetImagePreview::Clear() {
	
	ttml_regions.clear();
	mImage = nullimage;
	repaint();
}

void WidgetImagePreview::keyPressEvent(QKeyEvent *pEvent) {

	if (pEvent->key() == Qt::Key_Escape) {
		if (isFullScreen() == true) {
			setWindowFlags(Qt::Widget);
			showNormal();
		}
		return;
	}
	emit keyPressed(pEvent);
}


void WidgetImagePreview::setSmoothing(bool rsmooth) {
	smooth = rsmooth;
	repaint();
}

void WidgetImagePreview::setScaling(bool rscale) {
	scaling = rscale;
	repaint();
}

void WidgetImagePreview::setExtract(int rextract) {
	extract_area = rextract;
	//qDebug() << "extract" << extract_area;
	repaint();
}

void WidgetImagePreview::regionOptionsChanged(int state) {

	if (state == 0) {
		show_ttml_regions = false;
	}
	else {
		show_ttml_regions = true;
	}
	repaint();
}
