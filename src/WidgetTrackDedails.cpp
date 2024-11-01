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
#include "WidgetTrackDedails.h"
#include <QLabel>
#include <QButtonGroup>
#include <QToolButton>
#include <QPushButton>
#include <QGridLayout>
#include <QMenu>


AbstractWidgetTrackDetails::AbstractWidgetTrackDetails(QWidget *pParent /*= NULL*/) :
QFrame(pParent) {

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	setObjectName("WidgetTrackDetailBase");
	setStyleSheet(QString(
		"QFrame#%1 {"
		"background-color: %2;"
		"border-width: 1px;"
		"border-top-width: 0px;"
		"border-left-width: 0px;"
		"border-color: %3;"
		"border-style: solid;"
		"border-radius: 0px;"
		"}").arg(objectName()).arg(QColor(CPL_COLOR_BACKGROUND).name()).arg(QColor(CPL_BORDER_COLOR).name()));
}

void AbstractWidgetTrackDetails::resizeEvent(QResizeEvent *pEvent) {

	emit HeightAboutToChange(pEvent->size().height());
}

WidgetTrackDetailsTimeline::WidgetTrackDetailsTimeline(QWidget *pParent /*= NULL*/) :
AbstractWidgetTrackDetails(pParent), mpTimecodeLabel(NULL), mpButtonGroup(NULL) {

	InitLayout();
}

void WidgetTrackDetailsTimeline::InitLayout() {

	mpTimecodeLabel = new QLabel(this);
	mpTimecodeLabel->setText(Timecode().GetFramesAsString());
	QFont font("Arial");
	font.setFixedPitch(true);
	if(font.pointSize() <= -1) font.setPixelSize(font.pixelSize() * 2);
	else font.setPointSize(font.pointSize() * 2);
	mpTimecodeLabel->setFont(font);
	mpButtonGroup = new QButtonGroup(this);
	mpButtonGroup->setExclusive(false);

	QPushButton *p_button_timecode = new QPushButton(this);
	p_button_timecode->setText("Timecode");
	p_button_timecode->setFlat(true);
	p_button_timecode->setFixedSize(80, 20);
	p_button_timecode->setCheckable(true);
	//p_button_timecode->setDown(true);
	p_button_timecode->setChecked(false);

	QPushButton *p_button_frames = new QPushButton(this);
	p_button_frames->setText("Frames");
	p_button_frames->setFlat(true);
	p_button_frames->setFixedSize(80, 20);
	p_button_frames->setCheckable(true);
	p_button_frames->setChecked(true);


	mpButtonGroup->addButton(p_button_timecode, ButtonTimecode);
	mpButtonGroup->addButton(p_button_frames, ButtonFrames);

	QGridLayout *p_layout = new QGridLayout();
	p_layout->setHorizontalSpacing(0);
	p_layout->addWidget(mpTimecodeLabel, 0, 0, 1, 2);
	p_layout->addWidget(p_button_frames, 1, 0, 1, 1);
	p_layout->addWidget(p_button_timecode, 1, 1, 1, 1);

	setLayout(p_layout);

	connect(mpButtonGroup, SIGNAL(idToggled(int, bool)), this, SLOT(rButtonToggled(int, bool)));
	connect(this, SIGNAL(FramesTimecodeToggled(bool)), this, SLOT(slotShowFrames(bool)));
}

void WidgetTrackDetailsTimeline::SetTimecode(const Timecode &rTimeCode) {

	if (mShowFrames) mpTimecodeLabel->setText(rTimeCode.GetFramesAsString());
	else mpTimecodeLabel->setText(rTimeCode.GetAsString());
	mLastTimeCode = rTimeCode;
}


void WidgetTrackDetailsTimeline::rButtonToggled(int id, bool checked) {


	if (id == ButtonTimecode) {
		QAbstractButton *p_button =  mpButtonGroup->button(ButtonFrames);
		if (p_button) {
			p_button->setChecked(!checked);
		}
		emit FramesTimecodeToggled(!checked);
	}
	if (id == ButtonFrames) {
		QAbstractButton *p_button = mpButtonGroup->button(ButtonTimecode);
		if (p_button) {
			p_button->setChecked(!checked);
		}
		emit FramesTimecodeToggled(checked);
	}


}

void WidgetTrackDetailsTimeline::slotShowFrames(bool rShowFrames) {
	mShowFrames = rShowFrames;
	this->SetTimecode(mLastTimeCode);
}

WidgetTrackDetails::WidgetTrackDetails(const QUuid &rTrackId, eSequenceType type, QWidget *pParent /*= NULL*/) :
AbstractWidgetTrackDetails(pParent), mTrackId(rTrackId), mType(type) {

	InitLayout();
}

void WidgetTrackDetails::InitLayout() {

	QToolButton *p_tool_button = new QToolButton(this);
	p_tool_button->setSizePolicy(p_tool_button->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
	QLabel *p_label = new QLabel(this);
	switch(mType) {
		case AncillaryDataSequence:
			p_label->setText(tr("Ancillary"));
			p_tool_button->setIcon(QIcon(":/attach_bw.png"));
			break;
		case CommentarySequence:
			p_label->setText(tr("Commentary"));
			p_tool_button->setIcon(QIcon(":/comment_bw.png"));
			break;
		case HearingImpairedCaptionsSequence:
			p_label->setText(tr("HI"));
			p_tool_button->setIcon(QIcon(":/hi_bw.png"));
			break;
		case KaraokeSequence:
			p_label->setText(tr("Karaoke"));
			p_tool_button->setIcon(QIcon(":/microphone_bw.png"));
			break;
		case MainAudioSequence:
			p_label->setText(tr("Audio"));
			p_tool_button->setIcon(QIcon(":/sound_bw.png"));
			break;
		case MainImageSequence:
			p_label->setText(tr("Video"));
			p_tool_button->setIcon(QIcon(":/film_bw.png"));
			break;
		case SubtitlesSequence:
			p_label->setText(tr("Subtitles"));
			p_tool_button->setIcon(QIcon(":/text_bw.png"));
			break;
		case VisuallyImpairedTextSequence:
			p_label->setText(tr("VI"));
			p_tool_button->setIcon(QIcon(":/vi_bw.png"));
			break;
		case ForcedNarrativeSequence:
			p_label->setText(tr("Forced Narrative"));
			p_tool_button->setIcon(QIcon(":/text_bw.png"));
			break;
		case MarkerSequence:
			p_label->setText(tr("Marker"));
			p_tool_button->setIcon(QIcon(":/marker_bw.png"));
			break;
		case IABSequence:
			p_label->setText(tr("IAB Audio"));
			p_tool_button->setIcon(QIcon(":/sound_bw.png"));
			break;
		case ISXDSequence:
			p_label->setText(tr("ISXD RDD47"));
			p_tool_button->setIcon(QIcon(":/xml-icon.png"));
			break;
		case MGASADMSignalSequence:
			p_label->setText(tr("MGA S-ADM"));
			p_tool_button->setIcon(QIcon(":/sound_bw.png"));
			break;
		case ADMAudioSequence:
			p_label->setText(tr("ADM Audio"));
			p_tool_button->setIcon(QIcon(":/sound_bw.png"));
			break;
		case  Unknown:
		default:
			p_label->setText(tr("Unknown"));
			break;
	}
	p_tool_button->setAutoRaise(true);
	p_tool_button->setPopupMode(QToolButton::InstantPopup);
	p_tool_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	QMenu *p_menu = new QMenu(this);
	mpDelete = new QAction(QIcon(":/delete.png"), tr("&Remove Track"), this);
	mShowTrackId = new QAction(QIcon(":/information.png"), tr("&Show Track ID"), this);
	connect(p_menu, SIGNAL(aboutToShow()), this, SLOT(EnableDeleteAction()));
	connect(mpDelete, SIGNAL(triggered()), this, SLOT(DeleteAction()));
	connect(mShowTrackId, SIGNAL(triggered()), this, SLOT(ShowTrackId()));
	p_menu->addAction(mpDelete);
	p_menu->addAction(mShowTrackId);
	p_menu->addSeparator();
// 	p_menu->addAction(QIcon(":/up.png"), tr("Move &Up"), this, SLOT(MoveUpAction()));
// 	p_menu->addAction(QIcon(":/down.png"), tr("Move &Down"), this, SLOT(MoveDownAction()));
	p_tool_button->setMenu(p_menu);
	QHBoxLayout *p_layout = new QHBoxLayout();
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->addWidget(p_tool_button);
	p_layout->addWidget(p_label);

	p_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	setLayout(p_layout);

	mpMsgBox = new QMessageBox(this);
	mpMsgBox->setIcon(QMessageBox::Information);
}

void WidgetTrackDetails::EnableDeleteAction() {

	if (mType == MainImageSequence)
		mpDelete->setDisabled(true);
	else
		mpDelete->setEnabled(true);
}

void WidgetTrackDetails::ShowTrackId() {

	mpMsgBox->setText(tr("Track ID:"));
	mpMsgBox->setIcon(QMessageBox::Information);
	mpMsgBox->setInformativeText(this->GetId().toString().remove("{").remove("}"));
	mpMsgBox->setStandardButtons(QMessageBox::Ok);
	mpMsgBox->setDefaultButton(QMessageBox::Ok);
	mpMsgBox->exec();
}

QSize WidgetTrackDetails::sizeHint() const {

	QSize size = QFrame::sizeHint();
	if(mType == MarkerSequence) size.setHeight(32);
	else size.setHeight(80);
	return size;
}

WidgetAudioTrackDetails::WidgetAudioTrackDetails(const QUuid &rTrackId, QWidget *pParent /*= NULL*/) :
AbstractWidgetTrackDetails(pParent), mTrackId(rTrackId) {

	InitLayout();
}

QSize WidgetAudioTrackDetails::sizeHint() const {

	QSize size = QFrame::sizeHint();
	size.setHeight(80);
	return size;
}

void WidgetAudioTrackDetails::InitLayout() {

	QToolButton *p_tool_button = new QToolButton(this);
	p_tool_button->setSizePolicy(p_tool_button->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
	QLabel *p_label = new QLabel(this);
	p_label->setText(tr("Audio"));
	p_tool_button->setIcon(QIcon(":/sound_bw.png"));
	p_tool_button->setAutoRaise(true);
	p_tool_button->setPopupMode(QToolButton::InstantPopup);
	p_tool_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	QMenu *p_menu = new QMenu(this);
	p_menu->addAction(QIcon(":/delete.png"), tr("&Remove Track"), this, SLOT(DeleteAction()));
	mShowTrackId = new QAction(QIcon(":/information.png"), tr("&Show Track ID"), this);
	connect(mShowTrackId, SIGNAL(triggered()), this, SLOT(ShowTrackId()));
	p_menu->addAction(mShowTrackId);
	p_menu->addSeparator();
	p_tool_button->setMenu(p_menu);
	QHBoxLayout *p_layout = new QHBoxLayout();
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->addWidget(p_tool_button);
	p_layout->addWidget(p_label);
	p_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	setLayout(p_layout);
	mpMsgBox = new QMessageBox(this);
	mpMsgBox->setIcon(QMessageBox::Information);
}

void WidgetAudioTrackDetails::ShowTrackId() {

	mpMsgBox->setText(tr("Track ID:"));
	mpMsgBox->setIcon(QMessageBox::Information);
	mpMsgBox->setInformativeText(this->GetId().toString().remove("{").remove("}"));
	mpMsgBox->setStandardButtons(QMessageBox::Ok);
	mpMsgBox->setDefaultButton(QMessageBox::Ok);
	mpMsgBox->exec();
}

