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
#include "DelegateComboBox.h"
#include "global.h"
#include <QComboBox>
#include <QStringList>
#include <QCompleter>


DelegateComboBox::DelegateComboBox(QObject *pParent /*= NULL*/, bool showPopup /*= true*/, bool editable /*= false*/) :
QStyledItemDelegate(pParent), mShowPopup(showPopup), mEdiable(editable) {

}

QWidget* DelegateComboBox::createEditor(QWidget *pParent, const QStyleOptionViewItem &rOption, const QModelIndex &rIndex) const {

	QComboBox *p_combo_box = new QComboBox(pParent);
	connect(p_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(rCurrentIndexChanged(int)));
	return p_combo_box;
}

void DelegateComboBox::setEditorData(QWidget *pEditor, const QModelIndex &rIndex) const {

	QComboBox *p_combo_box = qobject_cast<QComboBox*>(pEditor);
	if(p_combo_box) {
		QStringList list = rIndex.data(UserRoleComboBox).toStringList();
		p_combo_box->clear();
		p_combo_box->addItems(list);
		QString show_index = rIndex.data(Qt::DisplayRole).toString();
		int goto_index = p_combo_box->findText(show_index);
		p_combo_box->setCurrentIndex(goto_index);
		p_combo_box->setEditable(mEdiable);
		if (p_combo_box->isEditable() == true) {
			if(p_combo_box->completer() != NULL) p_combo_box->completer()->setCompletionMode(QCompleter::PopupCompletion);
		}
		p_combo_box->setInsertPolicy(QComboBox::NoInsert);
		if(mShowPopup == true) p_combo_box->showPopup();
	}
	else {
		qWarning() << "Couldn't extract combobox";
	}
}

void DelegateComboBox::setModelData(QWidget *pEditor, QAbstractItemModel *pModel, const QModelIndex &rIndex) const {

	QComboBox *p_combo_box = qobject_cast<QComboBox*>(pEditor);
	if(p_combo_box) {
		pModel->setData(rIndex, p_combo_box->currentText(), Qt::EditRole);
	}
	else {
		qWarning() << "Couldn't extract combobox";
	}
}

void DelegateComboBox::rCurrentIndexChanged(int index) {

	QWidget *editor = qobject_cast<QWidget*>(sender());
	if(editor) {
		emit commitData(editor);
		emit closeEditor(editor);
	}
	else {
		qWarning() << "Couldn't extract combobox";
	}
}
