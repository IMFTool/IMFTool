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
#include "ImfPackage.h"
#include <QMimeData>


class AbstractGraphicsWidgetResource;

class ImfMimeData : public QMimeData {

public:
	ImfMimeData() : QMimeData() {}
	virtual ~ImfMimeData() {}
	void SetAsset(const QSharedPointer<Asset> &rAsset) { mAsset = rAsset; }
	QSharedPointer<Asset> GetAsset() const { return mAsset; };

private:
	QSharedPointer<Asset> mAsset;
};
