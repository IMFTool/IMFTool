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
#pragma once
#include "ImfPackage.h"
#include <QWidget>


class QTabWidget;
class WidgetVideoPreview;
class QMessageBox;	
class WidgetCompositionInfo;


class WidgetCentral : public QWidget {

	Q_OBJECT

public:
	WidgetCentral(QWidget *pParent = NULL);
	virtual ~WidgetCentral();
	void InstallImp(const QSharedPointer<ImfPackage> &rImfPackage);
	void UninstallImp();
	bool IsImpInstalled() const { return mpImfPackage; }
	int ShowCplEditor(const QUuid &rCplAssetId);
	void SaveCurrentCpl() const;
	void CopyCPL(const QSharedPointer<AssetCpl> &rDestination);
	int GetIndex(const QUuid &rCplAssetId);
	QUndoStack* GetUndoStack(int index) const;
	QUndoStack* GetCurrentUndoStack() const;

signals:
	void UndoStackChanged(QUndoStack *pStack);

private slots:
void rCurrentChanged(int tabWidgetIndex);
void rTabCloseRequested(int index);

private:
	Q_DISABLE_COPY(WidgetCentral);
	void InitLyout();
	void SaveCpl(int index) const;

	QSharedPointer<ImfPackage> mpImfPackage;
	QSharedPointer<AssetCpl> mpDestination;
	QMessageBox *mpMsgBox;
	QTabWidget *mpTabWidget;
	WidgetVideoPreview *mpPreview;
	WidgetCompositionInfo *mpDetailsWidget;
};
