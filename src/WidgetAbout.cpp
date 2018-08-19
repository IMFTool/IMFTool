/* Copyright(C) 2016 Björn Stresing, Wolfgang Ruppel
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
#include "WidgetAbout.h"
#include "info.h"
#include "AS_DCP.h"
#include <QButtonGroup>
#include <QStackedWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QGridLayout>
#include <QLabel>
#include <QBoxLayout>
#include <QFile>

namespace
{
QString third_party = QString("Third Party:\n\n"
															"Certain Licenses, such as the GNU General Public License, GNU Lesser(or Library) General Public License require\n"
															"that make available the source code corresponding to FOSS binaries distributed under those licenses.\n"
															"Recipients who would like to receive a copy of such source code should submit a request by email, at wolfgang.ruppel@hs-rm.de\n"
															"QT (Version: %1): The Qt Toolkit is Copyright (C) 2015 The Qt Company Ltd and other contributors.\n"
															"Contact: http ://www.qt.io/licensing/\n\n"
															"OpenJPEG: Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium.\n\n"
															"AS-DCP lib. (Version %2): is Copyright (c) 2003-2012, John Hurst All rights reserved.\n\n"
															"regxmllib: Copyright (c) 2014, Pierre-Anthony Lemieux (pal@sandflow.com) All rights reserved.\n\n"
															"CodeSynthesis XSD: Provided under GPLv2 by Code Synthesis, http://www.codesynthesis.com/licenses/gpl-2.txt.\n\n"
															"Window Icon: Toma4025 http://toma4025.deviantart.com/art/RumixIP-107029752\n\n"
															"Other Icons: Copyright 2009-2018 FatCow Web Hosting, http://www.fatcow.com/free-icons\n\n"
															"QtWaitingSpinner: Original Work Copyright (c) 2012-2014 Alexander Turkin Modified 2014 by William Hallatt\n\n"
															"Marker Icon: This icon is provided by icons8.com as Creative Commons Attribution-NoDerivs 3.0 Unported\n\n"
															"Photon: Available at https://github.com/Netflix/photon/ under Apache 2.0 License\n\n"

		"\n"
		"BEGIN OpenJPEG License, copied from https://github.com/uclouvain/openjpeg/LICENSE\n"
		"The copyright in this software is being made available under the 2-clauses \n"
		"BSD License, included below. This software may be subject to other third \n"
		"party and contributor rights, including patent rights, and no such rights\n"
		"are granted under this license.\n"
		"\n"
		"Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium\n"
		"Copyright (c) 2002-2014, Professor Benoit Macq\n"
		"Copyright (c) 2003-2014, Antonin Descampe\n"
		"Copyright (c) 2003-2009, Francois-Olivier Devaux\n"
		"Copyright (c) 2005, Herve Drolon, FreeImage Team\n"
		"Copyright (c) 2002-2003, Yannick Verschueren\n"
		"Copyright (c) 2001-2003, David Janssens\n"
		"Copyright (c) 2011-2012, Centre National d'Etudes Spatiales (CNES), France \n"
		"Copyright (c) 2012, CS Systemes d'Information, France\n"
		"\n"
		"All rights reserved.\n"
		"\n"
		"Redistribution and use in source and binary forms, with or without\n"
		"modification, are permitted provided that the following conditions\n"
		"are met:\n"
		"1. Redistributions of source code must retain the above copyright\n"
		"   notice, this list of conditions and the following disclaimer.\n"
		"2. Redistributions in binary form must reproduce the above copyright\n"
		"   notice, this list of conditions and the following disclaimer in the\n"
		"   documentation and/or other materials provided with the distribution.\n"
		"\n"
		"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'\n"
		"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
		"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
		"ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
		"LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
		"CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
		"SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
		"INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
		"CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
		"ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
		"POSSIBILITY OF SUCH DAMAGE.\n"
		"END OpenJPEG License, copied from https://github.com/uclouvain/openjpeg/LICENSE\n"
															).arg(QT_VERSION_STR).arg(ASDCP::Version());

QString notice(PROJECT_NAME" Copyright(C) 2015-2017 Björn Stresing, Denis Manthey, Krispin Weiss, Wolfgang Ruppel\n"
							 "This program comes with ABSOLUTELY NO WARRANTY.\n"
							 "This is free software, and you are welcome to redistribute it\n"
							 "under certain conditions.\n\n"
							 "The initial development of this software has kindly been sponsored by Netflix Inc.\n"
							 );
}

WidgetAbout::WidgetAbout(QWidget *pParent /*= NULL*/) :
QWidget(pParent) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWindowFlags(Qt::Popup);
	setWindowTitle(tr("About").append(" " PROJECT_NAME));
	InitLayout();
}

QSize WidgetAbout::sizeHint() const {

	return QSize(600, 500);
}

void WidgetAbout::InitLayout() {

	QPushButton *p_button_License = new QPushButton(tr("&License"), this);
	p_button_License->setFlat(true);
	QPushButton *p_button_third = new QPushButton(tr("&Third Party"), this);
	p_button_third->setFlat(true);
	QTextBrowser *p_text_license = new QTextBrowser(this);
	QFile license_file(":/other/license");
	license_file.open(QFile::ReadOnly | QIODevice::Text);
	p_text_license->setText(QString::fromUtf8(license_file.readAll()));
	license_file.close();
	QTextBrowser *p_text_third_party = new QTextBrowser(this);
	p_text_third_party->setText(third_party);
	QLabel *p_label_name = new QLabel(PROJECT_NAME, this);
	p_label_name->setFont(QFont("Arial", 20, QFont::Bold));
	QLabel *p_label_author = new QLabel(tr("Authors: ").append("Björn Stresing, Denis Manthey, Krispin Weiß, Justin Hug, Wolfgang Ruppel"), this);
	QLabel *p_label_version = new QLabel(tr("Version: ").append(VERSION_MAJOR"." VERSION_MINOR"." SVN_REV), this);
	QLabel *p_label_notice = new QLabel(notice, this);
	QLabel *p_label_icon = new QLabel(this);
	p_label_icon->setPixmap(QPixmap(":/icon1.ico"));
	mpStackedWidget = new QStackedWidget(this);
	mpButtonGroup = new QButtonGroup(this);
	int index = mpStackedWidget->addWidget(p_text_license);
	mpButtonGroup->addButton(p_button_License, index);
	index = mpStackedWidget->addWidget(p_text_third_party);
	mpButtonGroup->addButton(p_button_third, index);
	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(p_label_name, 0, 1, 1, 1);
	p_layout->addWidget(p_label_icon, 1, 0, 4, 1);
	p_layout->addWidget(p_label_author, 1, 1, 1, 1);
	p_layout->addWidget(p_label_version, 2, 1, 1, 1);
	p_layout->addWidget(p_label_notice, 3, 1, 1, 1);
	p_layout->addWidget(mpStackedWidget, 4, 1, 1, 1);
	QBoxLayout *p_layout_sub = new QBoxLayout(QBoxLayout::LeftToRight);
	p_layout_sub->addWidget(p_button_License);
	p_layout_sub->addWidget(p_button_third);
	p_layout->addLayout(p_layout_sub, 5, 0, 1, 2);
	setLayout(p_layout);

	connect(mpButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(ToggleWidget(int)));
}

void WidgetAbout::ToggleWidget(int id) const {

	mpStackedWidget->setCurrentIndex(id);
}
