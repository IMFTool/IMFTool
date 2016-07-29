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

#define ASSET_ID_DYNAMIK_PROPERTY "AssetId"

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
		TTMLMode
	};
	WizardResourceGenerator(QWidget *pParent = NULL);
	virtual ~WizardResourceGenerator() {}
	virtual QSize sizeHint() const;
	void SwitchMode(eMode mode);

private:
	Q_DISABLE_COPY(WizardResourceGenerator);
	void	InitLayout();
	int		mPageId;
};


class WizardResourceGeneratorPage : public QWizardPage {

	Q_OBJECT
		Q_PROPERTY(QStringList FilesSelected READ GetFilesList WRITE SetSourceFiles NOTIFY FilesListChanged)
		Q_PROPERTY(SoundfieldGroup SoundfieldGroupSelected READ GetSoundfieldGroup WRITE SetSoundfieldGroup NOTIFY SoundfieldGroupChanged)
		Q_PROPERTY(EditRate EditRateSelected READ GetEditRate WRITE SetEditRate NOTIFY EditRateChanged)
		Q_PROPERTY(Duration DurationSelected READ GetDuration WRITE SetDuration NOTIFY DurationChanged)

public:
	WizardResourceGeneratorPage(QWidget *pParent = NULL);
	virtual ~WizardResourceGeneratorPage() {}
	void SwitchMode(WizardResourceGenerator::eMode mode);
	QStringList GetFilesList() const;
	SoundfieldGroup GetSoundfieldGroup() const;
	EditRate GetEditRate() const;
	Duration GetDuration() const;

protected:

signals:
	void FilesListChanged();
	void SoundfieldGroupChanged();
	void EditRateChanged();
	void DurationChanged();

	public slots:
	void SetSourceFiles(const QStringList &rFiles);
	void SetSoundfieldGroup(const SoundfieldGroup &rSoundfieldGroup);
	void SetEditRate(const EditRate &rEditRate);
	void SetDuration(const Duration &Duration);
	void ChangeSoundfieldGroup(const QString &rName);
	void ShowFileDialog();
	void ShowDirDialog();
	virtual bool isComplete() const;
	void textChanged();
	void GenerateEmptyTimedText();
	private slots:
	void hideGroupBox();

private:
	enum eStackedLayoutIndex {
		ExrIndex = 0,
		WavIndex,
		TTMLIndex
	};
	Q_DISABLE_COPY(WizardResourceGeneratorPage);
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

private:
	Q_DISABLE_COPY(TimedTextModel);
	QStringList	mSelectedFile;
	MetadataExtractor *mpAs02Wrapper;
};
		/* -----Denis Manthey----- */



Q_DECLARE_METATYPE(WizardResourceGenerator::eMode)
