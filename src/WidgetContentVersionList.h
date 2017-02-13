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
#include <QWidget>
#include <QAbstractTableModel>
#include <QPointer>
#include <QGridLayout>
#include <QUndoStack>


class QDataWidgetMapper;
class ContentVersionListModel;
class WidgetComposition;
class ContentVersionListModel;
class UndoProxyModel;

class WidgetContentVersionList : public QWidget {

	Q_OBJECT

public:
	WidgetContentVersionList(QWidget *pParent = NULL);
	virtual ~WidgetContentVersionList() {}
	void SetComposition(WidgetComposition *pComposition);
	void Clear();

private:
	Q_DISABLE_COPY(WidgetContentVersionList);
	void InitLayout();

	QDataWidgetMapper *mpMapper;
	ContentVersionListModel *mpModel;
	UndoProxyModel *mpProxyModel;
	QGridLayout *p_layout;
	QUndoStack *mpUndoStack;

private slots:
	void slotDeleteClicked();
	void slotAddClicked();
};


class ContentVersionListModel : public QAbstractTableModel {

	Q_OBJECT

public:
	ContentVersionListModel(QObject *pParent = NULL);
	virtual ~ContentVersionListModel() {}
	//! Tracks deletion of composition.
	void SetComposition(WidgetComposition *pComposition, QUndoStack *rpUndoStack);
	virtual Qt::ItemFlags flags(const QModelIndex &rIndex) const;
	virtual int rowCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &rIndex, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &rIndex, const QVariant &rValue, int role = Qt::EditRole);
	void deleteItem(const int rPos, WidgetContentVersionList* widget);
	void addItem(WidgetContentVersionList* widget);

private:
	Q_DISABLE_COPY(ContentVersionListModel);

	QPointer<WidgetComposition> mpComposition;
	QUndoStack *mpUndoStack;
};
