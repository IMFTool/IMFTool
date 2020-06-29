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
class WidgetTimedTextPreview;
class TimelineParser;
class WidgetContentVersionList; //WR
class WidgetLocaleList; //WR

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
	void SaveAllCpl() const;
	void CopyCPL(const QSharedPointer<AssetCpl> &rDestination);
	int GetIndex(const QUuid &rCplAssetId);
	QUndoStack* GetUndoStack(int index) const;
	QUndoStack* GetCurrentUndoStack() const;
	//WR begin
	QSharedPointer<ImfPackage> GetMpImfPackage() const {return mpImfPackage;}

	//WidgetVideoPreview *mpPreview; // (k)
	QVector<VideoResource> playlist; // (k) make private?
	QVector<TTMLtimelineResource> ttmls;

signals:
	void UndoStackChanged(QUndoStack *pStack);
	void CplSaveStateChanged(bool isDirty);
//WR begin
	void SaveAllCplFinished() const;
//WR end;


	void UpdateStatusBar(const QString &, const int &, const QString &); // (k)
private slots:
	void rCurrentChanged(int tabWidgetIndex);
	void rToggleTTML(int tabWidgetIndex);
	void rTabCloseRequested(int index);
public slots:
	void rUpdatePlaylist(); // (k)
	void rPlaylistFinished(); // (k)
	void rPrevFrame(); // (k)
	void rNextFrame(); // (k)
private:
	Q_DISABLE_COPY(WidgetCentral);
	void InitLyout();
	void SaveCpl(int index) const;

	QSharedPointer<ImfPackage> mpImfPackage;
	QSharedPointer<AssetCpl> mpDestination;
	//QSharedPointer<AssetCpl> mpDestination; delete?
	QMessageBox *mpMsgBox;
	QTabWidget *mpTabWidget;
	WidgetVideoPreview *mpPreview;
	QTabWidget *mpTabDetailTTML; // (k)
	WidgetTimedTextPreview *mpTTMLDetailsWidget; // (k)
	WidgetCompositionInfo *mpDetailsWidget;
	WidgetContentVersionList *mpContentVersionListWidget; //WR
	WidgetLocaleList *mpLocaleListWidget;
	QThread *tpThread; // (k)
	TimelineParser *timelineParser; // (k)
	bool playListUpdateSuccess = true; // (k)
	bool uninstalling_imp = false;
	QTime *timelineParserTime;
	const QMap<QString, eImfApplications> mApplicationIdentificationIntegerMap {
			{"http://www.smpte-ra.org/schemas/2067-20/2016", ::App2},
			{"http://www.smpte-ra.org/schemas/2067-21/2016", ::App2e},
			{"http://www.smpte-ra.org/schemas/2067-40/2016", ::App4},
			{"http://www.smpte-ra.org/schemas/2067-20/2013", ::App2},
			{"http://www.smpte-ra.org/schemas/2067-21/2014", ::App2e},
			{"http://www.smpte-ra.org/ns/2067-50/2017", ::App5},
			{"http://www.smpte-ra.org/ns/2067-40/2020 http://www.smpte-ra.org/ns/2067-40-DCDM/2020", ::App4DCDM_HTJ2K},
			{"http://www.smpte-ra.org/ns/2067-40-DCDM/2020 http://www.smpte-ra.org/ns/2067-40/2020", ::App4DCDM_HTJ2K},
			{"http://www.smpte-ra.org/ns/2067-40/2020", ::App4},
	};
};
