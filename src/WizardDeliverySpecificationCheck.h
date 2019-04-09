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
#include <QTreeWidget>
#include <QProgressDialog>
#include "IMF_DeliverySchema.h"


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
class ResultPage;
class JobQueue;

class WizardDeliverySpecificationCheck : public QWizard {

	Q_OBJECT

public:
	enum { Page_SelectSpecification, Page_SelectDeliverable, Page_SelectCpl, Page_Result};
	WizardDeliverySpecificationCheck(QWidget *parent = 0);

	WizardDeliverySpecificationCheck(QWidget *pParent, QVector< QSharedPointer<AssetCpl> > rCplAssets = QVector< QSharedPointer<AssetCpl> >());
	virtual ~WizardDeliverySpecificationCheck() {}
	virtual QSize sizeHint() const;

	QString GetDeliverySpecificationListPath() {return mSelectedDeliverySpecificationListPath; };
	void SetDeliverySpecificationListPath(const QString rDeliverySpec) { mSelectedDeliverySpecificationListPath = rDeliverySpec; };

	dsl::DeliverySpecificationList* GetDeliverySpecificationList() { return mDeliverySpecificationList; };
	void SetDeliverySpecificationList(dsl::DeliverySpecificationList* rDeliverySpecificationList) {	mDeliverySpecificationList = rDeliverySpecificationList; };

	QVector< QSharedPointer<AssetCpl> > GetSelectedCpls() { return mSelectedCpls; };
	void SetSelectedCpls(QVector< QSharedPointer<AssetCpl> > rSelectedCpls) { mSelectedCpls = rSelectedCpls; };

	QUuid GetDeliverableId() { return mDeliverableId; };
	void SetDeliverableId(const QUuid rDeliverableId) { mDeliverableId = rDeliverableId; };

public slots:
	
private:
	Q_DISABLE_COPY(WizardDeliverySpecificationCheck);
	void	InitLayout();
	QString mSelectedDeliverySpecificationListPath;
	dsl::DeliverySpecificationList* mDeliverySpecificationList;
	QUuid mDeliverableId;
	QVector< QSharedPointer<AssetCpl> > mSelectedCpls;

	ResultPage* mResultPage;
	//input private members
	QVector< QSharedPointer<AssetCpl> > mCplAssets; //CPL assets of IMP

};

/*
 * SelectDeliverySpecificationListPage
 */
class SelectDeliverySpecificationListPage : public QWizardPage
{
	Q_OBJECT
		Q_PROPERTY(QString DeliverySpecificationListSelected READ GetDeliverySpecificationList WRITE SetDeliverySpecificationPath NOTIFY DeliverySpecChanged)

public:
	SelectDeliverySpecificationListPage(QWidget *pParent, QVector< QSharedPointer<AssetCpl> > rCplAssets = QVector< QSharedPointer<AssetCpl> >());
	QFileDialog *mpFileDialog;

public slots:
	void SetSourceFiles(const QStringList &rFiles);

protected:
	void initializePage() override;
	bool validatePage() override;

signals:
	void DeliverySpecChanged();

private:
	QString GetDeliverySpecificationList() {return mSelectedDeliverySpecificationListPath; };
	void SetDeliverySpecificationPath(const QString rDeliverySpec) { mSelectedDeliverySpecificationListPath = rDeliverySpec; };

	QTableView *mpTableViewDeliverySpecs;
	QStandardItemModel* mpModelDeliverySpecs;
	QStringList mDeliverySpecifications;
	QWidget* mpParent;
	QPushButton *mButtonBrowse;
	QRadioButton* mRadioButton1;
	QRadioButton* mRadioButton2;
	QLineEdit* mSelectedFile;
	QString mSelectedDeliverySpecificationListPath;

private slots:
	void ShowFileDialog();
	void slotCellClicked(const QModelIndex &rIndex);
	void button1Clicked();
	void button2Clicked();
};

/*
 * SelectDeliverySpecificationPage
 */
class SelectDeliverySpecificationPage : public QWizardPage
{
	Q_OBJECT
		Q_PROPERTY(QUuid DeliverySpecificationSelected READ GetDeliverySpecificationId WRITE SetDeliverySpecificationId NOTIFY DeliverySpecificationIdChanged)

public:
	SelectDeliverySpecificationPage(QWidget *parent = 0);
	SelectDeliverySpecificationPage(QWidget *pParent, QVector< QSharedPointer<AssetCpl> > rCplAssets = QVector< QSharedPointer<AssetCpl> >());

public slots:
	//void SetSourceFiles(const QString &rFile);
	//QStringList GetSourceFiles() const;

protected:
	void initializePage() override;
	bool validatePage() override;


signals:
	void DeliverySpecificationIdChanged();

private:
	QUuid GetDeliverySpecificationId() {return mDeliverySpecificationId; };
	void SetDeliverySpecificationId(const QUuid rDeliverySpecificationId) { mDeliverySpecificationId = rDeliverySpecificationId; };
	void RemoveLayout (QWidget* widget);

	QVector< QSharedPointer<AssetCpl> > mCplAssets;
	QWidget* mpParent;
	//QString mSelectedDeliverySpecification;
	dsl::DeliverySpecificationList* mDeliverySpecificationList;
	QUuid mDeliverySpecificationId;
	QButtonGroup* mButtonGroup;

private slots:
	void slotButtonClicked(const int index);
};

/*
 * SelectCplPage
 */
class SelectCplPage : public QWizardPage
{
	Q_OBJECT
		Q_PROPERTY(QList<QUuid> CplsSelected READ GetSelectedCpls WRITE SetSelectedCpls NOTIFY CplListChanged)

public:
	SelectCplPage(QWidget *parent = 0);
	SelectCplPage(QWidget *parent, QVector< QSharedPointer<AssetCpl> >);
	void SetSelectedCpls(QList<QUuid> rSelectedCpls);


public slots:
	void SetCplFilesList(QVector< QSharedPointer<AssetCpl> > rCplAssets);
	QList<QUuid> GetSelectedCpls() const;
	bool selectedOneOfEach() const;

	virtual bool isComplete() const override;
	virtual void initializePage() override;
	virtual void cleanupPage() override;
	virtual void mpTableViewScmCPL_ItemClicked();
	bool handleBackButton();

protected:
	bool validatePage() override;

signals:
	void CplListChanged();

private:
	QTableView * mpTableViewScmCPL;
	QStandardItemModel *mpModelScmCPL;
	QVector< QSharedPointer<AssetCpl> > mCplAssets;
	QWidget* mpParent;
	QLineEdit *mpLineEditAnnotation;
	QString mAnnotationText;
private slots:
	void CplChecked(QStandardItem *);
};


/*
 * ResultPage
 */
class ResultPage : public QWizardPage
{
	Q_OBJECT
		
public:
	ResultPage(QWidget *parent);
	
public slots:
	virtual void initializePage();
	void ShowResult(const QList<QStringList> rResult);
	bool handleBackButton();

signals:

private:
	QWidget* mpParent;
	QLineEdit* mpLineEditDeliverable;
	QLineEdit* mpLineEditDeliverableId;
	QTreeWidget* mpTree;
	JobQueue *mpJobQueue;
	QProgressDialog *mpProgressDialog;
	QMessageBox *mpMsgBox;

private slots:
	void rJobQueueFinished();
};

