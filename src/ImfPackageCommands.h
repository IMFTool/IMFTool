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
#include "ImfPackage.h"
#include <QUndoCommand>


class AddAssetCommand : public QUndoCommand {

public:
	AddAssetCommand(const QSharedPointer<ImfPackage> &rImfPackage, const QSharedPointer<Asset> &rAsset, const QUuid &rPklId = QUuid(), QUndoCommand *pParent = NULL);
	virtual ~AddAssetCommand() {}
	virtual void undo();
	//!! Called ounce when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddAssetCommand);
	QSharedPointer<ImfPackage>		mpImfPackage;
	QSharedPointer<Asset> mpAsset;
	QUuid													mPklIdBackup;
};


class RemoveAssetCommand : public QUndoCommand {

public:
	RemoveAssetCommand(const QSharedPointer<ImfPackage> &rImfPackage, const QSharedPointer<Asset> &rAsset, QUndoCommand *pParent = NULL);
	virtual ~RemoveAssetCommand() {}
	virtual void undo();
	//! Called ounce when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveAssetCommand);
	QSharedPointer<ImfPackage>		mpImfPackage;
	QSharedPointer<Asset> mpAsset;
	QUuid													mPklIdBackup;
};
