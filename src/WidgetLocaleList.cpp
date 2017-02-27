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
#include "WidgetLocaleList.h"
#include "WidgetComposition.h"
#include "UndoProxyModel.h"
#include "WidgetLocaleListCommands.h"
#include <QLineEdit>
#include <QLabel>
#include <QDataWidgetMapper>
#include <QSpacerItem>
#include <QPushButton>
#include <QList>

WidgetLocaleList::WidgetLocaleList(QWidget *pParent /*= NULL*/) :
QTreeWidget(pParent), mpMapper(NULL), /*mpModel(NULL),*/ mpProxyModel(NULL) {

	InitLayout();
}

void WidgetLocaleList::InitLayout() {
	setColumnCount(2);
	setHeaderHidden(true);
}

void WidgetLocaleList::SetComposition(WidgetComposition *pComposition) {

	clear();
	LocaleList locale_list = pComposition->GetLocaleList();

	LocaleList::iterator i;
	for (i = locale_list.begin(); i < locale_list.end(); i++) {
		Locale locale = *i;
		QStringList items;
		if (!locale.getAnnotation().first.isEmpty())
			items << QString("Annotation") << locale.getAnnotation().first;
		else
			items << QString("Annotation") << "(not present)";
		QTreeWidgetItem *annotationNode = new QTreeWidgetItem(this, items);

		QList<QString> language_list = locale.getLanguageList();
		QTreeWidgetItem *child_languages = new QTreeWidgetItem(annotationNode, QStringList(QObject::tr("LanguageList")));

		foreach( QString lang, language_list ) {
			QStringList langEntry;
			langEntry << "Language" << lang;
			child_languages->addChild(new QTreeWidgetItem(langEntry));
		}

		QList<QString> region_list = locale.getRegionList();
		QTreeWidgetItem *child_regions = new QTreeWidgetItem(annotationNode, QStringList(QObject::tr("RegionList")));

		foreach( QString region, region_list ) {
			QStringList regionEntry;
			regionEntry << "Region" << region;
			child_regions->addChild(new QTreeWidgetItem(regionEntry));
		}

		QList<ContentMaturityRating> rating_list = locale.getContentMaturityRating();
		QTreeWidgetItem *child_ratings = new QTreeWidgetItem(annotationNode, QStringList(QObject::tr("ContentMaturityRatingList")));

		foreach( ContentMaturityRating rating, rating_list ) {
			QStringList agencyEntry;
			agencyEntry << "Agency" << rating.getAgency().first;

			QTreeWidgetItem *ratingNode = new QTreeWidgetItem(child_ratings, agencyEntry);

			QStringList ratingEntry;
			ratingEntry << "Rating" << rating.getRating().first;
			ratingNode->addChild(new QTreeWidgetItem(ratingEntry));

			if (!rating.getAudience().second.isEmpty()) {
				QStringList audienceURIEntry;
				audienceURIEntry << "Audience scope" << rating.getAudience().first;
				ratingNode->addChild(new QTreeWidgetItem(audienceURIEntry));
				QStringList audienceEntry;
				audienceEntry << "Audience" << rating.getAudience().second;
				ratingNode->addChild(new QTreeWidgetItem(audienceEntry));
			}
		}

		expandItem(annotationNode);
		resizeColumnToContents(0);
	}
}

void WidgetLocaleList::Clear() {

	//mpProxyModel->SetUndoStack(NULL);
	//mpModel->SetComposition(NULL, NULL);
	//mpMapper->toFirst();
	//delete mpUndoStack;
	clear();
}

