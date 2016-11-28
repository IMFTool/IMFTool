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
#include "TTMLDetails.h"
#include <QGridLayout>

TTMLDetails::TTMLDetails(QWidget *pParent /*= NULL*/) : QWidget(pParent) {
	InitLayout();
}

void TTMLDetails::InitLayout() {

	// create layout
	QGridLayout *p_layout = new QGridLayout();
	p_layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
	//p_layout->setContentsMargins(0, 0, 0, 0);
	//p_layout->setVerticalSpacing(0);

	// time label
	ttml_time = new QLabel();
	p_layout->addWidget(ttml_time, 0, 0, 1, 1); // add to layout
	ttml_time->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// search time label
	ttml_search_time = new QLabel();
	p_layout->addWidget(ttml_search_time, 0, 1, 1, 1); // add to layout
	ttml_search_time->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// show regions?
	show_regions = new QCheckBox(QString("show regions"), this);
	p_layout->addWidget(show_regions, 0, 2, 1, 1); // add to layout
	show_regions->setChecked(true);
	show_regions->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// wrap text?
	wrap_text = new QCheckBox(QString("wrap"));
	p_layout->addWidget(wrap_text, 0, 3, 1, 1); // add to layout
	wrap_text->setChecked(true);
	wrap_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(wrap_text, SIGNAL(stateChanged(int)), this, SLOT(wrapTextChanged(int)));

	// render style
	render_style = new QComboBox();
	p_layout->addWidget(render_style, 0, 4, 1, 1); // add to layout
	render_style->addItem("HTML");
	render_style->addItem("TEXT");
	render_style->addItem("pure TEXT");
	connect(render_style, SIGNAL(currentIndexChanged(int)), this, SLOT(renderStyleChanged(int)));

	// create textbox
	ttml_text = new QTextEdit();
	ttml_text->setReadOnly(true);
	ttml_text->setStyleSheet("QTextEdit{ background-color:#858585; color:#000; }"); // default styling
	ttml_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	p_layout->addWidget(ttml_text, 1, 0, 1, 5);

	setLayout(p_layout);
}

void TTMLDetails::rShowTTML(const QVector<QString> &time, QString text, int search_time) {

	if(time.length() > 0) ttml_time->setText(QString("time: %1 [ %2 ]").arg(time.at(0)).arg(time.length()));

	// set search time
	ttml_search_time->setText(QString("found in: %1 ms").arg(search_time));

	last_tt = text; // save (for change in rendering style..)
	renderStyleChanged(render_style->currentIndex());
}


void TTMLDetails::renderStyleChanged(int index) {

	QString temp(last_tt.data()); // copy

	// check if style is still the same
	if (lastStyle != index) { // style was changed!

		switch (index) {
		case 0: // HTML
			qDebug() << "default styling";
			ttml_text->setStyleSheet("QTextEdit{ background-color:#858585; color:#000; }"); // default styling
			break;
		case 1: // Text
			ttml_text->setStyleSheet("QTextEdit{ background-color:#fff; color:#000; font-size:12pt; }");
			break;
		case 2: // plain text
			ttml_text->setStyleSheet("QTextEdit{ background-color:#000; color:#fff; font-size:20pt; }");
			break;
		}
		lastStyle = index;
	}

	switch (index) {
	case 0: // HTML
		ttml_text->setHtml(temp);
		break;
	case 1: // Text
		ttml_text->setHtml(temp.toHtmlEscaped());
		break;
	case 2: // plain text
		ttml_text->setHtml(temp.remove(QRegExp("<[^>]*>")));
		break;
	}
	//ttml_text->adjustSize();
}

void TTMLDetails::wrapTextChanged(int index) {

	switch (index) {
	case 0: // 
		ttml_text->setLineWrapMode(QTextEdit::NoWrap);
		break;
	case 2: // Text
		ttml_text->setLineWrapMode(QTextEdit::WidgetWidth);
		break;
	}
}

