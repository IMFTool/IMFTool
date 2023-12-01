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
#include "As02AcesIStream.h"
#include "global.h"


As02AcesIStream::As02AcesIStream() : Imf::IStream("Unknown") {

	d = new As02AcesIStreamData;
}

As02AcesIStream::As02AcesIStream(const As02AcesIStream &rOther) : Imf::IStream("Unknown"), d(rOther.d) {

}

bool As02AcesIStream::read(char c[], int n) {

	if(d->mpReadPosition - d->mpData + n > d->mSize) return false;
	memcpy(c, d->mpReadPosition, n);
	d->mpReadPosition += n;
	return true;
}

uint64_t As02AcesIStream::tellg() {

	return (d->mpReadPosition - d->mpData);
}

void As02AcesIStream::seekg(uint64_t pos) {

	if(pos > (uint64_t)d->mSize) return;
	d->mpReadPosition = d->mpData + pos;
}

void As02AcesIStream::clear() {

	//qDebug() << "Clear was called. Nothing to do here.";
}

void As02AcesIStream::InitBuffer(const AS_02::ACES::FrameBuffer &rBuffer) {

	d->mSize = rBuffer.Size();
	d->mpData = (byte_t*)malloc(d->mSize);
	if(d->mpData != NULL) memcpy(d->mpData, rBuffer.RoData(), d->mSize);
	d->mpReadPosition = d->mpData;
}

As02AcesIStreamData::As02AcesIStreamData() :
mpData(NULL), mSize(0), mpReadPosition(NULL) {

}

As02AcesIStreamData::As02AcesIStreamData(const As02AcesIStreamData &rOther) :
mpData(NULL), mSize(0), mpReadPosition(NULL) {

	if(rOther.mpData != NULL) {
		mSize = rOther.mSize;
		if(mSize > 0) mpData = (byte_t*)malloc(mSize);
		if(mpData != NULL) memcpy(mpData, rOther.mpData, mSize);
		mpReadPosition = mpData;
	}
}

As02AcesIStreamData::~As02AcesIStreamData() {

	free(mpData);
}
