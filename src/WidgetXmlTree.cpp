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
#include "WidgetComposition.h"
#include "UndoProxyModel.h"
#include "WidgetXmlTree.h"
#include <QLineEdit>
#include <QLabel>
#include <QDataWidgetMapper>
#include <QSpacerItem>
#include <QPushButton>
#include <QList>
#include <QMenu>

WidgetXmlTree::WidgetXmlTree(QWidget *pParent /*= NULL*/) :
QWidget(pParent), mpMapper(NULL), /*mpModel(NULL),*/ mpProxyModel(NULL) {

	InitLayout();
}

void WidgetXmlTree::InitLayout() {
	p_tree = new QTreeView(this);
	p_tree->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(p_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));
	mpModel = new XmlModel(this);
	mpProxyModel = new UndoProxyModel(NULL, this);
	mpProxyModel->setSourceModel(mpModel);
	mpMapper = new QDataWidgetMapper(this);
	mpMapper->setModel(mpProxyModel);
	p_tree->setModel(mpProxyModel);
	mpMapper->toFirst();
	p_tree->setHeaderHidden(true);
	p_layout = new QHBoxLayout();
	p_layout->setContentsMargins(0, 0, 0, 0);
	p_layout->addWidget(p_tree);
	setLayout(p_layout);
	this->setEnabled(false);

}

void WidgetXmlTree::SetComposition(WidgetComposition *pComposition) {

	mpComposition = pComposition;
	mpModel->clear();
	mpProxyModel->SetUndoStack(pComposition->GetUndoStack());
	mpUndoStack = pComposition->GetUndoStack();

	mpModel->SetComposition(pComposition, mpUndoStack);

	p_tree->expandAll();
	p_tree->resizeColumnToContents(0);
	//p_tree->setColumnWidth(0, 2*p_tree->columnWidth(0));

	p_tree->resizeColumnToContents(1);
	mpMapper->toFirst();
	this->setEnabled(true);

}

void WidgetXmlTree::Clear() {

	//mpProxyModel->SetUndoStack(NULL);
	//mpModel->SetComposition(NULL, NULL);
	//mpMapper->toFirst();
	//delete mpUndoStack;
	mpModel->clear();
	this->setEnabled(false);
}

void WidgetXmlTree::onCustomContextMenu(const QPoint &point) {
	if (!this->isEnabled()) return;

	QModelIndex index = p_tree->indexAt(point);
	contextMenu = new QMenu(p_tree);

    if (index.isValid() && !index.parent().isValid() && index.column() == 0 ) { // Annotation
		QAction *remove_action = new QAction(QIcon(":/delete.png"), tr("Remove Locale"), this);
		QAction *insert_action = new QAction(QIcon(":/add.png"), tr("Insert Locale before this one"), this);
		QAction *append_action = new QAction(QIcon(":/add.png"), tr("Insert Locale after this one"), this);
		connect(remove_action, SIGNAL(triggered(bool)), this, SLOT(slotRemoveRow()));
		connect(insert_action, SIGNAL(triggered(bool)), this, SLOT(slotInsertRow()));
		connect(append_action, SIGNAL(triggered(bool)), this, SLOT(slotAppendRow()));
		contextMenu->addAction(remove_action);
		contextMenu->addAction(insert_action);
		contextMenu->addAction(append_action);
        contextMenu->exec(p_tree->mapToGlobal(point));
    }
    else if (index.isValid() && index.column() == 0 && !index.parent().parent().isValid() ) { //Object has root as grand-parent, i.e. LanguageList, RegionList, ContentMaturityRatingList
		QAction *append_action = new QAction(QIcon(":/add.png"), tr("Add"), this);
		connect(append_action, SIGNAL(triggered(bool)), this, SLOT(slotAppendItem()));
		contextMenu->addAction(append_action);
        contextMenu->exec(p_tree->mapToGlobal(point));
    } else if (!index.isValid()){
		QAction *insert_action = new QAction(QIcon(":/add.png"), tr("Insert Locale"), this);
		connect(insert_action, SIGNAL(triggered(bool)), this, SLOT(slotInsertRow()));
		contextMenu->addAction(insert_action);
        contextMenu->exec(p_tree->mapToGlobal(point));
    }
}

XmlModel::XmlModel(QObject *pParent /*= NULL*/) :
QAbstractItemModel(pParent), mpComposition(NULL) {
	QVector<QVariant> rootData;
    rootData << QString("Elements") << QString("Values");
    mpRootItem = new XmlTreeItem(rootData);
}

XmlModel::~XmlModel() {
	delete mpRootItem;
}


bool XmlModel::setData(const QModelIndex &rIndex, const QVariant &rValue, int role /*= Qt::EditRole*/) {
	const int row = rIndex.row();
	const int column = rIndex.column();
	QString lValue = static_cast<XmlTreeItem*>(rIndex.internalPointer())->data(0).toString();
	// List field are not allow to contain a value in the second column
	if(role == Qt::EditRole && !lValue.contains("List")) {

		if (static_cast<XmlTreeItem*>(rIndex.internalPointer())->data(0).toString() == "Agency") {
			if (rValue.toString().isEmpty()) return false; // TODO Will also need regexp to check for valid URI!
		}
		if (static_cast<XmlTreeItem*>(rIndex.internalPointer())->data(0).toString() == "Rating") {
			if (rValue.toString().isEmpty()) return false;
		}
		static_cast<XmlTreeItem*>(rIndex.internalPointer())->setData(column, rValue);
		SetLocaleListFromModel(mpComposition);
		emit dataChanged(rIndex, rIndex);
		return true;
	}
	return false;

}

QModelIndex XmlModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    XmlTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = mpRootItem;
    else
        parentItem = static_cast<XmlTreeItem*>(parent.internalPointer());

    XmlTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();

}
QModelIndex XmlModel::parent(const QModelIndex &index) const {
    if (!index.isValid())
        return QModelIndex();

    XmlTreeItem *childItem = getItem(index);
    XmlTreeItem *parentItem = childItem->parent();

    if (parentItem == mpRootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int XmlModel::rowCount(const QModelIndex &parent) const {
    XmlTreeItem *parentItem;

    if (parent.column() > 0)
        return 0;

    if (!parent.isValid()) {
        parentItem = mpRootItem;
    }
    else {
        parentItem = getItem(parent);
    }
    return parentItem->childCount();
}

int XmlModel::columnCount(const QModelIndex &parent) const {
	//Didn't work - all items forced to have two columns.

	return mpRootItem->columnCount();
}

QVariant XmlModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    XmlTreeItem *item = getItem(index);
    if (index.column() < item->columnCount())
    	return item->data(index.column());
    else {
    	return QString();
    }

}

Qt::ItemFlags XmlModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return 0;
    if (index.column() == 0) return QAbstractItemModel::flags(index);
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

XmlTreeItem* XmlModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        XmlTreeItem *item = static_cast<XmlTreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return mpRootItem;
}


void XmlModel::SetComposition(WidgetComposition *pComposition, QUndoStack *rpUndoStack) {

	mpComposition = pComposition;
	mpUndoStack = rpUndoStack;
	LocaleList locale_list = pComposition->GetLocaleList();

	LocaleList::iterator i;
	for (i = locale_list.begin(); i < locale_list.end(); i++) {
		XmlTreeItem* parentItem;
		Locale locale = *i;
		QVector<QVariant> itemList;

		itemList << QString("Annotation");
		if (!locale.getAnnotation().first.isEmpty())
			itemList << locale.getAnnotation().first;
		else
			itemList << "(not present)";
		mpRootItem->appendChild(itemList);

		parentItem = mpRootItem->child(mpRootItem->childCount() - 1);
		itemList.clear();
		itemList << QString("LanguageList");
		parentItem->appendChild(itemList);

		XmlTreeItem* parentItem2 = parentItem->child(parentItem->childCount() - 1);
		QList<QString> language_list = locale.getLanguageList();
		foreach( QString lang, language_list ) {

			itemList.clear();
			itemList << QString("Language") << lang;
			parentItem2->appendChild(itemList);
		}

		itemList.clear();
		itemList << QString("RegionList");
		parentItem->appendChild(itemList);
		parentItem2 = parentItem->child(parentItem->childCount() - 1);
		QList<QString> region_list = locale.getRegionList();

		foreach( QString region, region_list ) {
			itemList.clear();
			itemList << QString("Region") << region;
			parentItem2->appendChild(itemList);
		}

		itemList.clear();
		itemList << QString("ContentMaturityRatingList");
		parentItem->appendChild(itemList);
		parentItem2 = parentItem->child(parentItem->childCount() - 1);
		QList<ContentMaturityRating> rating_list = locale.getContentMaturityRating();

		foreach( ContentMaturityRating rating, rating_list ) {
			itemList.clear();
			QStringList agencyEntry;
			itemList << QString("Agency") << rating.getAgency().first;
			parentItem2->appendChild(itemList);
			XmlTreeItem *agencyParentItem = parentItem2->child(parentItem2->childCount() - 1);

			QVector<QVariant> itemList2;
			itemList2 << QString("Rating") << rating.getRating().first;
			agencyParentItem->appendChild(itemList2);
			if (!rating.getAudience().second.isEmpty()) {
				QVector<QVariant> itemList;
				itemList << QString("Audience scope") << rating.getAudience().first;
				agencyParentItem->appendChild(itemList);
				itemList.clear();
				itemList << QString("Audience") << rating.getAudience().second;
				agencyParentItem->appendChild(itemList);
			}
		}


	}
}

void XmlModel::SetLocaleListFromModel(WidgetComposition *pComposition) const {


	LocaleList localeList = pComposition->GetLocaleList();
	localeList.clear();

	for (int i=0; i < mpRootItem->childCount(); i++) {
		//Locale locale;
		QList<QString> languageList;
		QList<QString> regionList;
		QList<class ContentMaturityRating> cmrList;
		for (int ii=0; ii < mpRootItem->child(i)->childCount(); ii++) {
			XmlTreeItem* item = mpRootItem->child(i)->child(ii);
			if (item->data(0) == "LanguageList") {
				for (int iii=0; iii < item->childCount(); iii++) languageList << item->child(iii)->data(1).toString();
			}

			else if (item->data(0) == "RegionList") {
				for (int iii=0; iii < item->childCount(); iii++) regionList << item->child(iii)->data(1).toString();
			}

			else if (item->data(0) == "ContentMaturityRatingList") {
				for (int iii=0; iii < item->childCount(); iii++) {
					XmlTreeItem* agency = item->child(iii);
					ContentMaturityRating cmr;
					cmr.setAgency(UserText(agency->data(1).toString()));
					for (int iiii=0; iiii < agency->childCount(); iiii++) {
						if (agency->child(iiii)->data(0) == "Rating") cmr.setRating(UserText(agency->child(iiii)->data(1).toString()));
						else if (agency->child(iiii)->data(0) == "Audience scope") {
							cmr.setAudience(QPair<QString, QString>(agency->child(iiii)->data(1).toString(), QString()));
						}
						else if (agency->child(iiii)->data(0) == "Audience") {
							cmr.setAudience(QPair<QString, QString>(cmr.getAudience().first, agency->child(iiii)->data(1).toString()));

						}
					}
					cmrList << cmr;
				}
			}
		}
		localeList.append(Locale(UserText(mpRootItem->child(i)->data(1).toString()), languageList, regionList, cmrList));

	}

	pComposition->SetLocaleList(localeList);
	return;
}


void XmlModel::clear(){
   this->beginResetModel();
   // clear the content of your model here
   delete mpRootItem;
   QVector<QVariant> rootData;
   rootData << "Elements" << "Values";
   mpRootItem = new XmlTreeItem(rootData);
   this->endResetModel();
}

XmlTreeItem::XmlTreeItem(const QVector<QVariant> &data, XmlTreeItem *parent) {
    parentItem = parent;
    itemData = data;
}

XmlTreeItem::~XmlTreeItem() {
    qDeleteAll(childItems);
}

XmlTreeItem *XmlTreeItem::child(int number) {
    return childItems.value(number);
}

int XmlTreeItem::childCount() const {
    return childItems.count();
}

int XmlTreeItem::childNumber() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<XmlTreeItem*>(this));

    return 0;
}

int XmlTreeItem::columnCount() const {
    return itemData.count();
}

QVariant XmlTreeItem::data(int column) const {
    return itemData.value(column);
}

bool XmlTreeItem::insertChild(int position, int columns) {
    if (position < 0 || position > childItems.size())
        return false;

	QVector<QVariant> data(columns);
	XmlTreeItem *item = new XmlTreeItem(data, this);
	childItems.insert(position, item);

    return true;
}

void XmlTreeItem::appendChild(QVector<QVariant> &itemList) {
	if (itemList.size() == 0)
		return;
	else {
		insertChild(childCount(), itemList.size());
		for (int column = 0; column < itemList.size(); ++column)
			this->child(this->childCount() - 1)->setData(column, itemList[column]);
	}
}

XmlTreeItem *XmlTreeItem::parent() {
    return parentItem;
}

bool XmlTreeItem::setData(int column, const QVariant &value) {
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

