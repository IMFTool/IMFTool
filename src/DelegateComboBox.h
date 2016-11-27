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
#pragma once
#include <QStyledItemDelegate>


class DelegateComboBox : public QStyledItemDelegate {

	Q_OBJECT

public:
	DelegateComboBox(QObject *pParent = NULL, bool showPopup = true, bool editable = false);
	virtual ~DelegateComboBox() {}
	virtual QWidget* createEditor(QWidget *pParent, const QStyleOptionViewItem &rOption, const QModelIndex &rIndex) const;
	virtual void setEditorData(QWidget *pEditor, const QModelIndex &rIndex) const;
	virtual void setModelData(QWidget *pEditor, QAbstractItemModel *pModel, const QModelIndex &rIndex) const;

	private slots:
	void rCurrentIndexChanged(int index);

private:
	Q_DISABLE_COPY(DelegateComboBox);
	bool mShowPopup;
	bool mEdiable;
};