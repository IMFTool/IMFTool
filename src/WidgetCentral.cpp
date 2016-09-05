/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel
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
#include "WidgetCentral.h"
#include "WidgetComposition.h"
#include "WidgetCompositionInfo.h"
#include "ImfPackage.h"
#include "ImfCommon.h"
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>


WidgetCentral::WidgetCentral(QWidget *pParent /*= NULL*/) :
QWidget(pParent), mpImfPackage(), mpMsgBox(NULL), mpTabWidget(NULL), mpPreview(NULL), mpDetailsWidget(NULL) {

	InitLyout();
}

WidgetCentral::~WidgetCentral() {

	UninstallImp();
}

void WidgetCentral::InitLyout() {

	mpMsgBox = new QMessageBox(this);
	mpMsgBox->setIcon(QMessageBox::Warning);

	mpTabWidget = new QTabWidget(this);
	mpTabWidget->setTabsClosable(true);
	mpTabWidget->setMovable(true);
	QFrame *p_tab_widget_frame = new QFrame(this);
	p_tab_widget_frame->setFrameStyle(QFrame::StyledPanel);
	QHBoxLayout *p_tab_widget_frame_layout = new QHBoxLayout();
	p_tab_widget_frame_layout->setContentsMargins(0, 0, 0, 0);
	p_tab_widget_frame_layout->addWidget(mpTabWidget);
	p_tab_widget_frame->setLayout(p_tab_widget_frame_layout);


	mpDetailsWidget = new WidgetCompositionInfo(this);
	mpDetailsWidget->setDisabled(true);
	QFrame *p_details_widget_frame = new QFrame(this);
	p_details_widget_frame->setFrameStyle(QFrame::StyledPanel);
	QHBoxLayout *p_details_widget_frame_layout = new QHBoxLayout();
	p_details_widget_frame_layout->setContentsMargins(0, 0, 0, 0);
	p_details_widget_frame_layout->addWidget(mpDetailsWidget);
	p_details_widget_frame->setLayout(p_details_widget_frame_layout);

	QSplitter *p_inner_splitter = new QSplitter(this);
	p_inner_splitter->setOrientation(Qt::Horizontal);
	p_inner_splitter->setChildrenCollapsible(false);
	p_inner_splitter->setOpaqueResize(true);
	p_inner_splitter->addWidget(p_details_widget_frame);

	QSplitter *p_outer_splitter = new QSplitter(this);
	p_outer_splitter->setOrientation(Qt::Vertical);
	p_outer_splitter->setChildrenCollapsible(false);
	p_outer_splitter->setOpaqueResize(true);
	p_outer_splitter->addWidget(p_details_widget_frame);
	p_outer_splitter->addWidget(p_tab_widget_frame);
	QList<int> sizes;
	sizes << mpDetailsWidget->sizeHint().height() << -1;
	p_outer_splitter->setSizes(sizes);
	p_outer_splitter->setStretchFactor(1, 1);

	QHBoxLayout *p_layout = new QHBoxLayout();
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->addWidget(p_outer_splitter);
	setLayout(p_layout);

	connect(mpTabWidget, SIGNAL(currentChanged(int)), this, SLOT(rCurrentChanged(int)));
	connect(mpTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(rTabCloseRequested(int)));
}

void WidgetCentral::rCurrentChanged(int tabWidgetIndex) {

	WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->currentWidget());
	if(p_composition) {
	}
	p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(tabWidgetIndex));
	if(p_composition) {
		mpDetailsWidget->SetComposition(p_composition);
	}
}

void WidgetCentral::rTabCloseRequested(int index) {

	WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(index));
	if(p_composition) {
		if(p_composition->GetUndoStack() && p_composition->GetUndoStack()->count() > 0) {
			mpMsgBox->setText(tr("Save changes?"));
			mpMsgBox->setInformativeText(tr("The composition has unsaved changes!"));
			mpMsgBox->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
			mpMsgBox->setDefaultButton(QMessageBox::Save);
			mpMsgBox->setIcon(QMessageBox::Warning);
			int ret = mpMsgBox->exec();
			if(ret == QMessageBox::Save) {
				SaveCpl(index);
			}
			else if(ret == QMessageBox::Cancel) { return; }
		}
		mpTabWidget->removeTab(mpTabWidget->indexOf(p_composition));
		p_composition->deleteLater();
	}
}

int WidgetCentral::ShowCplEditor(const QUuid &rCplAssetId) {

	for(int i = 0; i < mpTabWidget->count(); i++) {
		WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(i));
		if(p_composition && p_composition->GetCplAssetId() == rCplAssetId) {
			mpTabWidget->setCurrentIndex(i);
			return i;
		}
	}
	WidgetComposition *p_widget = new WidgetComposition(mpImfPackage, rCplAssetId);
	QSharedPointer<Asset> asset = mpImfPackage->GetAsset(rCplAssetId);
	ImfError error = p_widget->Read();
	if(error.IsError() == false) {
		if(error.IsRecoverableError() == true) {
			QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
			mpMsgBox->setText(tr("CPL Warning?"));
			mpMsgBox->setInformativeText(error_msg);
			mpMsgBox->setIcon(QMessageBox::Warning);
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
		}
		int index = mpTabWidget->addTab(p_widget, mpImfPackage->GetAsset(rCplAssetId)->GetOriginalFileName().first);
		mpTabWidget->setCurrentWidget(p_widget);
		return index;
	}
	else {
		QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
		mpMsgBox->setText(tr("CPL Critical?"));
		mpMsgBox->setInformativeText(error_msg);
		mpMsgBox->setIcon(QMessageBox::Critical);
		mpMsgBox->setStandardButtons(QMessageBox::Ok);
		mpMsgBox->setDefaultButton(QMessageBox::Ok);
		mpMsgBox->exec();
	}
	return -1;
}

void WidgetCentral::InstallImp(const QSharedPointer<ImfPackage> &rImfPackage) {

	UninstallImp();
	mpImfPackage = rImfPackage;
	mpDetailsWidget->setEnabled(true);
}

void WidgetCentral::UninstallImp() {

	mpDetailsWidget->setDisabled(true);
	mpDetailsWidget->Clear();
	for(int i = 0; i < mpTabWidget->count(); i++) {
		if(QWidget *p_widget = mpTabWidget->widget(i)) {
			p_widget->deleteLater();
		}
	}
	mpImfPackage.clear();
}

int WidgetCentral::GetIndex(const QUuid &rCplAssetId) {

	int ret = -1;
	for(int i = 0; i < mpTabWidget->count(); i++) {
		if(WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(i))) {
			if(p_composition->GetCplAssetId().isNull() == false && p_composition->GetCplAssetId() == rCplAssetId) {
				ret = i;
				break;
			}
		}
	}
	return ret;
}

QUndoStack* WidgetCentral::GetUndoStack(int index) const {

	if(WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(index))) {
		return p_composition->GetUndoStack();
	}
	return NULL;
}

QUndoStack* WidgetCentral::GetCurrentUndoStack() const {

	WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->currentWidget());
	if(p_composition) return p_composition->GetUndoStack();
	return NULL;
}

void WidgetCentral::SaveCurrentCpl() const {

	SaveCpl(mpTabWidget->currentIndex());
}

void WidgetCentral::SaveAllCpl() const {

	for(int i = 0; i < mpTabWidget->count(); i++) {
		//QSharedPointer<AssetCpl> asset_cpl = this->GetMpImfPackage()->GetAsset(0).objectCast<AssetCpl>();
		WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(i));
		QSharedPointer<AssetCpl> asset_cpl = this->GetMpImfPackage()->GetAsset(p_composition->GetCplAssetId()).objectCast<AssetCpl>();

		if (asset_cpl->GetIsNewOrModified()) SaveCpl(i);
		if (asset_cpl->GetIsNewOrModified()) qDebug() << "asset_cpl->GetIsNewOrModified";
	}
	//WR begin
	emit SaveAllCplFinished();
	//WR end
}

void WidgetCentral::SaveCpl(int index) const {

	WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(index));
	if(p_composition) {
		ImfError error = p_composition->Write();
		if(error.IsError() == false) {
			if(error.IsRecoverableError() == true) {
				QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
				mpMsgBox->setText(tr("CPL Warning?"));
				mpMsgBox->setInformativeText(error_msg);
				mpMsgBox->setIcon(QMessageBox::Warning);
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->exec();
			}
		}
		else {
			QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
			mpMsgBox->setText(tr("CPL Critical?"));
			mpMsgBox->setInformativeText(error_msg);
			mpMsgBox->setIcon(QMessageBox::Critical);
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
		}
	}
}

				/* -----Denis Manthey----- */
void WidgetCentral::CopyCPL(const QSharedPointer<AssetCpl> &rDestination) {

	WidgetComposition *p_composition = qobject_cast<WidgetComposition*>(mpTabWidget->widget(mpTabWidget->currentIndex()));
	if(p_composition) {
		QUuid oldId = p_composition->GetId();
		p_composition->SetID(rDestination.data()->GetId());
		ImfError error = p_composition->WriteNew(rDestination.data()->GetPath().absoluteFilePath());
		//We will undo all changes for this object:
		QSharedPointer<AssetCpl> asset_cpl = mpImfPackage->GetAsset(oldId).objectCast<AssetCpl>();
		asset_cpl.data()->SetIsNewOrModified(false);
		p_composition->SetID(oldId);
		if(error.IsError() == false) {
			if(error.IsRecoverableError() == true) {
				QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
				mpMsgBox->setText(tr("CPL Warning?"));
				mpMsgBox->setInformativeText(error_msg);
				mpMsgBox->setIcon(QMessageBox::Warning);
				mpMsgBox->setStandardButtons(QMessageBox::Ok);
				mpMsgBox->setDefaultButton(QMessageBox::Ok);
				mpMsgBox->exec();
			}
		}
		else {
			QString error_msg = QString("%1\n%2").arg(error.GetErrorMsg()).arg(error.GetErrorDescription());
			mpMsgBox->setText(tr("CPL Critical?"));
			mpMsgBox->setInformativeText(error_msg);
			mpMsgBox->setIcon(QMessageBox::Critical);
			mpMsgBox->setStandardButtons(QMessageBox::Ok);
			mpMsgBox->setDefaultButton(QMessageBox::Ok);
			mpMsgBox->exec();
		}
	}
	for (int i = 0; i < GetCurrentUndoStack()->count(); i++) {
		GetCurrentUndoStack()->undo();
	}
	GetCurrentUndoStack()->clear();
}
