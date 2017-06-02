/* Copyright(C) 2017 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
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
#include "WidgetLocaleList.h"
#include "WidgetComposition.h"
#include <QUndoCommand>


class AddLocaleCommand : public QUndoCommand {

public:
	AddLocaleCommand(const QPointer<WidgetComposition> rpComposition, WidgetLocaleList* rWidget, const int rPos, LocaleListModel *rModel, QUndoCommand *pParent = NULL);
	virtual ~AddLocaleCommand() {}
	virtual void undo();
	//!! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddLocaleCommand);
	const QPointer<WidgetComposition> mpComposition;
	WidgetLocaleList* mWidget;
	int mPos;
	LocaleList mSavedLocaleList;
};


class RemoveLocaleCommand : public QUndoCommand {

public:
	RemoveLocaleCommand(const QPointer<WidgetComposition> rpComposition, WidgetLocaleList* rWidget, const int rPos, LocaleListModel *rModel, QUndoCommand *pParent = NULL);
	virtual ~RemoveLocaleCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveLocaleCommand);
	QPointer<WidgetComposition> mpComposition;
	WidgetLocaleList* mWidget;
	LocaleList mSavedLocaleList;
	int mPos;
	LocaleListModel *mModel;
};

class AddItemCommand : public QUndoCommand {

public:
	AddItemCommand(const QPointer<WidgetComposition> rpComposition, WidgetLocaleList* rWidget, const int rParentRow, const QString rIdentifier, LocaleListModel *rModel, QUndoCommand *pParent = NULL);
	virtual ~AddItemCommand() {}
	virtual void undo();
	//!! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddItemCommand);
	const QPointer<WidgetComposition> mpComposition;
	WidgetLocaleList* mWidget;
	//const QModelIndex mIndex;
	LocaleList mSavedLocaleList;
	LocaleListModel* mpModel;
	int mParentRow;
	QString mIdentifier;

};


