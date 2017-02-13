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
#include "WidgetContentVersionListCommands.h"
#include "global.h"


AddContentVersionCommand::AddContentVersionCommand(const QPointer<WidgetComposition> rpComposition, WidgetContentVersionList* rWidget, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Add Content Version"), pParent), mpComposition(rpComposition), mWidget(rWidget), mPositionAdded(0) {

}

void AddContentVersionCommand::undo() {

	ContentVersionList cvl = mpComposition->GetContentVersionList();
	cvl.removeAt(mPositionAdded);
	mpComposition->SetContentVersionList(cvl);
	mWidget->SetComposition(mpComposition);
}

void AddContentVersionCommand::redo() {

	ContentVersionList cvl = mpComposition->GetContentVersionList();
	cvl.append(ContentVersion(QString("urn:uuid:").append(strip_uuid(QUuid::createUuid())), UserText()));
	mpComposition->SetContentVersionList(cvl);
	mWidget->SetComposition(mpComposition);
	mPositionAdded = cvl.count() - 1;
}

RemoveContentVersionCommand::RemoveContentVersionCommand(const QPointer<WidgetComposition> rpComposition, WidgetContentVersionList* rWidget, const int rPos, ContentVersionListModel *rModel, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Remove Content Version"), pParent),mpComposition(rpComposition), mWidget(rWidget), mSavedContentVersion(ContentVersion()), mPos(rPos), mModel(rModel)  {

}

void RemoveContentVersionCommand::undo() {

	ContentVersionList cvl = mpComposition->GetContentVersionList();
	cvl.insert(mPos/3, mSavedContentVersion);
	mpComposition->SetContentVersionList(cvl);
	//emit dataChanged(createIndex(1, rPos), createIndex(1, rPos+1));
	mWidget->SetComposition(mpComposition);
}

void RemoveContentVersionCommand::redo() {
	ContentVersionList cvl = mpComposition->GetContentVersionList();
	mSavedContentVersion = cvl.at(mPos/3);
	cvl.removeAt(mPos/3);
	mpComposition->SetContentVersionList(cvl);
	//emit dataChanged(createIndex(1, rPos), createIndex(1, rPos+1));
	mModel->removeColumns(mPos, 3);
	mWidget->SetComposition(mpComposition);

}
