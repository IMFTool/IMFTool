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
#include "WidgetTimedTextPreview.h"
#include <QGridLayout>

WidgetTimedTextPreview::WidgetTimedTextPreview(QWidget *pParent /*= NULL*/) : QWidget(pParent) {
	InitLayout();
}

void WidgetTimedTextPreview::InitLayout() {

	// create layout
	QGridLayout *p_layout = new QGridLayout();
	p_layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
	p_layout->setContentsMargins(5, 2, 5, 2);

	// search time label
	ttml_search_time = new QLabel();
	p_layout->addWidget(ttml_search_time, 0, 0, 1, 1); // add to layout
	ttml_search_time->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// show regions?
	show_regions = new QCheckBox(QString("show regions"), this);
	p_layout->addWidget(show_regions, 0, 1, 1, 1); // add to layout
	show_regions->setChecked(true);
	show_regions->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// wrap text?
	wrap_text = new QCheckBox(QString("wrap"));
	p_layout->addWidget(wrap_text, 0, 2, 1, 1); // add to layout
	wrap_text->setChecked(true);
	wrap_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(wrap_text, SIGNAL(stateChanged(int)), this, SLOT(wrapTextChanged(int)));

	// buttons
	prev = new QPushButton("<");
	p_layout->addWidget(prev, 0, 3, 1, 1); // add to layout
	connect(prev, SIGNAL(clicked(bool)), this, SLOT(prevClicked(bool)));

	next = new QPushButton(">");
	next->setMinimumWidth(20);
	next->setMaximumWidth(40);
	p_layout->addWidget(next, 0, 4, 1, 1); // add to layout
	connect(next, SIGNAL(clicked(bool)), this, SLOT(nextClicked(bool)));

	// create splitter
	splitter = new QSplitter(Qt::Vertical);
	splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//connect(splitter, SIGNAL(), this, SLOT());

	p_layout->addWidget(splitter, 1, 0, 1, 5);

	// create time list
	ttml_times = new QListWidget();

	// create table model
	tableModel = new QStandardItemModel(0, 9);
	tableModel->setHorizontalHeaderItem(0, new QStandardItem(QString("track #")));

	QStandardItem *item1 = new QStandardItem(QString("hh:mm:ss"));
	item1->setToolTip("Media time/real time");
	tableModel->setHorizontalHeaderItem(1, item1);

	QStandardItem *item2 = new QStandardItem(QString("frames"));
	item2->setToolTip("Fractional TTML frames corresponding to TTML framerate");
	tableModel->setHorizontalHeaderItem(2, item2);

	tableModel->setHorizontalHeaderItem(3, new QStandardItem(QString("TXT prev."))); // text preview
	tableModel->setHorizontalHeaderItem(4, new QStandardItem(QString("IMG prev.")));
	// views
	tableModel->setHorizontalHeaderItem(5, new QStandardItem(QString("<xml>")));
	tableModel->setHorizontalHeaderItem(6, new QStandardItem(QString("<html>")));
	tableModel->setHorizontalHeaderItem(7, new QStandardItem(QString("plain text")));
	tableModel->setHorizontalHeaderItem(8, new QStandardItem(QString("HTML")));

	// create table view
	tableView = new QTableView();
	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	tableView->setColumnWidth(0, 30);
	tableView->setModel(tableModel);
	tableView->horizontalHeader()->setSectionsClickable(false);
	tableView->setSelectionMode(QAbstractItemView::NoSelection);
	tableView->verticalHeader()->setVisible(false);

	splitter->addWidget(tableView);

	// create textbox
	ttml_text = new QTextEdit();
	ttml_text->setReadOnly(true);
	splitter->addWidget(ttml_text);

	// create fonts
	font_small = QFont("Courier", 10);
	font_medium = QFont("Times", 15);
	font_html = QFont("Times", 18);

	setLayout(p_layout);
}

void WidgetTimedTextPreview::rShowTTML(const QVector<visibleTTtrack> &rttmls, int search_time) {

	ttmls = &rttmls; // save (for change in rendering style..)

	highlighted_btns = QVector<QPushButton*>();
	tableModel->setRowCount(0); // clear previous data
	QString text, tmp_text;
	int row_count = 0;

	ttml_times->clear();
	for (int i = 0; i < ttmls->length(); i++) { // loop tracks

		// loop visible elements
		for (int z = 0; z < ttmls->at(i).elements.length(); z++) {

			QList<QStandardItem*> row_items;

			// create track number
			QStandardItem *track_nr = new QStandardItem(QString("%1").arg((ttmls->at(i).resource.track_index + 1)));
			track_nr->setTextAlignment(Qt::AlignCenter);

			row_items.append(track_nr);
			row_items.append(new QStandardItem(ttmls->at(i).formatted_time));
			row_items.append(new QStandardItem(ttmls->at(i).fractional_frames));

			switch (ttmls->at(i).elements.at(z).type) {
			case 0: // TXT
				static QRegularExpression rm_regex("<[^>]*>");
				tmp_text = QString(ttmls->at(i).elements.at(z).text).remove(rm_regex);
				text.append(ttmls->at(i).elements.at(z).text);
				row_items.append(new QStandardItem(tmp_text)); // text preview
				row_items.append(new QStandardItem("")); // no image preview
				break;
			case 1: // IMG

				row_items.append(new QStandardItem("")); // no text preview
				QStandardItem *checkItem = new QStandardItem(); // QChar(0x2713));
				checkItem->setData(QPixmap::fromImage(ttmls->at(i).elements.at(z).bgImage).scaled(34,34,Qt::KeepAspectRatio), Qt::DecorationRole);
				row_items.append(checkItem);
				break;
			}
			
			tableModel->appendRow(row_items);

			// add buttons!
			switch (ttmls->at(i).elements.at(z).type) {
			case 0: // TXT
				createButton(row_count, i, 5, z, true); // XML
				createButton(row_count, i, 6, z, true); // <TEXT>
				createButton(row_count, i, 7, z, true); // pTEXT
				createButton(row_count, i, 8, z, true); // HTML
				break;
			case 1: // IMG
				createButton(row_count, i, 5, z, false); // XML
				createButton(row_count, i, 6, z, false); // <TEXT>
				break;
			}
			row_count++;
		}

		// show hidden track
		if (ttmls->at(i).elements.length() == 0) {

			QList<QStandardItem*> row_items;

			// create track number
			QStandardItem *track_nr = new QStandardItem(QString("%1").arg((ttmls->at(i).resource.track_index + 1)));
			track_nr->setTextAlignment(Qt::AlignCenter);

			row_items.append(track_nr);
			row_items.append(new QStandardItem(ttmls->at(i).formatted_time));
			row_items.append(new QStandardItem(ttmls->at(i).fractional_frames));
			tableModel->appendRow(row_items);

			createButton(row_count, i, 5, 0, false); // XML
			row_count++;
		}
	}

	// wrap text?
	if (wrap_text_enabled) {
		ttml_text->setLineWrapMode(QTextEdit::WidgetWidth);
	}
	else {
		ttml_text->setLineWrapMode(QTextEdit::NoWrap);
	}

	// render subtitles
	switch (render_style) { // 0 : TEXT, 1 : pTEXT, 2 : HTML
	case 0: // Text
		ttml_text->setFont(font_small);
		ttml_text->setHtml(text.toHtmlEscaped());
		break;
	case 1: // plain text
		static QRegularExpression rm_regex("<[^>]*>");
		ttml_text->setFont(font_medium);
		ttml_text->setHtml(QString(text).remove(rm_regex));
		break;
	case 2: // HTML
		ttml_text->setFont(font_html);
		ttml_text->setHtml(text);
		break;
	}

	// display search time
	ttml_search_time->setText(QString("found in: %1 ms").arg(search_time));
}


void WidgetTimedTextPreview::createButton(int table_row, int track_index, int col, int item, bool highlight) {

	QPushButton *btn_xml = new QPushButton("show");
	btn_xml->setProperty("track", track_index);
	btn_xml->setProperty("col", col);
	btn_xml->setProperty("item", item);

	if (highlight && render_style == (col - 6)) {
		btn_xml->setStyleSheet("QPushButton{ background-color:#2f6c99;}");
		highlighted_btns.append(btn_xml);
	}

	tableView->setIndexWidget(tableModel->index(table_row, col), btn_xml);
	connect(btn_xml, SIGNAL(clicked()), this, SLOT(pushButtonClicked()), Qt::UniqueConnection);
}

void WidgetTimedTextPreview::pushButtonClicked() {

	for (int i = 0; i < highlighted_btns.length(); i++) {
		highlighted_btns.at(i)->setStyleSheet("QPushButton{ background-color:#4b4b4b;}"); // reset style
	}
	highlighted_btns = QVector<QPushButton*>(); // clear list

	QPushButton *button = dynamic_cast<QPushButton*>(sender()); // get button
	button->setStyleSheet("QPushButton{ background-color:#2f6c99;}");

	showSelection(button->property("track").toInt(), button->property("col").toInt(), button->property("item").toInt());

	highlighted_btns.append(button);
}

void WidgetTimedTextPreview::showSelection(int track, int col, int item) {

	// wrap text?
	if (wrap_text_enabled) {
		ttml_text->setLineWrapMode(QTextEdit::WidgetWidth);
	}
	else {
		ttml_text->setLineWrapMode(QTextEdit::NoWrap);
	}
	
	// set style
	switch (col) { // 0 : TEXT, 1 : pTEXT, 2 : HTML
	case 5: // XML
		ttml_text->setLineWrapMode(QTextEdit::NoWrap);
		ttml_text->setFont(font_small);
		ttml_text->setPlainText(ttmls->at(track).resource.doc);
		break;
	case 6: // <HTML>
		render_style = 0;
		ttml_text->setFont(font_small);
		ttml_text->setHtml(ttmls->at(track).elements.at(item).text.toHtmlEscaped());
		break;
	case 7: // plain TEXT
		render_style = 1;
		ttml_text->setFont(font_medium);
		static QRegularExpression rm_regex ("<[^>]*>");
		ttml_text->setHtml(QString(ttmls->at(track).elements.at(item).text).remove(rm_regex));
		break;
	case 8: // HTML
		render_style = 2;
		ttml_text->setFont(font_html);
		ttml_text->setHtml(ttmls->at(track).elements.at(item).text);
		break;
	}
}

void WidgetTimedTextPreview::wrapTextChanged(int index) {

	switch (index) {
	case 0: // 
		wrap_text_enabled = false;
		ttml_text->setLineWrapMode(QTextEdit::NoWrap);
		break;
	case 2:
		wrap_text_enabled = true;
		ttml_text->setLineWrapMode(QTextEdit::WidgetWidth);
		break;
	}
}

void WidgetTimedTextPreview::ClearTTML() {
	tableModel->setRowCount(0); // clear table rows
	ttml_text->clear();
	ttml_search_time->clear();
}

void WidgetTimedTextPreview::prevClicked(bool) {
	emit PrevNextSubClicked(false);
}

void WidgetTimedTextPreview::nextClicked(bool) {
	emit PrevNextSubClicked(true);
}

