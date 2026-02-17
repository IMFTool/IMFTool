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
#include "WidgetContentVersionList.h"
#include "WidgetComposition.h"
#include "UndoProxyModel.h"
#include "WidgetContentVersionListCommands.h"
#include <QLineEdit>
#include <QLabel>
#include <QDataWidgetMapper>
#include <QSpacerItem>
#include <QPushButton>

WidgetContentVersionList::WidgetContentVersionList(QWidget *pParent /*= NULL*/) :
QWidget(pParent), mpMapper(NULL), mpModel(NULL), mpProxyModel(NULL) {

	InitLayout();
}

void WidgetContentVersionList::InitLayout() {

	mpModel = new ContentVersionListModel(this);
	mpProxyModel = new UndoProxyModel(NULL, this);
	mpProxyModel->setSourceModel(mpModel);
	mpMapper = new QDataWidgetMapper(this);
	mpMapper->setModel(mpProxyModel);
	mpMapper->toFirst();
	p_layout = new QGridLayout();
	p_layout->setVerticalSpacing(3);
	setLayout(p_layout);
	this->setEnabled(false);
}

void WidgetContentVersionList::SetComposition(WidgetComposition *pComposition) {

	if (pComposition == 0) return;

	mpProxyModel->SetUndoStack(pComposition->GetUndoStack());
	mpUndoStack = pComposition->GetUndoStack();
	mpMapper->clearMapping();
	ContentVersionList content_version_list = pComposition->GetContentVersionList();
	if (p_layout) {
		for( int i=p_layout->count() - 1; i >= 0 ; i-- ) {
			QWidget* widget = p_layout->itemAt(i)->widget();
			if( widget ) {
				p_layout->removeWidget(widget);
				delete widget;
			}
		}
	}


	for (int i = 0; i < content_version_list.count(); i++ ) {
		QLineEdit *p_id = new QLineEdit(this);
		//p_id->setEnabled(false);
		QLineEdit *p_label_text = new QLineEdit(this);
		p_label_text->setMinimumWidth(350);
		QPushButton *p_delete_button = new QPushButton("Delete");
		p_layout->addWidget(new QLabel(tr("Id:")), 3*i, 0, 1, 1);
		mpMapper->addMapping(p_id, 3*i);
		p_layout->addWidget(p_id, 3*i, 1, 1, 1);
		p_layout->addWidget(new QLabel(tr("LabelText:")), 3*i+1, 0, 1, 1);
		mpMapper->addMapping(p_label_text, 3*i+1);
		p_layout->addWidget(p_label_text, 3*i+1, 1, 1, 1);
		//mpMapper->addMapping(p_delete_button, 3*i+1);
		p_layout->addWidget(p_delete_button, 3*i, 2, 1, 1);
		connect(p_delete_button, SIGNAL(clicked()), this, SLOT(slotDeleteClicked()));
		p_layout->addItem(new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed), 3*i+2, 0, 1, 3);
	}
	QPushButton *p_add_button = new QPushButton("Add New");
	p_layout->addWidget(p_add_button, 3*content_version_list.count(), 2, 1, 1);
	connect(p_add_button, SIGNAL(clicked()), this, SLOT(slotAddClicked()));
	p_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),2*content_version_list.count()+1, 0, 1, 3);
	mpModel->SetComposition(pComposition, mpUndoStack);
	mpMapper->toFirst();
	this->setEnabled(true);
	}

void WidgetContentVersionList::Clear() {

	mpProxyModel->SetUndoStack(NULL);
	mpModel->SetComposition(NULL, NULL);
	mpMapper->toFirst();
	//SetComposition(0); Didn't work
	//delete mpUndoStack;
	this->setEnabled(false);
}

void WidgetContentVersionList::slotDeleteClicked() {

	QWidget *buttonWidget = qobject_cast<QWidget*>(sender());
	if (!buttonWidget)
	  return;

	int indexOfButton = p_layout->indexOf(buttonWidget);
	int rowOfButton, columnOfButton, rowSpanOfButton, columnSpanOfLabel;

	p_layout->getItemPosition(indexOfButton,
								   &rowOfButton, &columnOfButton, &rowSpanOfButton, &columnSpanOfLabel);
	mpModel->deleteItem(rowOfButton, this);

}

void WidgetContentVersionList::slotAddClicked() {
	mpModel->addItem(this);
}


ContentVersionListModel::ContentVersionListModel(QObject *pParent /*= NULL*/) :
QAbstractTableModel(pParent), mpComposition(NULL) {

}

Qt::ItemFlags ContentVersionListModel::flags(const QModelIndex &rIndex) const {

	const int column = rIndex.column();
	if((column%3 == 0) || (column%3 == 1)) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int ContentVersionListModel::rowCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	return 1;
}

int ContentVersionListModel::columnCount(const QModelIndex &rParent /*= QModelIndex()*/) const {
	if (mpComposition)
		return mpComposition->GetContentVersionList().count()*3 + 2;
	else
		return 0;
}

QVariant ContentVersionListModel::data(const QModelIndex &rIndex, int role /*= Qt::DisplayRole*/) const {
	//WIll be invoked rowCount x columnCount times by the view.

	const int row = rIndex.row();
	const int column = rIndex.column();

	if (!mpComposition->GetContentVersionList().isEmpty()) {
		if(row == 0 && mpComposition) {
			if(role == Qt::EditRole) {
				if ( column%3 == 0 ) {
					return mpComposition->GetContentVersionList().at(column/3).first;
				}
				if ( column%3 == 1 ) {
					return mpComposition->GetContentVersionList().at(column/3).second.first;
				}
			}
		}
	}

	return QVariant();
}

bool ContentVersionListModel::setData(const QModelIndex &rIndex, const QVariant &rValue, int role /*= Qt::EditRole*/) {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if (mpComposition) {
		if(role == Qt::EditRole) {

			ContentVersionList cvl = mpComposition->GetContentVersionList();
			if (column%3 == 0) {
				cvl.replace(column/3, ContentVersion(rValue.toString(), cvl.at(column/3).second));
				mpComposition->SetContentVersionList(cvl);
				emit dataChanged(rIndex, rIndex);
				return true;
			} else if (column%3 == 1) {
				cvl.replace(column/3, ContentVersion(cvl.at(column/3).first, UserText(rValue.toString())));
				mpComposition->SetContentVersionList(cvl);
				emit dataChanged(rIndex, rIndex);
				return true;
			}
		}
	}
	return false;
}

void ContentVersionListModel::SetComposition(WidgetComposition *pComposition, QUndoStack *rpUndoStack) {

	beginResetModel();
	mpComposition = pComposition;
	mpUndoStack = rpUndoStack;
	endResetModel();
}

void ContentVersionListModel::deleteItem(const int rPos, WidgetContentVersionList* widget) {

	beginRemoveColumns(createIndex(1, rPos), rPos, rPos);
	mpUndoStack->push(new RemoveContentVersionCommand(mpComposition, widget, rPos, this));
	endRemoveColumns();
}

void ContentVersionListModel::addItem(WidgetContentVersionList* widget) {

	mpUndoStack->push(new AddContentVersionCommand(mpComposition, widget));
}

