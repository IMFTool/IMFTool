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
	
	delete oetf_709;
	delete eotf_2020;
	delete eotf_PQ;
}

void ACES_Preview::setUp() {

	mDecode_time.start();

	max_f = 1 << bitdepth;
	max_f_ = (float)(max_f)-1.0;

	oetf_709 = new float[max_f];

	for (int i = 0; i < max_f; i++) {

		float input = (float)(i / max_f_); // convert input to value between 0...1

		// BT.709 - OETF
		oetf_709[i] = pow(input, 1.0f / 2.4f);
	}


}

void ACES_Preview::getProxy() {


	mCpus = 1; // (default for proxys)
	convert_to_709 = false; // (default for proxys)
	params.cp_reduce = 4; // (default for proxy)

	setAsset(); // initialize reader
	QImage p1, p2;

	// FIRST PROXY
	//if(extractFrame(0)) {			// Frame Nr manuell auf 0 gesetzt
	if (extractFrame(mFirst_proxy)) { 
		p1 = DataToQImage();
	}
	else {
		p1 = QImage(":/proxy_unknown.png");
	}
	
	// SECOND PROXY
	if (extractFrame(mSecond_proxy)) { // frame extraction was successful -> decode frame
		p2 = DataToQImage();
	}
	else {
		p2 = QImage(":/proxy_unknown.png");
	}

	emit proxyFinished(p1, p2);
	emit finished();
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

		reader = new AS_02::ACES::MXFReader(); // create new reader

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
	mDecode_time.restart(); // start calculating decode time

	if (operator!=(asset, current_asset) || !asset) { // asset changed!!
		setAsset();
	}

	if (!err && extractFrame(mFrameNr)) { // frame extraction was successfull -> decode frame

		// try to decode image
		if ( !err) {

			emit ShowFrame(DataToQImage());

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
		buff->Capacity(default_buffer_size); // set default size
	}
	
	// try reading requested frame number
	if (ASDCP_SUCCESS(reader->ReadFrame(frameNr, *buff, NULL, NULL))) {
		pAcesIStream = new As02AcesIStream();
		pAcesIStream->InitBuffer(*buff);
		return true;
	}
	else {
		mMsg = "Error reading frame!"; // ERROR
		err = true;
		return false;
	}

	return false; // default
}


QImage ACES::DataToQImage()
{

	Imf::RgbaInputFile mRgbaFile(*pAcesIStream);

	const Box2i &dw = mRgbaFile.dataWindow();
	int w = dw.max.x - dw.min.x + 1;
	int h = dw.max.y - dw.min.y + 1;
	QImage image(w, h, QImage::Format_RGB888); // create image


	Array2D<Rgba> inPixels(h, w);

	mRgbaFile.setFrameBuffer(&inPixels[0][0] - dw.min.x - dw.min.y * w, 1, w);
	mRgbaFile.readPixels(dw.min.y, dw.max.y);
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			Rgba pix = inPixels[y][x];
			Imath::Vec3<float> vec3in(pix.r, pix.g, pix.b);
			Imath::Vec3<float> vec3out;
			A0Rec709TransformMatrix.multVecMatrix(vec3in, vec3out);
			int r,g,b,a;
			r = max_f_*vec3out[0]; if (r>=max_f) r = max_f - 1; if (r<0) r = 0;
			g = max_f_*vec3out[1]; if (g>=max_f) g = max_f - 1; if (g<0) g = 0;
			b = max_f_*vec3out[2]; if (b>=max_f) b = max_f - 1; if (b<0) b = 0;
			r = oetf_709[r] * 255.0;
			g = oetf_709[g] * 255.0;
			b = oetf_709[b] * 255.0;
			image.setPixelColor(x, y, QColor(r,g,b));
		}
	}
	return image;
}


void ACES::info_callback(const char *mMsg, void *client_data) {
	qDebug() << "INFO" << mMsg;
}

void ACES::warning_callback(const char *mMsg, void *client_data) {
	qDebug() << "WARNING" << mMsg;
}

void ACES::error_callback(const char *mMsg, void *client_data) {
	qDebug() << "ERROR" << mMsg;
}

Imf::Chromaticities ACES::aces_chromaticities = Chromaticities(V2f(0.7347f, 0.2653f), V2f(0.0f, 1.0f), V2f(0.0001f, -0.077f), V2f(0.32168f, 0.33767f));

Imath::M44f ACES::A0Rec709TransformMatrix = RGBtoXYZ(aces_chromaticities, 1.0) * XYZtoRGB(Chromaticities(), 1.0);


