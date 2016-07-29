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
#pragma once
#include <QWidget>
#include <QAbstractTableModel>
#include <QPointer>


class QDataWidgetMapper;
class CompositionInfoModel;
class WidgetComposition;
class CompositionInfoModel;
class UndoProxyModel;

class WidgetCompositionInfo : public QWidget {

public:
	WidgetCompositionInfo(QWidget *pParent = NULL);
	virtual ~WidgetCompositionInfo() {}
	void SetComposition(WidgetComposition *pComposition);
	void Clear();

private:
	Q_DISABLE_COPY(WidgetCompositionInfo);
	void InitLayout();

	QDataWidgetMapper *mpMapper;
	CompositionInfoModel *mpModel;
	UndoProxyModel *mpProxyModel;
};


class CompositionInfoModel : public QAbstractTableModel {

	Q_OBJECT

public:
	enum eModelColumn {
		ColumnContentTitle = 0,
		ColumnIssuer,
		ColumnContentOriginator,
		ColumnContentKind,
		ColumnAnnotation,
		ColumnEditRate,
		ColumnIssuerDate,
		ColumnContentVersionTag,
		ColumnMax
	};
	CompositionInfoModel(QObject *pParent = NULL);
	virtual ~CompositionInfoModel() {}
	//! Tracks deletion of composition.
	void SetComposition(WidgetComposition *pComposition);
	virtual Qt::ItemFlags flags(const QModelIndex &rIndex) const;
	virtual int rowCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &rIndex, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &rIndex, const QVariant &rValue, int role = Qt::EditRole);

private:
	Q_DISABLE_COPY(CompositionInfoModel);

	QPointer<WidgetComposition> mpComposition;
};
