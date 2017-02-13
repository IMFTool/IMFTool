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
#include "ImfPackageCommon.h"
#include "WidgetContentVersionList.h"
#include "WidgetComposition.h"
#include <QUndoCommand>


class AddContentVersionCommand : public QUndoCommand {

public:
	AddContentVersionCommand(const QPointer<WidgetComposition> rpComposition, WidgetContentVersionList* rWidget, QUndoCommand *pParent = NULL);
	virtual ~AddContentVersionCommand() {}
	virtual void undo();
	//!! Called ounce when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddContentVersionCommand);
	const QPointer<WidgetComposition> mpComposition;
	WidgetContentVersionList* mWidget;
	int mPositionAdded;
};


class RemoveContentVersionCommand : public QUndoCommand {

public:
	RemoveContentVersionCommand(const QPointer<WidgetComposition> rpComposition, WidgetContentVersionList* rWidget, const int rPos, ContentVersionListModel *rModel, QUndoCommand *pParent = NULL);
	virtual ~RemoveContentVersionCommand() {}
	virtual void undo();
	//! Called ounce when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveContentVersionCommand);
	QPointer<WidgetComposition> mpComposition;
	WidgetContentVersionList* mWidget;
	ContentVersion mSavedContentVersion;
	int mPos;
	ContentVersionListModel *mModel;
};
