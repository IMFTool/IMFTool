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
#include "WidgetCompositionInfo.h"
#include "WidgetComposition.h"
#include "UndoProxyModel.h"
#include <QGridLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QDataWidgetMapper>


WidgetCompositionInfo::WidgetCompositionInfo(QWidget *pParent /*= NULL*/) :
QWidget(pParent), mpMapper(NULL), mpModel(NULL), mpProxyModel(NULL) {

	InitLayout();
}

void WidgetCompositionInfo::InitLayout() {

	mpModel = new CompositionInfoModel(this);
	mpProxyModel = new UndoProxyModel(NULL, this);
	mpProxyModel->setSourceModel(mpModel);
	mpMapper = new QDataWidgetMapper(this);
	mpMapper->setModel(mpProxyModel);
	QLineEdit *p_content_title = new QLineEdit(this);
	p_content_title->setPlaceholderText(tr("--The title for the composition--"));
	mpMapper->addMapping(p_content_title, CompositionInfoModel::ColumnContentTitle);
	QLineEdit *p_annotation = new QLineEdit(this);
	p_annotation->setPlaceholderText(tr("--Annotation describing the composition--"));
	mpMapper->addMapping(p_annotation, CompositionInfoModel::ColumnAnnotation);
	QLineEdit *p_issue_date = new QLineEdit(this);
	p_issue_date->setDisabled(true);
	mpMapper->addMapping(p_issue_date, CompositionInfoModel::ColumnIssuerDate);
	QLineEdit *p_issuer = new QLineEdit(this);
	p_issuer->setPlaceholderText(tr("--The entity that created the Composition Playlist--"));
	mpMapper->addMapping(p_issuer, CompositionInfoModel::ColumnIssuer);
	QLineEdit *p_content_originator = new QLineEdit(this);
	p_content_originator->setPlaceholderText(tr("--The originator of the content underlying the composition--"));
	mpMapper->addMapping(p_content_originator, CompositionInfoModel::ColumnContentOriginator);

	//WR
	/*	QComboBox *p_content_kind = new QComboBox(this);
	p_content_kind->setEditable(true);*/
	QLineEdit *p_content_kind = new QLineEdit(this);
	p_content_kind->setDisabled(true);
	//WR
	mpMapper->addMapping(p_content_kind, CompositionInfoModel::ColumnContentKind);
	QLineEdit *p_edit_rate = new QLineEdit(this);
	p_edit_rate->setDisabled(true);
	mpMapper->addMapping(p_edit_rate, CompositionInfoModel::ColumnEditRate);
	mpMapper->toFirst();

	QGridLayout *p_layout = new QGridLayout();
	p_layout->addWidget(new QLabel(tr("Content Title:")), 0, 0, 1, 1);
	p_layout->addWidget(p_content_title, 0, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Issuer:")), 1, 0, 1, 1);
	p_layout->addWidget(p_issuer, 1, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Content Originator:")), 2, 0, 1, 1);
	p_layout->addWidget(p_content_originator, 2, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Content Kind:")), 3, 0, 1, 1);
	p_layout->addWidget(p_content_kind, 3, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Annotation:")), 4, 0, 1, 1);
	p_layout->addWidget(p_annotation, 4, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Edit Rate:")), 5, 0, 1, 1);
	p_layout->addWidget(p_edit_rate, 5, 1, 1, 1);
	p_layout->addWidget(new QLabel(tr("Issue Date:")), 6, 0, 1, 1);
	p_layout->addWidget(p_issue_date, 6, 1, 1, 1);
	p_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 7, 0, 1, 2);
	setLayout(p_layout);
}

void WidgetCompositionInfo::SetComposition(WidgetComposition *pComposition) {

	mpProxyModel->SetUndoStack(pComposition->GetUndoStack());
	mpModel->SetComposition(pComposition);
	mpMapper->toFirst();
}

void WidgetCompositionInfo::Clear() {

	mpProxyModel->SetUndoStack(NULL);
	mpModel->SetComposition(NULL);
	mpMapper->toFirst();
}

CompositionInfoModel::CompositionInfoModel(QObject *pParent /*= NULL*/) :
QAbstractTableModel(pParent), mpComposition(NULL) {

}

Qt::ItemFlags CompositionInfoModel::flags(const QModelIndex &rIndex) const {

	const int column = rIndex.column();

	if(column == ColumnContentTitle
		 || column == ColumnIssuer
		 || column == ColumnContentOriginator
		 //WR
		 //|| column == ColumnContentKind
		 //WR
		 || column == ColumnAnnotation) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int CompositionInfoModel::rowCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	return 1;
}

int CompositionInfoModel::columnCount(const QModelIndex &rParent /*= QModelIndex()*/) const {

	return ColumnMax;
}

QVariant CompositionInfoModel::data(const QModelIndex &rIndex, int role /*= Qt::DisplayRole*/) const {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(row == 0 && mpComposition) {
		if(column == ColumnContentTitle) {
			if(role == Qt::EditRole) {
				return mpComposition->GetContentTitle().first;
			}
		}
		else if(column == ColumnIssuer) {
			if(role == Qt::EditRole) {
				return mpComposition->GetIssuer().first;
			}
		}
		else if(column == ColumnContentOriginator) {
			if(role == Qt::EditRole) {
				return mpComposition->GetContentOriginator().first;
			}
		}
		else if(column == ColumnAnnotation) {
			if(role == Qt::EditRole) {
				return mpComposition->GetAnnotation().first;
			}
		}
		else if(column == ColumnEditRate) {
			if(role == Qt::EditRole) {
				return mpComposition->GetEditRate().GetName();
			}		
		}
		else if(column == ColumnIssuerDate) {
			if(role == Qt::EditRole) {
				return mpComposition->GetIssuerDate().toString(Qt::SystemLocaleShortDate);
			}
		}
		//WR
		else if(column == ColumnContentKind) {
			if(role == Qt::EditRole) {
				return mpComposition->GetContentKind().first;
			}
		}
		//WR
	}
	return QVariant();
}

bool CompositionInfoModel::setData(const QModelIndex &rIndex, const QVariant &rValue, int role /*= Qt::EditRole*/) {

	const int row = rIndex.row();
	const int column = rIndex.column();

	if(row == 0 && mpComposition) {
		if(role == Qt::EditRole) {
			if(column == ColumnContentTitle && rValue.canConvert<QString>()) {
				mpComposition->SetContentTitle(rValue.toString());
				emit dataChanged(rIndex, rIndex);
				return true;
			}
			else if(column == ColumnIssuer && rValue.canConvert<QString>()) {
				mpComposition->SetIssuer(rValue.toString());
				emit dataChanged(rIndex, rIndex);
				return true;
			}
			else if(column == ColumnContentOriginator && rValue.canConvert<QString>()) {
				mpComposition->SetContentOriginator(rValue.toString());
				emit dataChanged(rIndex, rIndex);
				return true;
			}
			else if(column == ColumnAnnotation && rValue.canConvert<QString>()) {
				mpComposition->SetAnnotation(rValue.toString());
				emit dataChanged(rIndex, rIndex);
				return true;
			}
		}
	}
	return false;
}

void CompositionInfoModel::SetComposition(WidgetComposition *pComposition) {

	beginResetModel();
	mpComposition = pComposition;
	endResetModel();
}
