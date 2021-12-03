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
#include "HTJ2K_Preview.h"
#include <QThread>
#include <QTime>
#include "AS_DCP_internal.h"

//#define DEBUG_HT

// timeline preview constructor
HTJ2K_Preview::HTJ2K_Preview() {
	setUp();
}

HTJ2K_Preview::~HTJ2K_Preview()
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

void HTJ2K_Preview::setUp() {

	mDecode_time.start();
	cp_reduce = 2; // (default)
}

void HTJ2K_Preview::getProxy() {

	convert_to_709 = false; // (default for proxys)
	cp_reduce = 4; // (default for proxy)

	setAsset(); // initialize reader
	QImage p1, p2;

	// FIRST PROXY
	if (!err && extractFrame(mFirst_proxy)) { // frame extraction was successfull -> decode frame
#ifdef DEBUG_HT
		qDebug() << "proxy 1";
#endif
		if (decodeImage()) { // try to decode image
			p1 = DataToQImage();
			cleanUp();
		}
		else {
			p1 = QImage(":/proxy_unknown.png");
		}
	}
	else {
		p1 = QImage(":/proxy_unknown.png");
	}
#ifdef DEBUG_HT
	qDebug() << "HTJ2K_Preview::getProxy!";
#endif

	// SECOND PROXY
	if (!err && extractFrame(mSecond_proxy)) { // frame extraction was successful -> decode frame
#ifdef DEBUG_HT
		qDebug() << "proxy 2";
#endif
		if (decodeImage()) { // try to decode image
			p2 = DataToQImage();
			cleanUp();
		}
		else {
			p2 = QImage(":/proxy_unknown.png");
		}
	}
	else {
		p2 = QImage(":/proxy_unknown.png");
	}
#ifdef DEBUG_HT
	qDebug() << "HTJ2K_Preview::getProxy!";
#endif

	emit proxyFinished(p1, p2);
	emit finished();
}

void HTJ2K_Preview::setLayer(int index) {

	cp_reduce = index;
#ifdef DEBUG_HT
	qDebug() << "HTJ2K_Preview::setLayer" << cp_reduce;
#endif
}

void HTJ2K_Preview::setAsset() {

	if (asset && !asset.isNull()) {

		current_asset = asset;

		colorPrimaries = asset->GetMetadata().colorPrimaries;
		transferCharacteristics = asset->GetMetadata().transferCharcteristics;
		ColorEncoding = asset->GetMetadata().colorEncoding;

		src_bitdepth = asset->GetMetadata().componentDepth;
		ComponentMinRef = asset->GetMetadata().componentMinRef;
		ComponentMaxRef = asset->GetMetadata().componentMaxRef;

		if (setCodingParameters() == false ) {
			mMsg = "Unknown color encoding"; // ERROR
			err = true;
		}

		if (mMxf_path != "") { // close old asset/reader
			reader->Close();
			reader->~MXFReader();
		}

		mMxf_path = asset->GetPath().absoluteFilePath(); // get new path

		reader = new AS_02::JP2K::MXFReader(defaultFactory); // create new reader

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

void HTJ2K_Preview::decode() {

#ifdef DEBUG_HT
	qDebug() << "begin decoding image nr. " << mFrameNr;
#endif

	err = false; // reset
	mDecode_time.restart(); // start calculating decode time

	if (operator!=(asset, current_asset) || !asset) { // asset changed!!
		setAsset();
	}

	if (!err && extractFrame(mFrameNr)) { // frame extraction was successful -> decode frame

		// try to decode image
		if (decodeImage() && !err) {

			emit ShowFrame(DataToQImage());

			if (!err) mMsg = QString("Decoded frame %1 in %2 ms").arg(mFrameNr).arg(mDecode_time.elapsed()); // no error

			emit decodingStatus(mFrameNr, mMsg);
#ifdef DEBUG_HT
			qDebug() << "HTJ2K_Preview::decode" << mFrameNr;
#endif
			QApplication::processEvents();
			cleanUp();
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

bool HTJ2K::extractFrame(qint64 frameNr) {

	// create new buffer
	buff = new ASDCP::JP2K::FrameBuffer();

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
	    mpMemBuf = new ojph::mem_infile;
	    mpMemBuf->open((ojph::ui8 *)buff->Data(), (int)buff->Size());
		return true;
	}
	else {
		mMsg = "Error reading frame!"; // ERROR
		err = true;
		return false;
	}

	return false; // default
}

bool HTJ2K::decodeImage() {

	mpCodestream = new ojph::codestream();
	mpCodestream->enable_resilience();
	mpCodestream->read_headers(mpMemBuf);
#ifdef DEBUG_HT
	qDebug() << "HTJ2K_Preview::decode" << cp_reduce;
#endif

	mpCodestream->restrict_input_resolution(cp_reduce, cp_reduce);
	ojph::param_siz siz = mpCodestream->access_siz();
	w = siz.get_recon_width(0);
    h = siz.get_recon_height(0);
    mpOutBuf[0] = new ojph::ui16[w * h]; // For X component
    mpOutBuf[1] = new ojph::ui16[w * h]; // For Y component
    mpOutBuf[2] = new ojph::ui16[w * h]; // For Z component
	
	mpCodestream->create();

    if (siz.get_num_components() != 3) {
		qDebug() << "HTJ2K: Number of components is " << QString::number(siz.get_num_components());
		mMsg = "HTJ2K: Number of components is " + QString::number(siz.get_num_components());
		err = true;
		return false;
    }
	bool is_444 = true;
	bool is_422 = true;
	ojph::point p = siz.get_downsampling(0);
	for (int i = 1; i < siz.get_num_components(); ++i) {
		ojph::point p1 = siz.get_downsampling(i);
		is_444 = is_444 && (p1.x == p.x) && (p1.y == p.y);
		is_422 = is_422 && (p1.x == 2 * p.x) && (p1.y == p.y);
    }
	if (is_444) {
    	mpCodestream->set_planar(false);
	} else if(is_422) {
    	mpCodestream->set_planar(true);
	} else {
		qDebug() << "HTJ2K: Only 4:2:2 or 4:4:4 are supported!";
		mMsg = "HTJ2K: Only 4:2:2 or 4:4:4 are supported!";
		err = true;
		return false;
	}

    if (mpCodestream->is_planar()) {
		ojph::param_siz siz = mpCodestream->access_siz();
#ifdef DEBUG_HT
		qDebug() << src_bitdepth << max << w << h;
#endif
		for (int c = 0; c < siz.get_num_components(); ++c) {
			for (int i = 0; i < h; ++i) {
				try {
					int comp_num;
					ojph::line_buf *line = mpCodestream->pull(comp_num);
					if (comp_num != c) {
						mMsg = "Error during decoding";
						qDebug() << "Error during decoding";
						err = true;
						return false;
					}
					const ojph::si32 *sp = line->i32;
					if (c == 0) {
						for (int ii = 0; ii < w; ii++) {
							int val = *sp++;
							val = val >= 0 ? val : 0; //decoded image may contain negative values, when layers are skipped
							val = val <= max ? val : max;
							mpOutBuf[c][i*w + ii] = (ojph::ui16)val;
						}
					} else {
						for (int ii = 0; ii < w/2; ii++) {
							int val = *sp++;
							val = val >= 0 ? val : 0; //decoded image may contain negative values, when layers are skipped
							val = val <= max ? val : max;
							mpOutBuf[c][i*w + 2*ii] = (ojph::ui16)val;
							mpOutBuf[c][i*w + 2*ii + 1] = (ojph::ui16)val;
						}
					}
				}
				catch (const std::exception& e) {
					qDebug() << "OpenJPH exception caught!";
					if (e.what()) qDebug() << e.what();
					err = true;
					return false;
				}
			}
		}
    }
    else {
		for (int i = 0; i < h; ++i)	{
			for (int c = 0; c < siz.get_num_components(); ++c) {
				try {
					int comp_num;
					ojph::line_buf *line = mpCodestream->pull(comp_num);
					if (comp_num != c) {
						mMsg = "Error during decoding";
						qDebug() << "Error during decoding";
						err = true;
						return false;
					}
					const ojph::si32 *sp = line->i32;
					for (int ii = 0; ii < w; ii++) {
						int val = *sp++;
						val = val >= 0 ? val : 0; //decoded image may contain negative values, when layers are skipped
						val = val <= max ? val : max;
						mpOutBuf[c][i*w + ii] = (ojph::ui16)val;
					}
				}
				catch (const std::exception& e) {
					qDebug() << "OpenJPH exception caught!";
					if (e.what()) qDebug() << e.what();
					err = true;
					return false;
				}
			}
		}
    }
    mpMemBuf->close();
    mpCodestream->close();
    mpCodestream->~codestream();
	delete buff;


    return true;

}

void HTJ2K_Preview::cleanUp() {

}

QImage HTJ2K::DataToQImage()
{

	QImage image(w, h, QImage::Format_RGB888); // create image

	bytes_per_line = w * 3;
	img_buff = new unsigned char[bytes_per_line];

	if (convert_to_709) {

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				xpos = (y*w + x);
				switch (ColorEncoding) {
				case Metadata::RGBA:
					// get three color components
					cv_comp1 = mpOutBuf[0][xpos];
					cv_comp2 = mpOutBuf[1][xpos];
					cv_comp3 = mpOutBuf[2][xpos];

					// Scale to full range 16 bit
					if (ComponentMinRef != 0) { //offset compensation for legal range signals
						cv_comp1 -= ComponentMinRef;
						cv_comp2 -= ComponentMinRef;
						cv_comp3 -= ComponentMinRef;
						cv_comp1 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						cv_comp2 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						cv_comp3 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
						if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
						if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;
					} else { // faster for full scale input
						cv_comp1 = cv_comp1 << (bitdepth - src_bitdepth);
						cv_comp2 = cv_comp2 << (bitdepth - src_bitdepth);
						cv_comp3 = cv_comp3 << (bitdepth - src_bitdepth);
						if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
						if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
						if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;
					}

					break;
				case Metadata::CDCI:
					cv_compY =  mpOutBuf[0][xpos]  - offset_y;
					cv_compCb = mpOutBuf[1][xpos]  - (max + 1) / 2;
					cv_compCr = mpOutBuf[2][xpos]  - (max + 1) / 2;

					cv_compY =  cv_compY * max / range_y;
					cv_compCb = cv_compCb * max / range_c;
					cv_compCr = cv_compCr * max / range_c;

					cv_comp1 = cv_compY + ((2 * (1024 - Kr)*cv_compCr) / 1024);
					cv_comp2 = cv_compY - ((Kbg * cv_compCb + Krg * cv_compCr) / 1024);
					cv_comp3 = cv_compY + ((2 * (1024 - Kb)*cv_compCb) / 1024);

					if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max) cv_comp1 = max - 1;
					if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max) cv_comp2 = max - 1;
					if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max) cv_comp3 = max - 1;

					cv_comp1 = cv_comp1 << (bitdepth - src_bitdepth);
					cv_comp2 = cv_comp2 << (bitdepth - src_bitdepth);
					cv_comp3 = cv_comp3 << (bitdepth - src_bitdepth);
					break;
				default:
					return QImage(":/frame_error.png");  // abort!
				}

				if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
				if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
				if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;

				// linearize XYZ data
				if (linearize() == false) {
					qDebug() << "Unsupported Transfer Characteristic: " << SMPTE::vTransferCharacteristic[transferCharacteristics];
					return QImage(":/frame_error.png"); // abort!
				}
				if ((transferCharacteristics == SMPTE::TransferCharacteristic_ITU709) ||(transferCharacteristics == SMPTE::TransferCharacteristic_IEC6196624_xvYCC)) {
					// set data
					buff_pos = x * 3; // don't calculate 3 times
					img_buff[buff_pos] = (unsigned char)cv_comp1;
					img_buff[buff_pos + 1] = (unsigned char)cv_comp2;
					img_buff[buff_pos + 2] = (unsigned char)cv_comp3;
					continue;
				}

				if (colorTransform() == false) {
					qDebug() << "Unsupported Color Primaries: " << SMPTE::vColorPrimaries[colorPrimaries];
					return QImage(":/frame_error.png"); // abort!
				}


				//apply 709 OETF and set data
				buff_pos = x * 3; // don't calculate 3 times
				img_buff[buff_pos] = (unsigned char)oetf_709[out_ri];
				img_buff[buff_pos + 1] = (unsigned char)oetf_709[out_gi];
				img_buff[buff_pos + 2] = (unsigned char)oetf_709[out_bi];
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	else if (ColorEncoding == Metadata::eColorEncoding::RGBA) {
		// RGBA: keep original color encoding

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				xpos = (y*w + x); // don't calculate 3 times
				buff_pos = x * 3; // don't calculate 3 times

				img_buff[buff_pos] = mpOutBuf[0][xpos] >> prec_shift;
				img_buff[buff_pos + 1] = mpOutBuf[1][xpos] >> prec_shift;
				img_buff[buff_pos + 2] = mpOutBuf[2][xpos] >> prec_shift;
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	// YCbCr: Apply coding equations, keep color encoding
	else if (ColorEncoding == Metadata::eColorEncoding::CDCI) {
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				xpos = (y*w + x); // don't calculate 3 times
				buff_pos = x * 3; // don't calculate 3 times
				cv_compY =  mpOutBuf[0][xpos]  - offset_y;
				cv_compCb = mpOutBuf[1][xpos]  - (max + 1) / 2;
				cv_compCr = mpOutBuf[2][xpos]  - (max + 1) / 2;

				cv_compY =  (qint32)(cv_compY * max / range_y);
				cv_compCb = (qint32)(cv_compCb * max / range_c);
				cv_compCr = (qint32)(cv_compCr * max / range_c);

				cv_comp1 = cv_compY + ((2 * (1024 - Kr)*cv_compCr) / 1024);
				cv_comp2 = cv_compY - ((Kbg * cv_compCb + Krg * cv_compCr) / 1024);
				cv_comp3 = cv_compY + ((2 * (1024 - Kb)*cv_compCb) / 1024);

				if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max) cv_comp1 = max - 1;
				if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max) cv_comp2 = max - 1;
				if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max) cv_comp3 = max - 1;

				img_buff[buff_pos] = cv_comp1 >> prec_shift;
				img_buff[buff_pos + 1] = cv_comp2 >> prec_shift;
				img_buff[buff_pos + 2] = cv_comp3 >> prec_shift;
			}
			memcpy(image.scanLine(y), img_buff, bytes_per_line);
		}
	}
	else { // unknown ColorEncoding
		return QImage(":/frame_error.png");
	}

	delete[] img_buff;
	delete[] mpOutBuf[0];
	delete[] mpOutBuf[1];
	delete[] mpOutBuf[2];

	return image;
}

