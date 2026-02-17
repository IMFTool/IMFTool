/* Copyright(C) 2020 Wolfgang Ruppel
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
#include <QObject>
#include <QRunnable>
#include <QDebug>
#include "HTJ2K_Preview.h"

class FrameRequest;

class HTJ2K_Decoder : public QObject, public QRunnable, public HTJ2K {

	Q_OBJECT

public:
	HTJ2K_Decoder(QSharedPointer<DecodedFrames>&, QSharedPointer<FrameRequest>&);
private:

	QSharedPointer<DecodedFrames> decoded_shared;
	QSharedPointer<FrameRequest> request;

protected:
	void run();
signals:
	void finished();
};
