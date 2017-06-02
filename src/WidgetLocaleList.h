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
#include <QAbstractItemModel>
#include <QStandardItem>
#include <QPointer>
#include <QGridLayout>
#include <QUndoStack>
#include <QTreeView>
#include "ImfPackageCommon.h"


class QDataWidgetMapper;
class LocaleListModel;
class WidgetComposition;
class UndoProxyModel;

class WidgetLocaleList : public QWidget {

	Q_OBJECT

public:
	WidgetLocaleList(QWidget *pParent = NULL);
	virtual ~WidgetLocaleList() {}
	void SetComposition(WidgetComposition *pComposition);
	void Clear();

private:
	Q_DISABLE_COPY(WidgetLocaleList);
	void InitLayout();

	QDataWidgetMapper *mpMapper;
	LocaleListModel *mpModel;
	UndoProxyModel *mpProxyModel;
	QTreeView *p_tree;
	QHBoxLayout *p_layout;
	QUndoStack *mpUndoStack;
	QPointer<WidgetComposition> mpComposition;
	QMenu* contextMenu;

private slots:
	void slotRemoveRow();
	void slotInsertRow();
	void slotAppendRow();
	void slotAppendItem();
	void onCustomContextMenu(const QPoint &point);
};

class LocaleTreeItem; // forward

class LocaleListModel : public QAbstractItemModel {

	Q_OBJECT

public:
    explicit LocaleListModel(QObject *parent = 0);
    ~LocaleListModel();
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const override	;
    void clear();

    void SetComposition(WidgetComposition *pComposition, QUndoStack *rpUndoStack);
    void deleteItem(const int rPos, WidgetLocaleList* widget);
    void addLocale(const int rPos, WidgetLocaleList* widget);
    void addItem(const QModelIndex &index, WidgetLocaleList* widget);

private:
	Q_DISABLE_COPY(LocaleListModel);

    LocaleTreeItem* getItem(const QModelIndex &index) const;
    void SetLocaleListFromModel(WidgetComposition *pComposition) const;
	QPointer<WidgetComposition> mpComposition;
	QUndoStack *mpUndoStack;
	LocaleTreeItem *mpRootItem;
};

class LocaleTreeItem
{
public:
    explicit LocaleTreeItem(const QVector<QVariant> &data, LocaleTreeItem *parentItem = 0);
    ~LocaleTreeItem();

    void appendChild(QVector<QVariant> &itemList);

    LocaleTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool insertChild(int position, int columns);
    LocaleTreeItem *parent();
    int childNumber() const;
    bool setData(int column, const QVariant &value);

private:
    QList<LocaleTreeItem*> childItems;
    QVector<QVariant> itemData;
    LocaleTreeItem *parentItem;
};
