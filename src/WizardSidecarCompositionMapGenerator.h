/* Copyright(C) 2018 Justin Hug, Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
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
#include "MetadataExtractorCommon.h"
#include <QWizard>
#include <QAbstractTableModel>
#include <QStringList>
#include <QPersistentModelIndex>
#include <QMap>
#include <QFileInfo>
#include <QLineEdit>
#include <QGroupBox>
#include "ImfPackage.h"
#include <QTableView>
#include <QStandardItemModel>
#include <QFrame>

#include <QRadioButton>


//#define ASSET_ID_DYNAMIK_PROPERTY "AssetId"
//#define ASSET_ESSENCE_TYPE "EssenceType"

class QDialogButtonBox;
class QFileDialog;
class QStringListModel;
class QTableView;
class QLabel;
class WidgetProxyImage;
class QtWaitingSpinner;
class QStackedLayout;
class QComboBox;
class QMessageBox;
class MetadataExtractor;
class AdditionalInfoPage;

class WizardSidecarCompositionMapGenerator : public QWizard {

	Q_OBJECT

public:
	enum eMode {
		NewScm,
		ViewScm,
		EditScm,
	};

	enum { Page_SelectAssets, Page_SelectCpl, Page_AddAditionalInfo};

	WizardSidecarCompositionMapGenerator(QWidget *pParent, QSharedPointer<ImfPackage> rImfPackage, QVector< QSharedPointer<AssetCpl> > rCplAssets = QVector< QSharedPointer<AssetCpl> >(), QSharedPointer<AssetScm> rAssetScm = QSharedPointer<AssetScm>());
	virtual ~WizardSidecarCompositionMapGenerator() {}
	virtual QSize sizeHint() const;
	void SwitchMode(eMode rMode);
	eMode GetMode() { return mMode; };

	void setAssetFiles(QList<QStandardItem*>);
	QList<QStandardItem *> getAssetFiles();
	void setSidecarCompositionMapEntry(QStandardItem* file, QVector< QSharedPointer<AssetCpl> > rCplAssets, QList<QUuid> rCplIdsNotInCurrentImp);
	void deleteSidecarCompositionMap();
	QList<AssetScm::SidecarCompositionMapEntry*> getSidecarCompositionMapEntryList();
	void setSidecarCompositionMapEntryList(QList<AssetScm::SidecarCompositionMapEntry*> rSidecarCompositionMapEntryList)
			{ mSidecarCompositionMapEntryList = rSidecarCompositionMapEntryList; };
	QList<AssetScm::SidecarCompositionMapEntry*> getInitialSidecarCompositionMapEntries();
	QString getAnnotation();
	QString getIssuer();

	void setAnnotation(QString mAnnotation);
	void setIssuer(QString mIssuer);

    QSharedPointer<AssetScm> getAssetScm() { return mAssetScm; };

public slots:
	
private:
	Q_DISABLE_COPY(WizardSidecarCompositionMapGenerator);
	void	InitLayout();
	AdditionalInfoPage* mAdditionalInfoPage;
	//input private members
	QVector< QSharedPointer<AssetCpl> > mCplAssets; //CPL assets of IMP
	QSharedPointer<AssetScm> mAssetScm;  // SCM to be edited in EditScm mode
	QList<AssetScm::SidecarCompositionMapEntry*> mInitialSidecarCompositionMapEntryList;
	QSharedPointer<ImfPackage> mpImfPackage;
	QString mImfPackagePath;
	//output private members
	QList<QStandardItem *> mSidecarAssets; // Siedecar files selected in dialog
	QList<AssetScm::SidecarCompositionMapEntry*> mSidecarCompositionMapEntryList;
	QString mIssuer;
	QString mAnnotation;
	eMode mMode;

};

class sAssetsPage : public QWizardPage
{
	Q_OBJECT
		Q_PROPERTY(QStringList FilesSelected READ GetSourceFiles WRITE SetSourceFiles NOTIFY FilesListChanged)

public:

	sAssetsPage(QWidget *parent, QSharedPointer<ImfPackage> rImfPackage);
	QFileDialog *mpFileDialog;

public slots:
	void SetSourceFiles(const QStringList &rFiles);
	QStringList GetSourceFiles() const;
	void rCustomMenuRequested(QPoint pos);
	void rRemoveSelectedRow();
	void rRemoveSelectedRow(QModelIndex index);

protected:
	void initializePage() override;
	virtual void keyPressEvent(QKeyEvent *pEvent) override;

signals:
	void FilesListChanged();

private:
	QLabel * topLabel;
	QTableView *mpTableViewScmFiles;
	QStandardItemModel *mpModelScmFiles;
	QString mImfPackagePath;
	QList<QStandardItem *> mSideAssetsToAdd;
	QWidget* mpParent;
	QSharedPointer<ImfPackage> mpImfPackage;
	QPushButton *mButtonBrowse;

private slots:
	void ShowFileDialog();
	void SetScmFileDirectory(const QString&);
};

class sCplPage : public QWizardPage
{
	Q_OBJECT
		Q_PROPERTY(QList<QUuid> CplsSelected READ GetSelectedCpls WRITE SetSelectedCpls NOTIFY CplListChanged)
		
public:

	sCplPage(QWidget *parent, QVector< QSharedPointer<AssetCpl> >, QString filedirectory);
	void SetSelectedCpls(QList<QUuid> rSelectedCpls);
	

public slots:
	void SetCplFilesList(QVector< QSharedPointer<AssetCpl> > rCplAssets);
	QList<QUuid> GetSelectedCpls() const;
	bool selectedOneOfEach() const;
	
	virtual bool isComplete() const;
	virtual void initializePage();
	virtual void cleanupPage();
	virtual bool validatePage();
	virtual void mpTableViewScmCPL_ItemClicked();
	bool handleBackButton();

signals:
	void CplListChanged();
	
private:
	QTableView * mpTableViewScmCPL;
	QStandardItemModel *mpModelScmCPL;
	QVector< QSharedPointer<AssetCpl> > mCplAssets;
	QWidget* mpParent;
	QLineEdit *mpLineEditAnnotation;
	QString mAnnotationText;
	QString mImfPackagePath;
private slots:
	void CplChecked(QStandardItem *);
};


class AdditionalInfoPage : public QWizardPage
{
	Q_OBJECT
		
public:
	AdditionalInfoPage(QWidget *parent, QString filedirectory);
	void setScmList();
	
public slots:
	virtual void initializePage();
	void SetAnnotation(QString rAnnotation);
	void SetIssuer(QString rIssuer);

	virtual bool isComplete() const;

signals:
	void mAnnotationChanged();
	void mIssuerChanged();
	void IssuerChanged();

private:
	QTableView * mpTableViewScmInfo;
	QStandardItemModel *mpModelScmInfo;
	QWidget* mpParent;
	QList<AssetScm::SidecarCompositionMapEntry*> mSidecarCompositionMapEntryList;
	QLineEdit *mpLineEditAnnotation;
	QLineEdit *mpLineEditIssuer;
	QString mImfPackagePath;
	QString mAnnotation;
	QString mIssuer;
};

Q_DECLARE_METATYPE(WizardSidecarCompositionMapGenerator::eMode)
