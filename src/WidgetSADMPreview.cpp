/* Copyright(C) 2024 Wolfgang Ruppel
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
#include "WidgetSADMPreview.h"
#include <QGridLayout>
#include <QByteArray>
#include <QClipboard>
#include <QScrollBar>
#include "AS_02.h"
#include "AS_02_MGASADM.h"
//#include "XmlQSyntaxHighlighter.h"
#include "qcompressor.h"


WidgetSADMPreview::WidgetSADMPreview(QWidget *pParent /*= NULL*/) : QWidget(pParent) {
	InitLayout();
}

void WidgetSADMPreview::InitLayout() {

	// create layout
	QGridLayout *p_layout = new QGridLayout();
	p_layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
	p_layout->setContentsMargins(5, 2, 5, 2);

	mWrapText = new QCheckBox(QString("wrap lines"));
	p_layout->addWidget(mWrapText, 0, 1, 1, 1); // add to layout
	mWrapText->setChecked(true);
	mWrapText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(mWrapText, SIGNAL(stateChanged(int)), this, SLOT(wrapTextChanged(int)));

	mKeepScrollPosition = new QCheckBox(QString("keep scroll position"));
	p_layout->addWidget(mKeepScrollPosition, 0, 2, 1, 1); // add to layout
	mKeepScrollPosition->setChecked(true);
	mKeepScrollPosition->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	connect(mKeepScrollPosition, SIGNAL(stateChanged(int)), this, SLOT(keepScrollPosChanged(int)));;

	mPopOutButton = new QPushButton("Open in Pop-out");
	p_layout->addWidget(mPopOutButton, 0, 4, 1, 1); // add to layout
	connect(mPopOutButton, SIGNAL(clicked(bool)), this, SLOT(sadmPopOut()));

	mCopyClipboardButton = new QPushButton("Copy to Clipboard");
	p_layout->addWidget(mCopyClipboardButton, 0, 5, 1, 1); // add to layout
	connect(mCopyClipboardButton, SIGNAL(clicked(bool)), this, SLOT(copyClipboard()));

	// buttons
	mPrevButton = new QPushButton("<");
	p_layout->addWidget(mPrevButton, 0, 6, 1, 1); // add to layout
	connect(mPrevButton, SIGNAL(clicked(bool)), this, SLOT(prevClicked()));

	mNextButton = new QPushButton(">");
	mNextButton->setMinimumWidth(20);
	mNextButton->setMaximumWidth(40);
	p_layout->addWidget(mNextButton, 0, 7, 1, 1); // add to layout
	connect(mNextButton, SIGNAL(clicked(bool)), this, SLOT(nextClicked()));

	// create mSplitter
	mSplitter = new QSplitter(Qt::Vertical);
	mSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	p_layout->addWidget(mSplitter, 1, 0, 1, 8);

	// create table model
	mTableModel = new QStandardItemModel(0, 5);
	mTableModel->setHorizontalHeaderItem(0, new QStandardItem()); // column for "Show" buttons

	QStandardItem *item0 = new QStandardItem(QString("CPL Track No."));
	item0->setToolTip("Track in the timeline from top to bottom, image track is #1 ");
	mTableModel->setHorizontalHeaderItem(1, item0);

	mTableModel->setHorizontalHeaderItem(2, new QStandardItem(QString("CPL Track ID"))); // text preview

	QStandardItem *item2 = new QStandardItem(QString("CPL Frame"));
	item2->setToolTip("Frame number selected in the timeline");
	mTableModel->setHorizontalHeaderItem(3, item2);

	QStandardItem *item3 = new QStandardItem(QString("S-ADM Frame"));
	item3->setToolTip("S-ADM frame number - differs from CPL frame number if the S-ADM edit rate differs from he CPL edit rate");
	mTableModel->setHorizontalHeaderItem(4, item3);

//	QStandardItem *item4 = new QStandardItem();
//	mTableModel->setHorizontalHeaderItem(5, item4);


	// create table view
	mTableView = new MyTableView(this);
	mTableView->setModel(mTableModel);
	mTableView->horizontalHeader()->setSectionsClickable(false);
	mTableView->setSelectionMode(QAbstractItemView::NoSelection);
	mTableView->verticalHeader()->setVisible(false);
	mTableView->setColumnWidth(2, 300);
	mTableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//mTableView->horizontalHeader()->setStretchLastSection(true);
	mTableView->verticalHeader()->setHidden(true);
	mTableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	mSplitter->addWidget(mTableView);
	mSplitter->setOpaqueResize(false);


	// create textbox
	mSadmText = new QTextEdit("No S-ADM Track present");
	mSadmText->setReadOnly(true);
	mSadmText->setFontFamily("Courier New");
	mSadmText->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
	if (mWrapTextEnabled == false) mSadmText->setLineWrapMode(QTextEdit::NoWrap);

	mSplitter->addWidget(mSadmText);

	setLayout(p_layout);

	// Create Pop-up widget wizard
	mSadmPopupWizard = new QWizard(this, Qt::Popup | Qt::Dialog );
	// Create wizard page
	mSadmPageWizard = new QWizardPage(mSadmPopupWizard);
	mSadmPopupWizard->addPage(mSadmPageWizard);
	mSadmPopupWizard->resize(1200,1000);
	mSadmPopupWizard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	mSadmPopupWizard->setWindowModality(Qt::WindowModal);
	mSadmPopupWizard->setWindowTitle(tr("S-ADM Audio Metadata"));
	mSadmPopupWizard->setWizardStyle(QWizard::ModernStyle);
	mSadmPopupWizard->setStyleSheet("QWizard QPushButton {min-width: 60 px;}");
	// Table on the top
	mTableModelPopUp = new QStandardItemModel(1, 4);
	QStandardItem *item_popup0 = new QStandardItem(QString("CPL Track No."));
	item_popup0->setToolTip("Track in the timeline from top to bottom, image track is #1 ");
	mTableModelPopUp->setHorizontalHeaderItem(0, item_popup0);

	mTableModelPopUp->setHorizontalHeaderItem(1, new QStandardItem(QString("CPL Track ID"))); // text preview

	QStandardItem *item_popup2 = new QStandardItem(QString("CPL Frame"));
	item_popup2->setToolTip("Frame number selected in the timeline");
	mTableModelPopUp->setHorizontalHeaderItem(2, item_popup2);

	QStandardItem *item_popup3 = new QStandardItem(QString("S-ADM Frame"));
	item_popup3->setToolTip("S-ADM frame number - differs from CPL frame number if the S-ADM edit rate differs from he CPL edit rate");
	mTableModelPopUp->setHorizontalHeaderItem(3, item_popup3);

	// create table view
	mTableViewPopUp = new MyTableView(this);
	//mTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	mTableViewPopUp->setModel(mTableModelPopUp);
	mTableViewPopUp->horizontalHeader()->setSectionsClickable(false);
	mTableViewPopUp->setSelectionMode(QAbstractItemView::NoSelection);
	mTableViewPopUp->verticalHeader()->setVisible(false);
	mTableViewPopUp->setColumnWidth(1, 300);
	mTableViewPopUp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QVBoxLayout *vbox_layout = new QVBoxLayout();
	vbox_layout->addWidget(mTableViewPopUp);

	// Create layout for text widget and buttons
	mSadmPopUpText = new QTextEdit("No S-ADM Track present");
	mSadmPopUpText->setReadOnly(true);
	mSadmPopUpText->setLineWrapMode(QTextEdit::NoWrap);
	mSadmPopUpText->setFontFamily("Courier New");
	//mSadmPopUpText->setFont(font_small);
	mSadmPopUpText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	vbox_layout->addWidget(mSadmPopUpText);
	mSadmPageWizard->setLayout(vbox_layout);
	QList<QWizard::WizardButton> layout;
	mSadmPopupWizard->setOption(QWizard::HaveCustomButton1, true);
	mSadmPopupWizard->setOption(QWizard::HaveCustomButton2, true);
	mSadmPopupWizard->setOption(QWizard::HaveCustomButton3, true);
	mSadmPopupWizard->setButtonText(QWizard::CustomButton1, tr("Copy to Clipboard"));
	mSadmPopupWizard->setButtonText(QWizard::CustomButton2, tr("<"));
	mSadmPopupWizard->setButtonText(QWizard::CustomButton3, tr(">"));
	layout << QWizard::CustomButton1 << QWizard::Stretch << QWizard::CustomButton2 << QWizard::CustomButton3 << QWizard::Stretch << QWizard::CancelButton;
	mSadmPopupWizard->setButtonLayout(layout);
	connect(mSadmPopupWizard->button(QWizard::CustomButton1), SIGNAL(clicked()), this, SLOT(copyClipboard()));
	connect(mSadmPopupWizard->button(QWizard::CustomButton2), SIGNAL(clicked()), this, SLOT(prevClicked()));
	connect(mSadmPopupWizard->button(QWizard::CustomButton3), SIGNAL(clicked()), this, SLOT(nextClicked()));

	mSadmPopupWizard->setAttribute(Qt::WA_DeleteOnClose, false);
	mSadmPopupWizard->setAttribute(Qt::WA_QuitOnClose, false);
}

void WidgetSADMPreview::rSetSadmTimeline(const QMap<quint8, QUuid> &rMGASADMTracks) {

	mMGASADMTracks = rMGASADMTracks;
	mSadmTextMap.clear();
	mSadmScrollPositionMap.clear();
	mHighlightedButton = nullptr;
	mTableModel->setRowCount(0); // clear previous data
	QString text, tmp_text;
	quint8 row_count = 0;
	for (QMap<quint8, QUuid>::iterator iter_track_no = mMGASADMTracks.begin(); iter_track_no != mMGASADMTracks.end(); iter_track_no++) {
		QList<QStandardItem*> row_items;
		row_items.append(new QStandardItem());
		QStandardItem *track_nr = new QStandardItem(QString("%1").arg(iter_track_no.key()));
		track_nr->setTextAlignment(Qt::AlignCenter);
		track_nr->setEditable(false);
		row_items.append(track_nr);
		row_items.append(new QStandardItem(iter_track_no.value().toString().remove("{").remove("}")));
		mTableModel->appendRow(row_items);
		mSadmScrollPositionMap.insert(iter_track_no.key(), 0);
		createButton(row_count, iter_track_no.key(), row_count == 0);
		row_count++;
	}
	mTableView->resize(mTableView->sizeHint());
	mSadmText->updateGeometry();
}

void WidgetSADMPreview::rXPosChanged(const QSharedPointer<AssetMxfTrack> &rAsset, const qint64 &rOffset, const Timecode &rTimecode, const QUuid &track_id, const quint8 &rTrackNumber) {

    qint32 edit_rate_factor = 1;
    if (rAsset.isNull())
    	return;

	mOutOfTimeline = false;
    // Edit Rate check
    if (rAsset->GetEditRate().GetDenominator() == rTimecode.GetEditRate().GetDenominator()) {
    	edit_rate_factor = rAsset->GetEditRate().GetNumerator() / rTimecode.GetEditRate().GetNumerator();
    }

	AS_02::MGASADM::MXFReader reader(defaultFactory);
	ASDCP::Result_t result = reader.OpenRead(rAsset->GetPath().absoluteFilePath().toStdString());
	QStringList sadm_subframes;

	ASDCP::FrameBuffer *buff = new ASDCP::FrameBuffer();
	buff->Capacity(1000000);
	for (int i=0; i < edit_rate_factor; i++) {
		bool isCompressed = false;
	    quint8 lenLen = 0;
	    quint32 metaLen = 0;
	    QByteArray meta_payload_text;
		result = reader.ReadMetaFrame(edit_rate_factor*rOffset + i, *buff);
		if (result.Failure()) {
		  qDebug() <<"ReadFrame error: " <<  result.Value();
		}
		const QByteArray meta_payload_raw = QByteArray(reinterpret_cast<const char*>(buff->Data()), buff->Size());
		// Test if S-ADM Payload tag 0x12 is present
		if ((quint8)meta_payload_raw[0] == 0x12) {
			// Decode BER encoded metaLen
			if((quint8)meta_payload_raw[1] & 0x80) {
				lenLen = (quint8)meta_payload_raw[1] & 0x0F;
				for (int j=0; j<lenLen; j++) {
					metaLen += ((quint8)meta_payload_raw[1 + lenLen - j] ) << j*8;
				}
			} else{
				metaLen = (quint8)meta_payload_raw[1];
			}
			// Compression Byte is at Tag + BER length + 2 (ST 2127-10 Table 2)
			isCompressed = ((quint8)meta_payload_raw[1 + lenLen + 2] == 0x01);

			if (isCompressed) {
				QCompressor::gzipDecompress(meta_payload_raw.right(metaLen - 2), meta_payload_text);
			} else {
				meta_payload_text = meta_payload_raw.right(metaLen - 2);
			}
		} else {
			meta_payload_text = "No S-ADM Payload present in MGA metadata section 1";
		}
		//XmlQSyntaxHighlighter* rHighLighter = new XmlQSyntaxHighlighter(mSadmText);
		sadm_subframes << QString(meta_payload_text);
	}
	mSadmTextMap.insert(rTrackNumber, sadm_subframes);

	int active_track = mHighlightedButton->property("track").toInt();

	qint8 current_row = -1;
	for (int i=0; i < mTableModel->rowCount(); i++) {
		QPushButton* button = qobject_cast<QPushButton*>(mTableView->indexWidget(mTableView->model()->index(i, 0))); //qvariant_cast<QPushButton*>(mTableView->model()->data(mTableView->model()->index(i, 0)));
		if (button && (button->property("track") == rTrackNumber)) {
			current_row = i;
			button->setProperty("edit_rate_factor", edit_rate_factor);
		}
	}

	if (mHighlightedButton && (active_track == rTrackNumber)) {
		if (current_row >= 0) {
			int table_row = mHighlightedButton->property("table_row").toInt();
			int edit_rate_factor = mHighlightedButton->property("edit_rate_factor").toInt();
			QModelIndex index = mTableView->model()->index(table_row, 3);
			qint64 image_frame = mTableView->model()->data(index).toInt();
			index = mTableView->model()->index(table_row, 4);
			qint64 sadm_frame = mTableView->model()->data(index).toInt();
			int sub_frame_index = sadm_frame - edit_rate_factor * image_frame;
		}
		updateSadm(mSadmTextMap.value(active_track).at(0), active_track);
	}
	// clean up
	reader.Close();
	delete buff;

	if (current_row >= 0) {
		QModelIndex index = mTableView->model()->index(current_row, 3);
		mTableView->model()->setData(index,rTimecode.GetOverallFrames());
		index = mTableView->model()->index(current_row, 4);
		mTableView->model()->setData(index, edit_rate_factor * rTimecode.GetOverallFrames());
	}
	if (active_track == rTrackNumber) {
		QModelIndex index = mTableViewPopUp->model()->index(0, 2);
		mTableViewPopUp->model()->setData(index, rTimecode.GetOverallFrames());
		index = mTableViewPopUp->model()->index(0, 3);
		mTableViewPopUp->model()->setData(index, edit_rate_factor * rTimecode.GetOverallFrames());
	}
}

void WidgetSADMPreview::rOutOfTimeline(const qint64 &rPosition) {
	mOutOfTimeline = true;
}


void WidgetSADMPreview::createButton(int table_row, int track_index, bool highlight) {

	QPushButton *button = new QPushButton("show");
	button->setProperty("table_row", table_row);
	button->setProperty("track", track_index);

	if (highlight) {
		button->setStyleSheet("QPushButton{ background-color:#2f6c99;}");
		mHighlightedButton = button;
		QModelIndex index_set = mTableViewPopUp->model()->index(0, 0);
		QModelIndex index_get = mTableView->model()->index(table_row, 1);
		mTableViewPopUp->model()->setData(index_set, mTableView->model()->data(index_get));
		index_set = mTableViewPopUp->model()->index(0, 1);
		index_get = mTableView->model()->index(table_row, 2);
		mTableViewPopUp->model()->setData(index_set, mTableView->model()->data(index_get));
		mLastActiveTrack = track_index;
	}

	mTableView->setIndexWidget(mTableModel->index(table_row, 0), button);
	connect(button, SIGNAL(clicked()), this, SLOT(pushButtonClicked()), Qt::UniqueConnection);
}

void WidgetSADMPreview::pushButtonClicked() {

	int sub_frame_index = 0;
	QPushButton *button = dynamic_cast<QPushButton*>(sender()); // get button
	if (mHighlightedButton) {
		mHighlightedButton->setStyleSheet("QPushButton{ background-color:#4b4b4b;}");
		mLastActiveTrack = mHighlightedButton->property("track").toInt();
	}
	button->setStyleSheet("QPushButton{ background-color:#2f6c99;}");
	mHighlightedButton = button;

	if (mSadmTextMap.contains(button->property("track").toInt())) {
		int table_row = mHighlightedButton->property("table_row").toInt();
		int edit_rate_factor = mHighlightedButton->property("edit_rate_factor").toInt();
		QModelIndex index = mTableView->model()->index(table_row, 3);
		qint64 image_frame = mTableView->model()->data(index).toInt();
		index = mTableView->model()->index(table_row, 4);
		qint64 sadm_frame = mTableView->model()->data(index).toInt();
		int new_active_track = button->property("track").toInt();
		int sub_frame_index = sadm_frame - edit_rate_factor * image_frame;
		updateSadm(mSadmTextMap.value(new_active_track).at(sub_frame_index), new_active_track);
		mLastActiveTrack = new_active_track;
	}
	else mSadmText->setText(QString());
	// wrap text?
	if (mWrapTextEnabled) {
		mSadmText->setLineWrapMode(QTextEdit::WidgetWidth);
	}
	else {
		mSadmText->setLineWrapMode(QTextEdit::NoWrap);
	}
	QModelIndex index_set;
	QModelIndex index_get;
	for (int col = 0; col < 4; col ++) {
		index_set = mTableViewPopUp->model()->index(0, col);
		index_get = mTableView->model()->index(button->property("table_row").toInt(), col + 1);
		mTableViewPopUp->model()->setData(index_set, mTableView->model()->data(index_get));
	}
}

void WidgetSADMPreview::wrapTextChanged(int index) {

	switch (index) {
	case 0: // 
		mWrapTextEnabled = false;
		mSadmText->setLineWrapMode(QTextEdit::NoWrap);
		break;
	case 2:
		mWrapTextEnabled = true;
		mSadmText->setLineWrapMode(QTextEdit::WidgetWidth);
		break;
	}
}

void WidgetSADMPreview::keepScrollPosChanged(int index) {

	switch (index) {
	case 0: //
		mKeepScrollPos = false;
		break;
	case 2:
		mKeepScrollPos = true;
		break;
	}
}

void WidgetSADMPreview::ClearSADM() {

	mTableModel->setRowCount(0); // clear table rows
	mSadmText->setText("No MGA S-ADM Track present");
}

void WidgetSADMPreview::prevClicked() {

	if (mHighlightedButton) {
		int table_row = mHighlightedButton->property("table_row").toInt();
		int edit_rate_factor = mHighlightedButton->property("edit_rate_factor").toInt();
		int active_track = mHighlightedButton->property("track").toInt();
		QModelIndex index = mTableView->model()->index(table_row, 3);
		qint64 image_frame = mTableView->model()->data(index).toInt();
		index = mTableView->model()->index(table_row, 4);
		qint64 sadm_frame = mTableView->model()->data(index).toInt();
		int sub_frame_index = sadm_frame - edit_rate_factor * image_frame;

		if (mOutOfTimeline || !mSadmTextMap.contains(active_track)) {
			emit PrevClicked();
		} else {
			if ((sub_frame_index == 0) || (edit_rate_factor == 1)) {
				emit PrevClicked();
				if (sadm_frame > 0) {
					updateSadm(mSadmTextMap.value(active_track).at(edit_rate_factor - 1), active_track);
				}
			} else if (sub_frame_index > 0) {
				updateSadm(mSadmTextMap.value(active_track).at(sub_frame_index - 1), active_track);
			}
			if ((sadm_frame > 0) && (sub_frame_index >= 0)) {
				for (int i=0; i < mTableModel->rowCount(); i++) {
					QPushButton* button = qobject_cast<QPushButton*>(mTableView->indexWidget(mTableView->model()->index(i, 0)));
					QModelIndex index = mTableView->model()->index(i, 4);
					int current_edit_rate_factor = button->property("edit_rate_factor").toInt();
					if (current_edit_rate_factor == edit_rate_factor) mTableView->model()->setData(index, sadm_frame - 1);
					else if (current_edit_rate_factor == 2 * edit_rate_factor) mTableView->model()->setData(index, 2 * sadm_frame - 2);
					else if ((current_edit_rate_factor == edit_rate_factor / 2) && (sadm_frame % 2 == 0)) mTableView->model()->setData(index, sadm_frame/2 - 1);
					if (i == table_row) {
						QModelIndex index = mTableViewPopUp->model()->index(0, 3);
						mTableViewPopUp->model()->setData(index, sadm_frame - 1);
					}
				}
			}
		}
	}
}

void WidgetSADMPreview::nextClicked() {

	if (mHighlightedButton) {
		int table_row = mHighlightedButton->property("table_row").toInt();
		int edit_rate_factor = mHighlightedButton->property("edit_rate_factor").toInt();
		int active_track = mHighlightedButton->property("track").toInt();
		QModelIndex index = mTableView->model()->index(table_row, 3);
		qint64 image_frame = mTableView->model()->data(index).toInt();
		index = mTableView->model()->index(table_row, 4);
		qint64 sadm_frame = mTableView->model()->data(index).toInt();
		int sub_frame_index = sadm_frame - edit_rate_factor * image_frame;
		if ((sub_frame_index == edit_rate_factor - 1) || !mSadmTextMap.contains(active_track)) {
			emit NextClicked();
		} else {
			updateSadm(mSadmTextMap.value(active_track).at(sub_frame_index + 1), active_track);
			for (int i=0; i < mTableModel->rowCount(); i++) {
				QPushButton* button = qobject_cast<QPushButton*>(mTableView->indexWidget(mTableView->model()->index(i, 0)));
				QModelIndex index = mTableView->model()->index(i, 4);
				int current_edit_rate_factor = button->property("edit_rate_factor").toInt();
				if (current_edit_rate_factor == edit_rate_factor) mTableView->model()->setData(index, sadm_frame + 1);
				else if (current_edit_rate_factor == 2 * edit_rate_factor) mTableView->model()->setData(index, 2 * sadm_frame + 2);
				else if ((current_edit_rate_factor == edit_rate_factor / 2) && (sadm_frame % 2 != 0)) mTableView->model()->setData(index, sadm_frame/2 + 1);
				if (i == table_row) {
					QModelIndex index = mTableViewPopUp->model()->index(0, 3);
					mTableViewPopUp->model()->setData(index, sadm_frame + 1);
				}
			}

		}
	}
}

void WidgetSADMPreview::copyClipboard() {

	QClipboard *clipboard = QGuiApplication::clipboard();
	clipboard->setText(mSadmText->toPlainText());
}

void WidgetSADMPreview::sadmPopOut() {

	mSadmPopUpText->setText(mSadmText->toPlainText());

	mSadmPopupWizard->show();
	mSadmPopupWizard->activateWindow();
}

void WidgetSADMPreview::updateSadm(QString text, int rActiveTrack) {

	if (mSadmScrollPositionMap.contains(mLastActiveTrack)) mSadmScrollPositionMap.remove(mLastActiveTrack);
	if (mLastActiveTrack > 0) mSadmScrollPositionMap.insert(mLastActiveTrack, mSadmText->verticalScrollBar()->value());
	mSadmText->setText(text);
	if (mKeepScrollPos) mSadmText->verticalScrollBar()->setValue(mSadmScrollPositionMap.value(rActiveTrack, 0));
	int position = mSadmPopUpText->verticalScrollBar()->value();
	mSadmPopUpText->setText(mSadmText->toPlainText());
	if (mKeepScrollPos) mSadmPopUpText->verticalScrollBar()->setValue(position);
}

MyTableView::MyTableView(QWidget* parent)
  : QTableView(parent)
{
}

QSize MyTableView::sizeHint() const
{
  return minimumSizeHint();
}

QSize MyTableView::minimumSizeHint() const
{
  QAbstractItemModel* m = model();

  int colCount = m->columnCount();
  int w = 0;
  for(int i=0; i<colCount; ++i) {
    w += columnWidth(i);
  }

  int rowCount = m->rowCount();
  int h = 0;
  for(int i=0; i<rowCount; ++i) {
    h += rowHeight(i);
  }

  int doubleFrame = 2 * frameWidth();

  w += verticalHeader()->width() + doubleFrame;
  h += horizontalHeader()->height() + doubleFrame;

  return QSize(w, h);
}
