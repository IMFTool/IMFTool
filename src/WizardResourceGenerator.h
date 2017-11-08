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
#include "MetadataExtractorCommon.h"
#include <QWizard>
#include <QAbstractTableModel>
#include <QStringList>
#include <QPersistentModelIndex>
#include <QMap>
#include <QFileInfo>
#include <QLineEdit>
#include <QGroupBox>
#include "EmptyTimedTextGenerator.h"
#include "ImfPackage.h"


#define ASSET_ID_DYNAMIK_PROPERTY "AssetId"
#define ASSET_ESSENCE_TYPE "EssenceType"

class QDialogButtonBox;
class QFileDialog;
class QStringListModel;
class QTableView;
class SoundFieldGroupModel;
class TimedTextModel;
class QLabel;
class WidgetProxyImage;
class QtWaitingSpinner;
class QStackedLayout;
class QComboBox;
class QMessageBox;
class MetadataExtractor;

class WizardResourceGenerator : public QWizard {

	Q_OBJECT

public:
	enum eMode {
		ExrMode,
		WavMode,
		TTMLMode,
		Jpeg2000Mode,
	};
	WizardResourceGenerator(QWidget *pParent = NULL, QVector<EditRate> rEditRates = QVector<EditRate>(), QSharedPointer<AssetMxfTrack> rAsset = QSharedPointer<AssetMxfTrack>());
	virtual ~WizardResourceGenerator() {}
	virtual QSize sizeHint() const;
	void SwitchMode(eMode mode);

private:
	Q_DISABLE_COPY(WizardResourceGenerator);
	void	InitLayout();
	int		mPageId;
	QVector<EditRate> mEditRates;
	QSharedPointer<AssetMxfTrack> mAsset;
	bool mReadOnly;
};


class WizardResourceGeneratorPage : public QWizardPage {

	Q_OBJECT
		Q_PROPERTY(QStringList FilesSelected READ GetFilesList WRITE SetSourceFiles NOTIFY FilesListChanged)
		Q_PROPERTY(SoundfieldGroup SoundfieldGroupSelected READ GetSoundfieldGroup WRITE SetSoundfieldGroup NOTIFY SoundfieldGroupChanged)
		Q_PROPERTY(EditRate EditRateSelected READ GetEditRate WRITE SetEditRate NOTIFY EditRateChanged)
		Q_PROPERTY(Duration DurationSelected READ GetDuration WRITE SetDuration NOTIFY DurationChanged)
		//WR LanguageTagSelected
		Q_PROPERTY(QString LanguageTagWavSelected READ GetLanguageTagWav WRITE SetLanguageTagWav NOTIFY LanguageTagWavChanged)
		Q_PROPERTY(QString LanguageTagTTSelected READ GetLanguageTagTT WRITE SetLanguageTagTT NOTIFY LanguageTagTTChanged)
		Q_PROPERTY(QString MCATitleSelected READ GetMCATitle WRITE SetMCATitle NOTIFY MCATitleChanged)
		Q_PROPERTY(QString MCATitleVersionSelected READ GetMCATitleVersion WRITE SetMCATitleVersion NOTIFY MCATitleVersionChanged)
		Q_PROPERTY(QString MCAAudioContentKindSelected READ GetMCAAudioContentKind WRITE SetMCAAudioContentKind NOTIFY MCAAudioContentKindChanged)
		Q_PROPERTY(QString MCAAudioElementKindSelected READ GetMCAAudioElementKind WRITE SetMCAAudioElementKind NOTIFY MCAAudioElementKindChanged)
		Q_PROPERTY(EditRate CplEditRateSelected READ GetCplEditRate WRITE SetCplEditRate NOTIFY CplEditRateChanged)
		//WR

public:
	WizardResourceGeneratorPage(QWidget *pParent = NULL, 	QVector<EditRate> rEditRates = QVector<EditRate>(), bool rReadOnly = false, QSharedPointer<AssetMxfTrack> rAsset = QSharedPointer<AssetMxfTrack>());
	virtual ~WizardResourceGeneratorPage() {}
	void SwitchMode(WizardResourceGenerator::eMode mode);
	QStringList GetFilesList() const;
	SoundfieldGroup GetSoundfieldGroup() const;
	EditRate GetEditRate() const;
	Duration GetDuration() const;
	//WR
	QString GetLanguageTagWav() const;
	QString GetLanguageTagTT() const;
	QString GetMCATitle() const;
	QString GetMCATitleVersion() const;
	QString GetMCAAudioContentKind() const;
	QString GetMCAAudioElementKind() const;
	EditRate GetCplEditRate() const;
	//WR

protected:

signals:
	void FilesListChanged();
	void SoundfieldGroupChanged();
	void EditRateChanged();
	void DurationChanged();
	//WR
	void LanguageTagWavChanged();
	void LanguageTagTTChanged();
	void MCATitleChanged();
	void MCATitleVersionChanged();
	void MCAAudioContentKindChanged();
	void MCAAudioElementKindChanged();
	void CplEditRateChanged();

	//WR

	public slots:
	void SetSourceFiles(const QStringList &rFiles);
	void SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup);
	void SetEditRate(const EditRate &rEditRate);
	void SetDuration(const Duration &Duration);
	//WR
	void SetLanguageTagWav(const QString &rLanguageTag);
	void SetLanguageTagTT(const QString &rLanguageTag);
	void SetMCATitle(const QString &rLanguageTag);
	void SetMCATitleVersion(const QString &rLanguageTag);
	void SetMCAAudioContentKind(const QString &rLanguageTag);
	void SetMCAAudioElementKind(const QString &rLanguageTag);
	void SetCplEditRate(const EditRate &rEditRate);
	//WR
	void ChangeSoundfieldGroup(const QString &rName);
	void ShowFileDialog();
	void ShowDirDialog();
	virtual bool isComplete() const;
	void textChanged();
	void GenerateEmptyTimedText();
	private slots:
	void hideGroupBox();
	//WR
	void languageTagWavChanged();
	void languageTagTTChanged();
	//WR

private:
	enum eStackedLayoutIndex {
		ExrIndex = 0,
		WavIndex,
		TTMLIndex,
		Jpeg2000Index,
	};
	Q_DISABLE_COPY(WizardResourceGeneratorPage);
	QVector<EditRate> mEditRates;
	void InitLayout();

	QFileDialog	*mpFileDialog;
	SoundFieldGroupModel *mpSoundFieldGroupModel;
	TimedTextModel *mpTimedTextModel;									// Denis Manthey
	QTableView *mpTableViewExr;
	QTableView *mpTableViewWav;
	QTableView *mpTableViewTimedText;							// Denis Manthey
	WidgetProxyImage *mpProxyImageWidget;
	QStackedLayout *mpStackedLayout;
	QComboBox	*mpComboBoxEditRate;
	QComboBox *mpComboBoxSoundfieldGroup;
	//WR
	QComboBox *mpComboBoxCplEditRate;
	QLineEdit *mpLineEditLanguageTagWav;
	QLineEdit *mpLineEditLanguageTagTT;
	QLineEdit *mpLineEditMCATitle;
	QLineEdit *mpLineEditMCATitleVersion;
	QLineEdit *mpLineEditMCAAudioContentKind;
	QLineEdit *mpLineEditMCAAudioElementKind;
	//WR
	QFileDialog *mpDirDialog;
	QLineEdit *mpLineEditDuration;
	QLineEdit *mpLineEditFileDir;
	QLineEdit *mpLineEditFileName;
	QPushButton *mpGenerateEmpty_button;
	QMessageBox	*mpMsgBox;
	MetadataExtractor *mpAs02Wrapper;
	EmptyTimedTextGenerator *mpEmptyTt;
	QGroupBox *mpGroupBox;
	bool mGroupBoxCheck;

	QSharedPointer<AssetMxfTrack> mAsset;
	bool mReadOnly;
};


class WidgetProxyImage : public QWidget {

	Q_OBJECT

public:
	WidgetProxyImage(QWidget *pParent = NULL);
	virtual ~WidgetProxyImage() {}
	//! Returns the model index this proxy image belongs to.
	const QPersistentModelIndex& GetModelIndex() const { return mIndex; }
	void SetModelIndex(const QPersistentModelIndex &rIndex) { mIndex = rIndex; }

	public slots:
	void hide();

	private slots:
	void rTimeout();
	void rTransformationFinished(const QImage &rImage, const QVariant &rIdentifier);

private:
	Q_DISABLE_COPY(WidgetProxyImage);
	void InitLayout();
	void setVisible(bool visible) { QWidget::setVisible(visible); }
	void show() { QWidget::show(); }

	QPersistentModelIndex mIndex;
	QtWaitingSpinner *mpSpinner;
	QTimer *mpTimer;
	QLabel *mpImageLabel;
};




class SoundFieldGroupModel : public QAbstractTableModel {

	Q_OBJECT

public:
	enum eModelColumn {
		ColumnIcon = 0,
		ColumnSourceFile,
		ColumnSrcChannel,
		ColumnDstChannel,
		ColumnMax
	};
	SoundFieldGroupModel(QObject *pParent = NULL);
	virtual ~SoundFieldGroupModel() {}
	void SetFilesList(const QStringList &rSourceFile);
	QStringList GetSourceFiles() const;
	void SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup);
	SoundfieldGroup GetSoundfieldGroup() const { return mSoundfieldGroup; }
	virtual Qt::ItemFlags flags(const QModelIndex &rIndex) const;
	virtual int rowCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &rIndex, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &rIndex, const QVariant &rValue, int role = Qt::EditRole);
	void ChangeSoundfieldGroup(const QString &rName);
	//WR
	//void SetLanguageTagWav(const QString &rLanguageTag);
	//WR

private:
	Q_DISABLE_COPY(SoundFieldGroupModel);

	QList<QPair<QString, unsigned int> > mSourceFilesChannels;
	SoundfieldGroup	mSoundfieldGroup;
	MetadataExtractor *mpAs02Wrapper;
};




		/* -----Denis Manthey----- */

class TimedTextModel : public QAbstractTableModel {

	Q_OBJECT

public:
	enum eModelColumn {
		ColumnIcon = 0,
		ColumnFilePath,
		ColumnMax
	};
	TimedTextModel(QObject *pParent = NULL);
	virtual ~TimedTextModel() {}
	void SetFile(const QStringList &rSourceFile);
	QStringList GetSourceFile() const { return mSelectedFile; }
	virtual int rowCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &rParent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &rIndex, int role = Qt::DisplayRole) const;
	//WR
	void SetLanguageTagTT(const QString &rLanguageTag);
	void SetMCATitle(const QString &rString);
	void SetMCATitleVersion(const QString &rString);
	void SetMCAAudioContentKind(const QString &rString);
	void SetMCAAudioElementKind(const QString &rString);
	void SetCplEditRate(const EditRate &rEditRate);
	//WR

private:
	Q_DISABLE_COPY(TimedTextModel);
	QStringList	mSelectedFile;
	MetadataExtractor *mpAs02Wrapper;
};
		/* -----Denis Manthey----- */



Q_DECLARE_METATYPE(WizardResourceGenerator::eMode)
