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
#include <QWizard>
#include "ImfPackage.h"



class QLabel;
class QMessageBox;
class MetadataExtractor;

class WizardEssenceDescriptor : public QWizard {

	Q_OBJECT

public:
	WizardEssenceDescriptor(QWidget *pParent = NULL, QSharedPointer<AssetMxfTrack> rAsset = QSharedPointer<AssetMxfTrack>());
	virtual ~WizardEssenceDescriptor() {}
	virtual QSize sizeHint() const;

private:
	Q_DISABLE_COPY(WizardEssenceDescriptor);
	void	InitLayout();
	QSharedPointer<AssetMxfTrack> mAsset;
	int		mPageId;
};

class WizardEssenceDescriptorPage : public QWizardPage {
public:
	WizardEssenceDescriptorPage(QWidget *pParent = NULL, QSharedPointer<AssetMxfTrack> rAsset = QSharedPointer<AssetMxfTrack>());
private:
	void InitLayout();
	QSharedPointer<AssetMxfTrack> mAsset;

};
