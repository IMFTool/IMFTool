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
#include "ACES_Preview.h"
#include <QThread>
#include <QTime>
#include "AS_DCP_internal.h"

#include "ACES.h"
#include "AS_02_ACES.h"

using namespace Imf;
using namespace Imath;

// timeline preview constructor
ACES_Preview::ACES_Preview() {
	setUp();
}

ACES_Preview::~ACES_Preview()
{
	if(!current_asset.isNull())
	{
		reader->Close();
		reader->~MXFReader();
	}else
	{
		qDebug() << "no reader found!";
	}
}

void ACES_Preview::setUp() {

	mDecode_time.start();
	mScale = 0;
}

void ACES_Preview::getProxy() {


	setAsset(); // initialize reader
	QImage p1, p2;

	// FIRST PROXY
	//if(extractFrame(0)) {			// Frame Nr manuell auf 0 gesetzt
	if (extractFrame(mFirst_proxy)) { 
		p1 = DataToQImage(3, true);
	}
	else {
		p1 = QImage(":/proxy_unknown.png");
	}
	
	// SECOND PROXY
	if (extractFrame(mSecond_proxy)) { // frame extraction was successful -> decode frame
		p2 = DataToQImage(3, true);
	}
	else {
		p2 = QImage(":/proxy_unknown.png");
	}

	emit proxyFinished(p1, p2);
	emit finished();
}

// set decoding layer
void ACES_Preview::setLayer(int index) {

	mScale = index;
}

// Show Active Area (true) or Show Native Resolution (false)
void ACES_Preview::showActiveArea(bool rShowActiveArea) {

	mShowActiveArea = rShowActiveArea;
}



void ACES_Preview::setAsset() {

	if (asset && !asset.isNull()) {

		current_asset = asset;

		// get color space
		colorPrimaries = asset->GetMetadata().colorPrimaries;
		switch (colorPrimaries) {
		case SMPTE::ColorPrimaries_ACES:
			//ACES is 4:4:4 only
			break;
		default:
			mMsg = "Unknown color encoding"; // ERROR
			err = true;
			break; // abort!
		}

		if (mMxf_path != "") { // close old asset/reader
			reader->Close();
			reader->~MXFReader();
		}

		mMxf_path = asset->GetPath().absoluteFilePath(); // get new path

		reader = new AS_02::ACES::MXFReader(defaultFactory); // create new reader

		Result_t result_o = reader->OpenRead(mMxf_path.toStdString()); // open file for reading
		if (!ASDCP_SUCCESS(result_o)) {
			mMsg = QString("Failed to init. reader: %1").arg(result_o.Message()); // ERROR
			err = true;
		}
	}
	else {
		mMsg = "Asset is invalid!"; // ERROR
		err = true;
	}
}

void ACES_Preview::decode() {

#ifdef DEBUG_JP2K
	qDebug() << "begin decoding image nr. " << mFrameNr;
#endif


	err = false; // reset
	mDecode_time.start(); // start calculating decode time

	if (operator!=(asset, current_asset) || !asset) { // asset changed!!
		setAsset();
	}

	if (!err && extractFrame(mFrameNr)) { // frame extraction was successful -> decode frame

		// try to decode image
		if ( !err) {
			emit ShowFrame(DataToQImage(mScale, mShowActiveArea));

			if (!err) mMsg = QString("Decoded frame %1 in %2 ms").arg(mFrameNr).arg(mDecode_time.elapsed()); // no error

			emit decodingStatus(mFrameNr, mMsg);
#ifdef DEBUG_JP2K
			qDebug() << "JP2K_Preview::decode" << mFrameNr;
#endif
			QApplication::processEvents();
			//cleanUp();
			emit finished();
		}
		else { // error decoding image
			emit ShowFrame(QImage(":/frame_error.png"));
			emit decodingStatus(mFrameNr, mMsg);
			emit finished();
		}
	}
	else {
		emit ShowFrame(QImage(":/frame_blank.png")); // show empty image
		emit decodingStatus(mFrameNr, mMsg);
		emit finished();
	}
}


bool ACES_Preview::extractFrame(qint64 frameNr) {

	// create new buffer
	buff = new AS_02::ACES::FrameBuffer();
	bool retValue = false;

	// calculate neccessary buffer size
	if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup((frameNr + 1), IndexF2))) { // next frame
		if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup(frameNr, IndexF1))) { // current frame
			if (!ASDCP_SUCCESS(buff->Capacity((IndexF2.StreamOffset - IndexF1.StreamOffset) - 20))) { // set buffer size

				mMsg = QString("Could not set FrameBuffer size to: %1").arg((IndexF2.StreamOffset - IndexF1.StreamOffset) - 20); // ERROR
				err = true;
				return false;
			}
		}
		else {
			mMsg = QString("Frame does not exist: %1").arg(frameNr); // ERROR
			err = true;
			return false;
		}
	}
	else {
		// ACES frames are of equal length, use size of previous frame
		ASDCP::MXF::IndexTableSegment::IndexEntry IndexPrevious;
		if (ASDCP_SUCCESS(reader->AS02IndexReader().Lookup((frameNr - 1), IndexPrevious))) { // previous frame
			if (!ASDCP_SUCCESS(buff->Capacity((IndexF1.StreamOffset - IndexPrevious.StreamOffset)))) {
				buff->Capacity(default_buffer_size); // set default size
			}
		} else
			buff->Capacity(default_buffer_size); // set default size
	}
	
	// try reading requested frame number
	if (ASDCP_SUCCESS(reader->ReadFrame(frameNr, *buff, NULL, NULL))) {
		pAcesIStream = new As02AcesIStream();
		pAcesIStream->InitBuffer(*buff);
		retValue = true;
	}
	else {
		mMsg = "Error reading frame!"; // ERROR
		err = true;
	}
	delete buff;

	return retValue;
}


QImage ACES::DataToQImage(quint8 rScale /* = 0 */, bool rShowDisplayWindow /* = false*/)
{
	bool show_display_window = rShowDisplayWindow;
	quint8 scale = 1 << rScale;
	RgbaInputFile mRgbaFile(*pAcesIStream);
	Box2i data_window;
	Box2i display_window;

	data_window = mRgbaFile.dataWindow();
	long data_w = data_window.max.x - data_window.min.x + 1;
	long data_h = data_window.max.y - data_window.min.y + 1;

	display_window = mRgbaFile.displayWindow();
	long display_w = display_window.max.x - display_window.min.x + 1;
	long display_h = display_window.max.y - display_window.min.y + 1;
	long offset_x = display_window.min.x - data_window.min.x;
	long offset_y = display_window.min.y - data_window.min.y;

	if (!data_window.intersects(display_window)) {
		show_display_window = false; // If dataWindow is smaller than displayWindow: show dataWindow
	}

	if (!show_display_window) { //Ignore displayWindow
		display_w = data_w;
		display_h = data_h;
		offset_x = 0;
		offset_y = 0;
	}

	display_w /= scale;
	display_h /= scale;

	long adjusted_height;
	if (show_display_window && (mRgbaFile.pixelAspectRatio() != 0.0)) {
		adjusted_height = qRound((float)display_h / mRgbaFile.pixelAspectRatio());
	} else {
		adjusted_height = display_h;
	}

	QImage image;

	Array2D<Rgba> inPixelsDataWindow(data_h, data_w);

	mRgbaFile.setFrameBuffer(&inPixelsDataWindow[0][0] - data_window.min.x - data_window.min.y * data_w, 1, data_w);
	mRgbaFile.readPixels(data_window.min.y, data_window.max.y);

	image = QImage(display_w, display_h, QImage::Format_RGB888); // create image

	for (int x = 0; x < display_w; x++) {
		for (int y = 0; y < display_h; y++) {
			Rgba pix = inPixelsDataWindow[y*scale + offset_y][x*scale + offset_x];
			Vec3<float> vec3in(pix.r, pix.g, pix.b);
			Vec3<float> vec3out;
			//vec3out.x = 0.1; vec3out.y = 0.2; vec3out.z = 0.3;
			A0Rec709TransformMatrix.multVecMatrix(vec3in, vec3out);
			int r,g,b;
			r = max_f_*vec3out[0]; if (r>=max_f) r = max_f - 1; if (r<0) r = 0;
			g = max_f_*vec3out[1]; if (g>=max_f) g = max_f - 1; if (g<0) g = 0;
			b = max_f_*vec3out[2]; if (b>=max_f) b = max_f - 1; if (b<0) b = 0;
			r = oetf_709[r];
			g = oetf_709[g];
			b = oetf_709[b];
			image.setPixelColor(x, y, QColor(r,g,b));
		}
	}
	if (show_display_window)
		image = image.scaled(display_w, adjusted_height);
	// Clean up
	if (pAcesIStream) pAcesIStream->~As02AcesIStream();
	return image;
}

Chromaticities ACES::aces_chromaticities = Chromaticities(V2f(0.7347f, 0.2653f), V2f(0.0f, 1.0f), V2f(0.0001f, -0.077f), V2f(0.32168f, 0.33767f));

M44f ACES::A0Rec709TransformMatrix = RGBtoXYZ(aces_chromaticities, 1.0) * XYZtoRGB(Chromaticities(), 1.0);
