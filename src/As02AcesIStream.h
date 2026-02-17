/* Archivist is a GUI application for interactive IMF Master Package creation.
 * Copyright(C) 2015 Björn Stresing
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
#include "ImfIO.h"
#include "AS_02_ACES.h"
#include <QSharedData>
#include <QExplicitlySharedDataPointer>
#include <QObject>


class As02AcesIStreamData : public QSharedData {

public:
	As02AcesIStreamData();
	~As02AcesIStreamData();
	As02AcesIStreamData(const As02AcesIStreamData &rOther);

	byte_t *mpData;
	i64_t  mSize;
	const byte_t *mpReadPosition;
};


class As02AcesIStream: public Imf::IStream {

public:
	As02AcesIStream();
	virtual ~As02AcesIStream() {}
	As02AcesIStream(const As02AcesIStream &rOther);
	virtual bool read(char c[], int n);
	virtual uint64_t tellg();
	virtual void seekg(uint64_t pos);
	virtual void clear();
	void InitBuffer(const AS_02::ACES::FrameBuffer &rBuffer);

private:
	QExplicitlySharedDataPointer<As02AcesIStreamData> d;
};


Q_DECLARE_METATYPE(As02AcesIStream)
