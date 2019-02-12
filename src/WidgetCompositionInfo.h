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


class QDataWidgetMapper;
class CompositionInfoModel;
class WidgetComposition;
class CompositionInfoModel;
class UndoProxyModel;

class WidgetCompositionInfo : public QWidget {

	Q_OBJECT

public:
	WidgetCompositionInfo(QWidget *pParent = NULL);
	virtual ~WidgetCompositionInfo() {}
	void SetComposition(WidgetComposition *pComposition);
	void Clear();

public slots:
		void onCurrentIndexChanged(QString text);

private:
	Q_DISABLE_COPY(WidgetCompositionInfo);
	void InitLayout();

	QDataWidgetMapper *mpMapper;
	CompositionInfoModel *mpModel;
	UndoProxyModel *mpProxyModel;
	const QMap<QString, QString> mApplicationIdentificationSelectionMap {
			{"http://www.smpte-ra.org/schemas/2067-20/2016", "App #2"},
			{"http://www.smpte-ra.org/schemas/2067-21/2016", "App #2E"},
			{"http://www.smpte-ra.org/schemas/2067-20/2013", "App #2 2013"},
			{"http://www.smpte-ra.org/schemas/2067-21/2014", "App #2E 2014"},
#ifdef APP5_ACES
			{"http://www.smpte-ra.org/ns/2067-50/2017", "App #5 ACES"},
#endif
	};
	const QStringList mContentKindList {
		"advertisement",
		"feature",
		"psa",
		"rating",
		"short",
		"teaser",
		"test",
		"trailer",
		"transitional",
		"episode",
		"highlights",
		"event",
		"supplemental",
		"documentary",
	};

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
		ColumnApplicationIdentification,
		ColumnApplicationString,
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
	const QMap<QString, QString> mApplicationIdentificationMap {
			{"http://www.smpte-ra.org/schemas/2067-20/2016", "App #2"},
			{"http://www.smpte-ra.org/schemas/2067-21/2016", "App #2E"},
			{"http://www.smpte-ra.org/schemas/2067-40/2016", "App #4"},
			{"http://www.smpte-ra.org/schemas/2067-20/2013", "App #2 2013"},
			{"http://www.smpte-ra.org/schemas/2067-21/2014", "App #2E 2014"},
			{"http://www.smpte-ra.org/ns/2067-50/2017", "App #5 ACES"},
			{"tag:apple.com,2017:imf:rdd45:2017", "App ProRes"},
			{"http://www.digitalproductionpartnership.co.uk/schema/imf/TSP2121-1/2018", "Application DPP (ProRes)"},
	};
};
