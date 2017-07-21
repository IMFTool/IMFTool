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
#include "GraphicsCommon.h"
#include "ImfPackage.h"
#include <QFrame>
#include <QSplitter>
#include <QDateTime>
#include <QVector> // (k)
#include "TTMLParser.h" // (k)

class AbstractWidgetTrackDetails;
class GraphicsViewScaleable;
class GraphicsSceneComposition;
class GraphicsSceneTimeline;
class QScrollArea;
class QSplitter;
class ImprovedSplitter;
class GraphicsWidgetComposition;
class GraphicsWidgetTimeline;
class QUndoStack;
class QToolBar;
class QAction;
class QButtonGroup;
class WidgetVideoPreview; // (k)

class WidgetComposition : public QFrame {

	Q_OBJECT

	friend class AddTrackDetailsCommand;
	friend class RemoveTrackDetailsCommand;
	friend class AddContentVersionCommand;
	friend class RemoveContentVersionCommand;

private:
	enum eButtons {
		ButtonAddTrack = 0,
	};
	xml_schema::NamespaceInfomap cpl_namespace;

public:
	WidgetComposition(const QSharedPointer<ImfPackage> &rImp, const QUuid &rCplAssetId, QWidget *pParent = NULL);
	virtual ~WidgetComposition();

	QUuid GetId() const { return ImfXmlHelper::Convert(mData.getId()); }
	UserText GetContentTitle() const { return ImfXmlHelper::Convert(mData.getContentTitle()); }
	UserText GetIssuer() const { return (mData.getIssuer().present() ? ImfXmlHelper::Convert(mData.getIssuer().get()) : UserText()); }
	UserText GetContentOriginator() const { return (mData.getContentOriginator().present() ? ImfXmlHelper::Convert(mData.getContentOriginator().get()) : UserText()); }
	//UserText GetContentVersionTag() const { return (mData.getContentVersionList().present() ? ImfXmlHelper::Convert(mData.getContentVersionList().get()) : UserText()); }
	ContentVersionList GetContentVersionList() { return (mData.getContentVersionList().present() ? ImfXmlHelper::Convert(mData.getContentVersionList().get()) : ContentVersionList()); }
	LocaleList GetLocaleList() { return (mData.getLocaleList().present() ? ImfXmlHelper::Convert(mData.getLocaleList().get()) : LocaleList()); }
	UserText GetAnnotation() const { return (mData.getAnnotation().present() ? ImfXmlHelper::Convert(mData.getAnnotation().get()) : UserText()); }
	//WR
	UserText GetContentKind() const { return (mData.getContentKind().present() ? ImfXmlHelper::Convert(mData.getContentKind().get()) : UserText()); }
	//WR
	QDateTime GetIssuerDate() const { return ImfXmlHelper::Convert(mData.getIssueDate()); }
	EditRate GetEditRate() const { return ImfXmlHelper::Convert(mData.getEditRate()); }
	UserText GetApplicationIdentification() const;

	void SetContentTitle(const UserText &rTitle) { mData.setContentTitle(ImfXmlHelper::Convert(rTitle)); }
	void SetIssuer(const UserText &rIssuer) { mData.setIssuer(ImfXmlHelper::Convert(rIssuer)); }
	void SetContentOriginator(const UserText &rOriginator) { mData.setContentOriginator(ImfXmlHelper::Convert(rOriginator)); }
	//void SetContentVersionTag(const UserText &rVersion) { mData.setContentVersionList(ImfXmlHelper::Convert(rVersion)); }
	void SetContentVersionList(const ContentVersionList &rContentVersionList) { if (rContentVersionList.count() == 0)  mData.getContentVersionList().detach(); else mData.setContentVersionList(ImfXmlHelper::Convert(rContentVersionList)); }
	void SetLocaleList(const LocaleList &rLocaleList) { if (rLocaleList.count() == 0)  mData.getLocaleList().detach(); else mData.setLocaleList(ImfXmlHelper::Convert(rLocaleList)); }
	void SetAnnotation(const UserText &rAnnotation) { mData.setAnnotation(ImfXmlHelper::Convert(rAnnotation)); }
	void SetID(QUuid rID) { mData.setId(ImfXmlHelper::Convert(rID)); }
	void SetApplicationIdentification(const UserText &rApplicationIdentification);
	const cpl2016::CompositionPlaylistType::ContentKindOptional contentKind;
	void SetContentKind(const UserText &rContentKind) { rContentKind.IsEmpty() ? mData.setContentKind(contentKind) : mData.setContentKind(ImfXmlHelper::Convert(rContentKind)); }

	QUuid GetCplAssetId() const { return(mAssetCpl ? mAssetCpl->GetId() : QUuid()); }
	QUndoStack* GetUndoStack() const { return mpUndoStack; }
	//! Reads the cpl xml file.
	ImfError Read();
	//! Writes the cpl to rDestination file. If rDestination is empty the cpl referenced by rCplAssetId will be overwritten.
	ImfError Write(const QString &rDestination = QString());
	//! Writes the cpl to rDestination file and creates new IDs for segments and sequences. This method is used to copy the data of an existing CPL into a new one.
	ImfError WriteNew(const QString &rDestination = QString());
	//! Disables wheel scrolling in detail track widget.
	bool eventFilter(QObject *pObj, QEvent *pEvt);
	bool DoesTrackExist(const QUuid &rId) const;
	bool DoesTrackExist(eSequenceType type) const;
	int GetTrackCount(eSequenceType type) const;

	QVector<VideoResource> *playlist; // (k)
	QVector<TTMLtimelineResource> *ttmls; // (k)
	int ImageSequenceIndex; // (k)
	int SubtitlesSequenceIndex; // (k)
	Timecode lastPosition; // (k)
	void getVerticalIndicator() { 
		if (lastPosition.IsValid()) {
			rCurrentFrameChanged(lastPosition);
		}
	}; // (k)

	GraphicsWidgetComposition* GetComposition() { return mpCompositionScene->GetComposition(); } // (k)

	//! Writes a minimalistic CPL
	static XmlSerializationError WriteMinimal(const QString &rDestination, const QUuid &rId, const EditRate &rEditRate, const UserText &rContentTitle, const UserText &rIssuer = UserText(), const UserText &rContentOriginator = UserText());

signals:
	void FrameInicatorActive(bool active);
	void CurrentAudioChanged(const QSharedPointer<AssetMxfTrack> &rAsset, const Duration &rOffset, const Timecode &rTimecode);
	void CurrentVideoChanged(const QSharedPointer<AssetMxfTrack> &rAsset, const qint64 &rOffset, const Timecode &rTimecode, const int &playlist_index);
	void PlaylistFinished();

	public slots:
	void AddNewSegmentRequest(int segmentIndex);
	void DeleteSegmentRequest(int segmentIndex);
	void DeleteSegmentRequest(const QUuid &rId);
	void MoveSegmentRequest(const QUuid &rId, const QUuid &rTargetSegmentId);
	void MoveSegmentRequest(const QUuid &rId, int targetIndex);
	void AddNewTrackRequest(eSequenceType type);
	void DeleteTrackRequest(int trackIndex);
	void DeleteTrackRequest(const QUuid &rId);
	void setVerticalIndicator(qint64); // (k)

	private slots:
	void rCurrentFrameChanged(const Timecode &rCplTimecode);
	//! Sets the sceneRect to the boundingRect of the root QGraphicsItem. This is done automatically. You should not need to call this function.
	void FitCompositionSceneRect();
	//! USED FOR COMMANDS
	void rPushCommand(QUndoCommand *pCommand);
	//! Splitter synchronization
	void rSplitterMoved(int pos, int index);
	void rAddTrackMenuAboutToShow();
	void rAddTrackMenuActionTriggered(QAction *pAction);
	void rToolBarActionTriggered(QAction *pAction);

private:
	Q_DISABLE_COPY(WidgetComposition);
	void InitLayout();
	void InitToolbar();
	void InitStyle();
	ImfError ParseCpl();

	//! Takes ownership.
	void AddTrackDetail(AbstractWidgetTrackDetails* pTrack, int TrackIndex);
	void MoveTrackDetail(AbstractWidgetTrackDetails* pTrack, int NewTrackIndex);
	//! Doesn't delete pTrack. Ownership of the item is transferred to the caller.
	void RemoveTrackDetail(AbstractWidgetTrackDetails *pTrack);
	AbstractWidgetTrackDetails* GetTrackDetail(int trackIndex) const;
	AbstractWidgetTrackDetails* GetTrackDetail(const QUuid &rId) const;
	int GetTrackDetailCount() const;
	int GetTrackDetailIndex(const AbstractWidgetTrackDetails *pTrackDetails) const;
	int GetLastTrackDetailIndexForType(eSequenceType type) const;

	GraphicsViewScaleable *mpCompositionView;
	GraphicsSceneComposition *mpCompositionScene;
	GraphicsViewScaleable *mpTimelineView;
	GraphicsSceneTimeline *mpTimelineScene;
	QScrollArea *mpCompositionTracksWidget;
	QSplitter *mpLeftInnerSplitter;
	QSplitter *mpRightInnerSplitter;
	QSplitter *mpOuterSplitter;
	ImprovedSplitter *mpTrackSplitter;
	GraphicsWidgetComposition *mpCompositionGraphicsWidget;
	GraphicsWidgetTimeline *mpTimelineGraphicsWidget;
	QUndoStack *mpUndoStack;
	QToolBar *mpToolBar;
	QSharedPointer<AssetCpl> mAssetCpl;
	QSharedPointer<ImfPackage> mImp;
	cpl2016::CompositionPlaylistType mData;
	// QActions
	QAction *mpAddMarkerTrackAction;
	QAction *mpAddAncillaryDataTrackAction;
	QAction *mpAddCommentaryTrackAction;
	QAction *mpAddHearingImpairedCaptionsTrackAction;
	QAction *mpAddKaraokeTrackAction;
	QAction *mpAddMainAudioTrackAction;
	QAction *mpAddSubtitlesTrackAction;
	QAction *mpAddVisuallyImpairedTextTrackAction;
	QAction *mpAddMainImageTrackAction;

};


class ImprovedSplitter : public QSplitter {

	Q_OBJECT

public:
	ImprovedSplitter(QWidget *pParent = NULL);
	ImprovedSplitter(Qt::Orientation orientation, QWidget *pPparent = NULL);
	virtual ~ImprovedSplitter() {}
	void addWidget(QWidget *pWidget);
	void insertWidget(int index, QWidget *pWidget);
	int count();

signals:
	void SizeChanged();

	private slots:
	void rSplitterMoved(int pos, int index);

protected:
	virtual bool eventFilter(QObject *pObject, QEvent *pEvent);
	virtual void childEvent(QChildEvent *pEvent);

private:
	Q_DISABLE_COPY(ImprovedSplitter);

	QWidget			*mpDummyWidget;
	QList<int>	mOldSplitterSizes;
};
