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
#include "ImfPackageCommands.h"
#include "global.h"


AddAssetCommand::AddAssetCommand(const QSharedPointer<ImfPackage> &rImfPackage, const QSharedPointer<Asset> &rAsset, const QUuid &rPklId /*= QUuid()*/, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Add Asset %1").arg(rAsset->GetId().toString()), pParent), mpImfPackage(rImfPackage), mpAsset(rAsset), mPklIdBackup(rPklId) {

}

void AddAssetCommand::undo() {

	mPklIdBackup = mpAsset->GetPklId();
	mpImfPackage->RemoveAsset(mpAsset->GetId());
}

void AddAssetCommand::redo() {

	// Try backup packing list id.
	if(mpImfPackage->AddAsset(mpAsset, mPklIdBackup) == true) return;
	// Use next best packing list id.
	else mpImfPackage->AddAsset(mpAsset, mpImfPackage->GetPackingListId());
}

RemoveAssetCommand::RemoveAssetCommand(const QSharedPointer<ImfPackage> &rImfPackage, const QSharedPointer<Asset> &rAsset, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(QObject::tr("Remove Asset %1").arg(rAsset->GetId().toString()), pParent), mpImfPackage(rImfPackage), mpAsset(rAsset), mPklIdBackup(QUuid()) {

}

void RemoveAssetCommand::undo() {

	// Try backup packing list id.
	if(mpImfPackage->AddAsset(mpAsset, mPklIdBackup) == true) return;
	// Use next best packing list id.
	else mpImfPackage->AddAsset(mpAsset, mpImfPackage->GetPackingListId());
}

void RemoveAssetCommand::redo() {

	mPklIdBackup = mpAsset->GetPklId();
	mpImfPackage->RemoveAsset(mpAsset->GetId());
}
