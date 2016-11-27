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
#include "createLUTs.h"

/*
void createLUTs::run() {

	elapsed_time.restart();
	emit updateProgress(QString("creating luts..."));

	bool luts_to_create[7] = { false };
	
	// check which to create
	for (int i = 1; i <= 6; i++) {
		if (luts->luts[i] != nullptr && luts->luts[i]->initialized == false) {
			qDebug() << "create LUT index" << i;
			luts_to_create[i] = true;
		}
	}

	
	int out_1, out_2, out_3;
	int in_1, in_2, in_3, i;
	int tmp_1, tmp_2, tmp_3;

	// loop through all possible values
	for (in_1 = 0; in_1 < 256; in_1++) {
		for (in_2 = 0; in_2 < 256; in_2++) {
			for (in_3 = 0; in_3 < 256; in_3++) {

				// loop luts
				for (i = 0; i <= 6; i++) {
					if (luts_to_create[i]) {
						// create this LUT! -> which one ?
						switch (i) {
						case 1: // RGB709
							break;
						case 2: // RGB_2020_PQ

							// https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2087-0-201510-I!!PDF-E.pdf
							// Inverse of BT.2020 -> BT.709
							out_1 = (int)in_1*1.6605 + in_2*-0.5877 + in_3*-0.0728;
							out_2 = (int)in_1*-0.1246 + in_2*1.133 + in_3*-0.0084;
							out_3 = (int)in_1*-0.0182 + in_2*-0.1006 + in_3*1.1187;

							break;
						case 3: // RGB_P3D65
							break;
						case 4: // YUV_709

							// convert to rgb(http://softpixel.com/~cwright/programming/colorspace/yuv/)
							out_1 = (int)(in_1 + 1.4075 * (in_3 - 128));
							out_2 = (int)(in_1 - 0.3455 * (in_2 - 128) - (0.7169 * (in_3 - 128)));
							out_3 = (int)(in_1 + 1.7790 * (in_2 - 128));

							break;
						case 5: // YUV_2020_LIN



							break;
						case 6: // YUV_2020_PQ

							// convert to rgb
							tmp_1 = (int)(in_1 + 1.4075 * (in_3 - 128));
							tmp_2 = (int)(in_1 - 0.3455 * (in_2 - 128) - (0.7169 * (in_3 - 128)));
							tmp_3 = (int)(in_1 + 1.7790 * (in_2 - 128));

							// clamp values
							if (tmp_1 < 0) { tmp_1 = 0; }
							else if (tmp_1 > 255) { tmp_1 = 255; }
							if (tmp_2 < 0) { tmp_2 = 0; }
							else if (tmp_2 > 255) { tmp_2 = 255; }
							if (tmp_3 < 0) { tmp_3 = 0; }
							else if (tmp_3 > 255) { tmp_3 = 255; }

							// Inverse of BT.2020 -> BT.709
							out_1 = (int)tmp_1*1.6605 + tmp_2*-0.5877 + tmp_3*-0.0728;
							out_2 = (int)tmp_1*-0.1246 + tmp_2*1.133 + tmp_3*-0.0084;
							out_3 = (int)tmp_1*-0.0182 + tmp_2*-0.1006 + tmp_3*1.1187;

							break;
						}

						// clamp values
						if (out_1 < 0) { out_1 = 0; }
						else if (out_1 > 255) { out_1 = 255; }
						if (out_2 < 0) { out_2 = 0; }
						else if (out_2 > 255) { out_2 = 255; }
						if (out_3 < 0) { out_3 = 0; }
						else if (out_3 > 255) { out_3 = 255; }

						// set data
						luts->luts[i]->rgb[in_1][in_2][in_3].data[0] = (unsigned char)out_1;
						luts->luts[i]->rgb[in_1][in_2][in_3].data[1] = (unsigned char)out_2;
						luts->luts[i]->rgb[in_1][in_2][in_3].data[2] = (unsigned char)out_3;
					}
				}
			}
		}
	}

	qDebug() << "creating luts took" << elapsed_time.elapsed();
	//emit updateProgress(QString("creating luts took %1 sek.").arg(QString::number(qRound(elapsed_time.elapsed() / (float)10 ) / (float)100, 'f', 2)));
	emit finished();
}

int createLUTs::GammaLinToPQ(int in) {

	if (in < 0.018) {
		return 0;
	}
	
	return 1;
}
*/
