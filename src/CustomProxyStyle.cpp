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
#include "CustomProxyStyle.h"
#include <QPainter>
#include <QBitmap>


QPixmap CustomProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &rPixmap, const QStyleOption *pOpt) const {

	QPixmap ret(rPixmap);
	if(iconMode == QIcon::Disabled) {
		QImage image = rPixmap.toImage();
		image = image.convertToFormat(QImage::Format_ARGB32);
		for(int y = 0; y < image.height(); ++y) {
			for(int x = 0; x < image.width(); ++x) {
				int pixel = image.pixel(x, y);
				int gray = qGray(pixel);
				int alpha = qAlpha(pixel);
				image.setPixel(x, y, qRgba(gray, gray, gray, alpha));
			}
		}
		ret = QPixmap::fromImage(image);
	}
	else {
		ret = QProxyStyle::generatedIconPixmap(iconMode, rPixmap, pOpt);
	}
	return ret;
}
