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
#include "UndoProxyModel.h"


SetEditRoleDataCommand::SetEditRoleDataCommand(const QPersistentModelIndex &rModelIndex, const QString &rData, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mData(rData), mDataBackup(rModelIndex.data(Qt::EditRole).toString()), mModelIndex(rModelIndex) {

}

void SetEditRoleDataCommand::undo() {

	if(mModelIndex.isValid() && mModelIndex.model()) {
		QAbstractItemModel *p_model = const_cast<QAbstractItemModel*>(mModelIndex.model());
		p_model->setData(mModelIndex, QVariant(mDataBackup));
	}
}

void SetEditRoleDataCommand::redo() {

	if(mModelIndex.isValid() && mModelIndex.model()) {
		QAbstractItemModel *p_model = const_cast<QAbstractItemModel*>(mModelIndex.model());
		p_model->setData(mModelIndex, QVariant(mData), Qt::EditRole);
	}
}

UndoProxyModel::UndoProxyModel(QUndoStack *pUndoStack /*= NULL*/, QObject *pParent /*= NULL*/) :
QIdentityProxyModel(pParent), mpUndoStack(pUndoStack) {

}

bool UndoProxyModel::setData(const QModelIndex &rIndex, const QVariant &rValue, int role /*= Qt::EditRole*/) {

	if(mpUndoStack) {
		if(role == Qt::EditRole) {
			if(rIndex.isValid() && rIndex.data(Qt::EditRole).canConvert<QString>() && (rIndex.data(Qt::EditRole).toString() != rValue.toString())) {
				mpUndoStack->push(new SetEditRoleDataCommand(QPersistentModelIndex(sourceModel()->index(rIndex.row(), rIndex.column(), rIndex.parent())), rValue.toString()));
			}
			emit dataChanged(rIndex, rIndex);
			return true;
		}
	}
	return QIdentityProxyModel::setData(rIndex, rValue, role);
}

void UndoProxyModel::SetUndoStack(QUndoStack *pUndoStack) {

	mpUndoStack = pUndoStack;
}
