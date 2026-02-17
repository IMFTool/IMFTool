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
#include "WidgetLocaleListCommands.h"
#include "global.h"


AddLocaleCommand::AddLocaleCommand(const QPointer<WidgetComposition> rpComposition, WidgetLocaleList* rWidget, const int rPos, LocaleListModel *rModel, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Add Locale"), pParent), mpComposition(rpComposition), mWidget(rWidget), mPos(rPos) {

}

void AddLocaleCommand::undo() {

	mpComposition->SetLocaleList(mSavedLocaleList);
	mWidget->SetComposition(mpComposition);
	return;
}

void AddLocaleCommand::redo() {

	LocaleList lol = mpComposition->GetLocaleList();
	mSavedLocaleList = lol;
	if (mPos < lol.length()) lol.insert(mPos, Locale(QString("(not present)"), QList<QString>(), QList<QString>(), QList<class ContentMaturityRating>()));
	else {
		mPos = lol.length();
		lol.append(Locale(QString("(not present)"), QList<QString>(), QList<QString>(), QList<class ContentMaturityRating>()));
	}
	mpComposition->SetLocaleList(lol);
	mWidget->SetComposition(mpComposition);
}

RemoveLocaleCommand::RemoveLocaleCommand(const QPointer<WidgetComposition> rpComposition, WidgetLocaleList* rWidget, const int rPos, LocaleListModel *rModel, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Remove Loc"), pParent),mpComposition(rpComposition), mWidget(rWidget), mPos(rPos), mModel(rModel)  {

}

void RemoveLocaleCommand::undo() {

	mpComposition->SetLocaleList(mSavedLocaleList);
	mWidget->SetComposition(mpComposition);
	return;
}

void RemoveLocaleCommand::redo() {
	LocaleList lol = mpComposition->GetLocaleList();
	mSavedLocaleList = lol;
	lol.removeAt(mPos);
	mpComposition->SetLocaleList(lol);
	mModel->removeRow(mPos);
	mWidget->SetComposition(mpComposition);

}

AddItemCommand::AddItemCommand(const QPointer<WidgetComposition> rpComposition, WidgetLocaleList* rWidget, const int rParentRow, const QString rIdentifier, LocaleListModel *rModel, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Add Item"), pParent), mpComposition(rpComposition), mWidget(rWidget), mParentRow(rParentRow), mIdentifier(rIdentifier), mpModel(rModel) {

}

void AddItemCommand::redo() {
	LocaleList lol = mpComposition->GetLocaleList();
	mSavedLocaleList = lol;
	Locale loc = lol.at(mParentRow);

	if (mIdentifier == "LanguageList") {
		QList<QString> list = loc.getLanguageList();
		list.append("enter language");
		loc.setLanguageList(list);
	}
	else if (mIdentifier == "RegionList") {
		QList<QString> list = loc.getRegionList();
		list.append("enter region");
		loc.setRegionList(list);
	}
	else if (mIdentifier == "ContentMaturityRatingList") {
		QList<ContentMaturityRating> list = loc.getContentMaturityRating();
		list.append(ContentMaturityRating("http://enter.agency.URI", "Enter Rating", QPair<QString, QString>("http://scope.example.invalid/audience-URI", "E.g. Adults, Children")));
		loc.setContentMaturityRating(list);
	}
	lol.replace(mParentRow, loc);
	mpComposition->SetLocaleList(lol);
	mWidget->SetComposition(mpComposition);

}

void AddItemCommand::undo() {

	mpComposition->SetLocaleList(mSavedLocaleList);
	mWidget->SetComposition(mpComposition);
}
