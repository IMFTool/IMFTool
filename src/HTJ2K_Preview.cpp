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

#define DEBUG_HT

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

		prec_shift = src_bitdepth - 8;
		max = (1 << src_bitdepth) - 1;


		if (mMxf_path != "") { // close old asset/reader
			reader->Close();
			reader->~MXFReader();
		}

		mMxf_path = asset->GetPath().absoluteFilePath(); // get new path

		reader = new AS_02::JP2K::MXFReader(); // create new reader

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
	mpCodestream->set_planar(false);

    if (siz.get_num_components() != 3) {
        qDebug() << "HTJ2K: Number of components is " << QString::number(siz.get_num_components());
    	mMsg = "HTJ2K: Number of components is " + QString::number(siz.get_num_components());
    	err = true;
    	return false;
    }
	bool all_same = true;
    ojph::point p = siz.get_downsampling(0);
    for (int i = 1; i < siz.get_num_components(); ++i)
    {
      ojph::point p1 = siz.get_downsampling(i);
      all_same = all_same && (p1.x == p.x) && (p1.y == p.y);
    }
	if (!all_same) {
        qDebug() << "HTJ2K: components do not have identical downsampling ratios";
    	mMsg = "HTJ2K: components do not have identical downsampling ratios";
    	err = true;
    	return false;
    }

    for (int i = 0; i < h; ++i)
    {
      for (int c = 0; c < siz.get_num_components(); ++c)
      {
        int comp_num;
        ojph::line_buf *line = mpCodestream->pull(comp_num);
        if (comp_num != c) {
        	mMsg = "Error during decoding";
        	qDebug() << "Error during decoding";
        	err = true;
        	return false;
        }
        const ojph::si32 *sp = line->i32;
        for (int ii = 0; ii < w; ii++)
        {
          int val = *sp++;
          val = val >= 0 ? val : 0; //decoded image may contain negative values, when layers are skipped
          val = val <= max ? val : max;
          mpOutBuf[c][i*w + ii] = (ojph::ui16)val;
        }

      }
    }
    mpMemBuf->close();
    mpCodestream->close();
    mpCodestream->~codestream();
    buff->~FrameBuffer();


    return true;

}

void HTJ2K_Preview::cleanUp() {

}

QImage HTJ2K::DataToQImage()
{

	QImage image(w, h, QImage::Format_RGB888); // create image

	bytes_per_line = w * 3;
	img_buff = new unsigned char[bytes_per_line];

	int offset = 16 << (src_bitdepth - 8);
	int maxcv = (1 << src_bitdepth) - 1;
	if (convert_to_709) {

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {

				switch (ColorEncoding) {
				case Metadata::RGBA:
					xpos = (y*w + x);
					// get three color components
					cv_comp1 = mpOutBuf[0][xpos];
					cv_comp2 = mpOutBuf[1][xpos];
					cv_comp3 = mpOutBuf[2][xpos];

					// clamp values between 0...maxcv
					if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 > maxcv) cv_comp1 = maxcv;
					if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 > maxcv) cv_comp2 = maxcv;
					if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 > maxcv) cv_comp3 = maxcv;

					// Scale to full range 16 bit
					if (ComponentMinRef != 0) { //offset compensation for legal range signals
						cv_comp1 -= ComponentMinRef;
						cv_comp2 -= ComponentMinRef;
						cv_comp3 -= ComponentMinRef;
						cv_comp1 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						cv_comp2 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
						cv_comp3 *= (max_f -1) / (ComponentMaxRef - ComponentMinRef);
					} else { // faster for full scale input
						cv_comp1 = cv_comp1 << (bitdepth - src_bitdepth);
						cv_comp2 = cv_comp2 << (bitdepth - src_bitdepth);
						cv_comp3 = cv_comp3 << (bitdepth - src_bitdepth);
					}

					break;
				case Metadata::CDCI:
					qDebug() << "CDCI color encoding is not supported!";
					// no break!
				default:
					return QImage(":/frame_error.png");  // abort!
				}

				// linearize XYZ data
				switch (transferCharacteristics) {

				case SMPTE::TransferCharacteristic_CinemaMezzanineDCDM:
				case SMPTE::TransferCharacteristic_CinemaMezzanineDCDM_Wrong:
					cv_comp1 = eotf_DCDM[cv_comp1]; // convert to 16 bit linear
					cv_comp2 = eotf_DCDM[cv_comp2]; // convert to 16 bit linear
					cv_comp3 = eotf_DCDM[cv_comp3]; // convert to 16 bit linear
					break;

				case SMPTE::TransferCharacteristic_CinemaMezzanineLinear:
					break;

				case SMPTE::TransferCharacteristic_SMPTEST2084:
					cv_comp1 = eotf_PQi[cv_comp1]; // convert to 16 bit linear, scale by 100 such that 100 nits correspond to 65535
					cv_comp2 = eotf_PQi[cv_comp2]; // convert to 16 bit linear
					cv_comp3 = eotf_PQi[cv_comp3]; // convert to 16 bit linear
					cv_comp1 *= 100;
					cv_comp2 *= 100;
					cv_comp3 *= 100;
					// clamp values between 0...maxcv
					if (cv_comp1 < 0) cv_comp1 = 0; else if (cv_comp1 >= max_f) cv_comp1 = max_f - 1;
					if (cv_comp2 < 0) cv_comp2 = 0; else if (cv_comp2 >= max_f) cv_comp2 = max_f - 1;
					if (cv_comp3 < 0) cv_comp3 = 0; else if (cv_comp3 >= max_f) cv_comp3 = max_f - 1;
					break;

				case SMPTE::TransferCharacteristic_ITU2020:
					cv_comp1 = eotf_2020i[cv_comp1]; // convert to 16 bit linear
					cv_comp2 = eotf_2020i[cv_comp2]; // convert to 16 bit linear
					cv_comp3 = eotf_2020i[cv_comp3]; // convert to 16 bit linear
					break;

				case SMPTE::TransferCharacteristic_HLG_OETF:
					cv_comp1 = eotf_HLGi[cv_comp1]; // convert to 16 bit linear
					cv_comp2 = eotf_HLGi[cv_comp2]; // convert to 16 bit linear
					cv_comp3 = eotf_HLGi[cv_comp3]; // convert to 16 bit linear
					break;

				case SMPTE::TransferCharacteristic_ITU709:
					break;

				default:
					qDebug() << "Unsupported Transfer Characteristic: " << SMPTE::vTransferCharacteristic[transferCharacteristics];
					return QImage(":/frame_error.png"); // abort!
				}

				switch (colorPrimaries) {
				case SMPTE::ColorPrimaries_CinemaMezzanine:
					// convert from XYZ -> BT.709, matrix coefficients are scaled by 1024 to allow for integer processing
					out_ri = cv_comp1*3319 + cv_comp2*-1574 + cv_comp3*-511;
					out_gi = cv_comp1*-993 + cv_comp2*1921 + cv_comp3*43;
					out_bi = cv_comp1*57  + cv_comp2*-209 + cv_comp3*1082;
					break;

				case SMPTE::ColorPrimaries_ITU2020:
					// convert from BT.2020 -> BT.709, matrix coefficients are scaled by 1024 to allow for integer processing (CV 655 = 100 nits, CV 65535 = 10,000 nits)
					out_ri = cv_comp1*1700 + cv_comp2*-602 + cv_comp3*-75;
					out_gi = cv_comp1*-128 + cv_comp2*1160 + cv_comp3*-9;
					out_bi = cv_comp1*-19  + cv_comp2*-103 + cv_comp3*1146;
					break;

				case SMPTE::ColorPrimaries_ITU709:
					out_ri = cv_comp1 << 10;
					out_gi = cv_comp2 << 10;
					out_bi = cv_comp3 << 10;
					break;

				default:
					qDebug() << "Unsupported Color Primaries: " << SMPTE::vColorPrimaries[colorPrimaries];
					return QImage(":/frame_error.png"); // abort!
				}

				// convert back to a 16 bit representation
				out_ri = out_ri >> 10;
				out_gi = out_gi >> 10;
				out_bi = out_bi >> 10;

				// clamp values between 0...65535
				if (out_ri < 0) out_ri = 0;	else if (out_ri >= max_f) out_ri = max_f - 1;
				if (out_gi < 0) out_gi = 0;	else if (out_gi >= max_f) out_gi = max_f - 1;
				if (out_bi < 0) out_bi = 0;	else if (out_bi >= max_f) out_bi = max_f - 1;

				//apply 709 OETF
				out_r8 = oetf_709i[out_ri];
				out_g8 = oetf_709i[out_gi];
				out_b8 = oetf_709i[out_bi];

				// set data
				buff_pos = x * 3; // don't calculate 3 times
				img_buff[buff_pos] = (unsigned char)out_r8;
				img_buff[buff_pos + 1] = (unsigned char)out_g8;
				img_buff[buff_pos + 2] = (unsigned char)out_b8;
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
	// YCbCr: not supported
	else if (ColorEncoding == Metadata::eColorEncoding::CDCI) {
		qDebug() << "HTJ2K: CDCI color encoding is not supported by IMF Tool!" ;
		return QImage(":/frame_error.png"); // abort!

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

