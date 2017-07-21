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
#include "WidgetComposition.h"
#include "WidgetTrackDedails.h"
#include "ImfPackage.h"
#include "GraphicScenes.h"
#include "GraphicsViewScaleable.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetTimeline.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetResources.h"
#include "WidgetVideoPreview.h" // (k)
#include "CompositionPlaylistCommands.h"

#include <QMessageBox>
#include <QToolBar>
#include <QBoxLayout>
#include <QScrollBar>
#include <QToolButton>
#include <QButtonGroup>
#include <QMenu>
#include <fstream>
#include <QPropertyAnimation>
#include <QTemporaryFile>



WidgetComposition::WidgetComposition(const QSharedPointer<ImfPackage> &rImp, const QUuid &rCplAssetId, QWidget *pParent /*= NULL*/) :
QFrame(pParent), mpCompositionView(NULL), mpCompositionScene(NULL), mpTimelineView(NULL), mpTimelineScene(NULL), mpCompositionTracksWidget(NULL),
mpLeftInnerSplitter(NULL), mpRightInnerSplitter(NULL), mpOuterSplitter(NULL), mpTrackSplitter(NULL), mpCompositionGraphicsWidget(NULL),
mpTimelineGraphicsWidget(NULL), mpUndoStack(NULL), mpToolBar(NULL),
mAssetCpl(rImp->GetAsset(rCplAssetId).objectCast<AssetCpl>()), mImp(rImp),
mData(ImfXmlHelper::Convert(QUuid::createUuid()), ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()), ImfXmlHelper::Convert(UserText(tr("Unnamed"))), ImfXmlHelper::Convert(EditRate::EditRate24), cpl2016::CompositionPlaylistType::SegmentListType())
{
	cpl_namespace[""].name = XML_NAMESPACE_CPL;
	cpl_namespace["dcml"].name = XML_NAMESPACE_DCML;
	cpl_namespace["cc"].name = XML_NAMESPACE_CC;
	cpl_namespace["ds"].name = XML_NAMESPACE_DS;
	cpl_namespace["xs"].name = XML_NAMESPACE_XS;

	mpUndoStack = new QUndoStack(this);
	InitLayout();
	InitToolbar();
	InitStyle();
}

WidgetComposition::~WidgetComposition() {

}

void WidgetComposition::InitLayout() {

	mpToolBar = new QToolBar(tr("Composition Toolbar"), this);
	mpToolBar->setIconSize(QSize(20, 20));
	mpToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	mpOuterSplitter = new QSplitter(Qt::Horizontal, this);
	mpOuterSplitter->setHandleWidth(0);
	mpOuterSplitter->setChildrenCollapsible(false);
	mpLeftInnerSplitter = new QSplitter(Qt::Vertical, this);
	mpLeftInnerSplitter->setHandleWidth(2);
	mpLeftInnerSplitter->setChildrenCollapsible(false);
	mpRightInnerSplitter = new QSplitter(Qt::Vertical, this);
	mpRightInnerSplitter->setHandleWidth(2);
	mpRightInnerSplitter->setChildrenCollapsible(false);
	mpTimelineScene = new GraphicsSceneTimeline(GetEditRate(), this);
	mpTimelineView = new GraphicsViewScaleable(this);
	mpTimelineView->setScene(mpTimelineScene);
	mpTimelineView->setFrameShape(QFrame::NoFrame);
	mpTimelineView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mpTimelineView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mpCompositionScene = new GraphicsSceneComposition(GetEditRate(), this);
	mpCompositionView = new GraphicsViewScaleable(this);
	mpCompositionView->setScene(mpCompositionScene);
	mpCompositionView->setFrameShape(QFrame::NoFrame);
	mpCompositionView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	mpCompositionView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mpCompositionTracksWidget = new QScrollArea(this);
	mpCompositionTracksWidget->setWidgetResizable(true);
	mpCompositionTracksWidget->setFrameShape(QFrame::NoFrame);
	mpCompositionTracksWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	mpCompositionTracksWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	WidgetTrackDetailsTimeline *p_timeline_detail = new WidgetTrackDetailsTimeline(this);
	mpLeftInnerSplitter->addWidget(p_timeline_detail);
	mpLeftInnerSplitter->addWidget(mpCompositionTracksWidget);
	mpRightInnerSplitter->addWidget(mpTimelineView);
	mpRightInnerSplitter->addWidget(mpCompositionView);
	mpOuterSplitter->addWidget(mpLeftInnerSplitter);
	mpOuterSplitter->addWidget(mpRightInnerSplitter);
	//Don't stretch the WidgetTrackDetailsTimeline widget
	mpOuterSplitter->setStretchFactor(0,0);
	// TimelineView may be stretched
	mpOuterSplitter->setStretchFactor(1,1);
	mpTrackSplitter = new ImprovedSplitter(Qt::Vertical); // mpCompositionTracksWidget gets parent with setWidget()
	mpTrackSplitter->setHandleWidth(0); // don't change
	mpTrackSplitter->setChildrenCollapsible(false);
	mpCompositionTracksWidget->setWidget(mpTrackSplitter); // don't invoke setWidget() befor all widgets are added
	QVBoxLayout *p_layout = new QVBoxLayout();
	p_layout->setMargin(0);
	p_layout->addWidget(mpToolBar);
	p_layout->addWidget(mpOuterSplitter);
	setLayout(p_layout);

	mpCompositionGraphicsWidget = mpCompositionScene->GetComposition(); // Faster access.

	mpTimelineGraphicsWidget = mpTimelineScene->GetTimeline(); // Faster access.

	mpCompositionView->viewport()->installEventFilter(this); // Synchronize view scaling.
	mpTimelineView->viewport()->installEventFilter(this); // Synchronize view scaling.
	mpTimelineView->installEventFilter(this);
	mpCompositionTracksWidget->viewport()->installEventFilter(this); // Disable wheel scrolling.
	lastPosition = Timecode(GetEditRate(), 0); // (k) initialize at beginning

	QList<int> splitter_sizes;
	splitter_sizes << mpTimelineView->minimumSizeHint().height() << -1;
	mpTimelineView->setMinimumSize(p_timeline_detail->minimumSizeHint()); // Overwrite minimumSizeHint. The parent (QAbstractScrollArea) returns a minimumSizeHint of (76, 76) even if the scroll bars are hidden.
	mpRightInnerSplitter->setStretchFactor(1, 1);
	mpLeftInnerSplitter->setStretchFactor(1, 1);
	mpRightInnerSplitter->setSizes(splitter_sizes);
	mpLeftInnerSplitter->setSizes(splitter_sizes);

	connect(mpToolBar, SIGNAL(actionTriggered(QAction*)), this, SLOT(rToolBarActionTriggered(QAction*)));
	connect(mpCompositionView->verticalScrollBar(), SIGNAL(valueChanged(int)), mpCompositionTracksWidget->verticalScrollBar(), SLOT(setValue(int)));
	connect(mpCompositionView->horizontalScrollBar(), SIGNAL(valueChanged(int)), mpTimelineView->horizontalScrollBar(), SLOT(setValue(int)));
	connect(mpLeftInnerSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(rSplitterMoved(int, int)));
	connect(mpRightInnerSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(rSplitterMoved(int, int)));
	connect(mpCompositionScene, SIGNAL(PushCommand(QUndoCommand*)), this, SLOT(rPushCommand(QUndoCommand*)));
	connect(mpTimelineScene->GetCurrentFrameIndicator(), SIGNAL(XPosChanged(qreal)), mpCompositionScene->GetCurrentFrameIndicator(), SLOT(SetXPos(qreal)));
	connect(p_timeline_detail, SIGNAL(HeightAboutToChange(qint32)), mpTimelineGraphicsWidget, SLOT(SetHeight(qint32)));
	connect(mpTimelineScene, SIGNAL(FrameInicatorActive(bool)), this, SIGNAL(FrameInicatorActive(bool)));
	connect(mpTimelineScene, SIGNAL(CurrentFrameChanged(const Timecode&)), p_timeline_detail, SLOT(SetTimecode(const Timecode&)));
	connect(mpTimelineScene, SIGNAL(CurrentFrameChanged(const Timecode&)), this, SLOT(rCurrentFrameChanged(const Timecode&)));
	connect(mpTimelineScene, SIGNAL(MoveSegmentRequest(const QUuid&, const QUuid&)), this, SLOT(MoveSegmentRequest(const QUuid&, const QUuid&)));
	connect(mpCompositionScene, SIGNAL(ClearSelectionRequest()), mpTimelineScene, SLOT(clearSelection()));
	connect(mpTimelineGraphicsWidget, SIGNAL(NewSegmentRequest(int)), this, SLOT(AddNewSegmentRequest(int)));
	connect(mpTimelineGraphicsWidget, SIGNAL(DeleteSegmentRequest(const QUuid&)), this, SLOT(DeleteSegmentRequest(const QUuid&)));
}

// (k) - start
// let's the player update the current-frame-indicator shown in the timeline
void WidgetComposition::setVerticalIndicator(qint64 frameNr) {
	//qDebug() << "set player postion" << frameNr;
	mpTimelineScene->GetCurrentFrameIndicator()->SetXPos(frameNr);
}
// (k) - end

bool WidgetComposition::eventFilter(QObject *pObj, QEvent *pEvt) {

	if(pEvt->type() == QEvent::KeyPress || pEvt->type() == QEvent::KeyRelease) {
		QKeyEvent *p_key_event = static_cast<QKeyEvent*>(pEvt);
		if(p_key_event->key() == Qt::Key_Left || p_key_event->key() == Qt::Key_Right || p_key_event->key() == Qt::Key_Up || p_key_event->key() == Qt::Key_Down) {
			if(pObj == mpTimelineView) return true;
		}
	}
	// Disable scrolling in track detail view.
	if(pEvt->type() == QEvent::Wheel) {
		if(pObj == mpCompositionTracksWidget->viewport()) return true;
		else if(pObj == mpTimelineView->viewport()) return true;
		else if(pObj == mpCompositionView->viewport()) {
			QWheelEvent *p_wheel_event = static_cast<QWheelEvent*>(pEvt);
			qreal scale_factor = pow((double)2, p_wheel_event->delta() / 240.0);
			mpTimelineView->ScaleView(scale_factor);
			mpCompositionView->ScaleView(scale_factor);
			mpTimelineView->horizontalScrollBar()->setValue(mpCompositionView->horizontalScrollBar()->value());
			return true;
		}
	}
	return false;
}

void WidgetComposition::AddNewTrackRequest(eSequenceType type) {

	if((type == MainImageSequence && GetTrackCount(MainImageSequence) >= 1) || (type == MarkerSequence && GetTrackCount(MarkerSequence) >= 1)) return;
	QUuid track_id(QUuid::createUuid());
	QUndoCommand *p_root = new QUndoCommand(NULL);
	AbstractWidgetTrackDetails *p_track = NULL;
	if(type == MainAudioSequence) p_track = new WidgetAudioTrackDetails(track_id, mpCompositionTracksWidget);

	else if(type == SubtitlesSequence) p_track = new WidgetTrackDetails(track_id, type, mpCompositionTracksWidget);

	else p_track = new WidgetTrackDetails(track_id, type, mpCompositionTracksWidget);
	int track_index = GetLastTrackDetailIndexForType(type) + 1;
	if(track_index == 0) {
		if(type == MarkerSequence) track_index = 0;
		else if(type == MainImageSequence) {
			if(GetTrackCount(MarkerSequence) >= 1) track_index = 1;
			else track_index = 0;
		}
		else track_index = GetTrackDetailCount();
	}

	for(int i = 0; i < mpCompositionGraphicsWidget->GetSegmentCount(); i++) {
		GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(i);
		if(p_segment) {
			GraphicsWidgetSequence *p_sequence = new GraphicsWidgetSequence(p_segment, type, track_id);
			AddSequenceCommand *p_add_sequence_command = new AddSequenceCommand(p_sequence, track_index, p_segment, p_root);
			connect(p_track, SIGNAL(HeightAboutToChange(qint32)), p_sequence, SLOT(SetHeight(qint32)));
			if(type == MarkerSequence) {
				AbstractGraphicsWidgetResource *p_resource = new GraphicsWidgetMarkerResource(p_sequence);
				AddResourceCommand *p_add_resource_command = new AddResourceCommand(p_resource, p_sequence->GetResourceCount(), p_sequence, p_root);
			}
		}
	}
	AddTrackDetailsCommand *p_track_d_command = new AddTrackDetailsCommand(p_track, this, track_index, p_root);
	mpUndoStack->push(p_root);
}

void WidgetComposition::DeleteTrackRequest(int trackIndex) {

	AbstractWidgetTrackDetails *p_track = GetTrackDetail(trackIndex);
	if(p_track) DeleteTrackRequest(p_track->GetId());
}

void WidgetComposition::DeleteTrackRequest(const QUuid &rId) {

	AbstractWidgetTrackDetails *p_track = GetTrackDetail(rId);
	if(p_track) {
		QUndoCommand *p_root = new QUndoCommand(NULL);
		for(int i = 0; i < mpCompositionGraphicsWidget->GetSegmentCount(); i++) {
			GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(i);
			if(p_segment) {
				GraphicsWidgetSequence *p_sequence = p_segment->GetSequenceWithTrackId(rId);
				if(p_sequence) {
					RemoveSequenceCommand *p_remove_sequence_command = new RemoveSequenceCommand(p_sequence, p_segment, p_root);
				}
			}
		}
		RemoveTrackDetailsCommand *p_remove_track_d_command = new RemoveTrackDetailsCommand(p_track, this, p_root);
		mpUndoStack->push(p_root);
	}
}

bool WidgetComposition::DoesTrackExist(const QUuid &rId) const {

	for(int i = 0; i < mpTrackSplitter->count(); i++) {
		AbstractWidgetTrackDetails *p_track_detail = qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(i));
		if(p_track_detail && p_track_detail->GetId() == rId) return true;
	}
	return false;
}

bool WidgetComposition::DoesTrackExist(eSequenceType type) const {

	for(int i = 0; i < mpTrackSplitter->count(); i++) {
		AbstractWidgetTrackDetails *p_track_detail = qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(i));
		if(p_track_detail && p_track_detail->GetType() == type) return true;
	}
	return false;
}

ImfError WidgetComposition::Read() {

	ImfError error; // Reset last error.
	if(mAssetCpl) {
		if(mAssetCpl->Exists() == true) {
			qDebug() << "Read Cpl: " << mAssetCpl->GetPath().absoluteFilePath();
			// clean old cpl
			mpUndoStack->clear();
			for(int i = 0; i < GetTrackDetailCount(); i++) {
				AbstractWidgetTrackDetails *p_widget = GetTrackDetail(i);
				RemoveTrackDetail(p_widget);
				if(p_widget) p_widget->deleteLater();
			}
			for(int i = 0; i < mpCompositionGraphicsWidget->GetSegmentCount(); i++) {
				GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(i);
				mpCompositionGraphicsWidget->RemoveSegment(p_segment);
				p_segment->deleteLater();
			}
			for(int i = 0; i < mpTimelineGraphicsWidget->GetSegmentIndicatorCount(); i++) {
				GraphicsWidgetSegmentIndicator *p_segment_indicator = mpTimelineGraphicsWidget->GetSegmentIndicator(i);
				mpTimelineGraphicsWidget->RemoveSegmentIndicator(p_segment_indicator);
				p_segment_indicator->deleteLater();
			}
			error = ParseCpl();
			mpCompositionView->ensureVisible(0, 0, 1, 1);
			mpTimelineView->ensureVisible(0, 0, 1, 1);

			qDebug() << "Finished Read Cpl: " << mAssetCpl->GetPath().absoluteFilePath();
		}
		else error = ImfError(ImfError::AssetFileMissing, tr("CPL: %1").arg(mAssetCpl->GetPath().absoluteFilePath()));
	}
	else error = ImfError(ImfError::AssetFileMissing, tr("CPL: %1").arg(tr("Location unknown.")));
	return error;
}


ImfError WidgetComposition::Write(const QString &rDestination /*= QString()*/) {

	ImfError error; // Reset last error.
	// namespace maps
/*	xml_schema::NamespaceInfomap cpl_namespace;
	cpl_namespace[""].name = XML_NAMESPACE_CPL;
	cpl_namespace["dcml"].name = XML_NAMESPACE_DCML;
	cpl_namespace["cc"].name = XML_NAMESPACE_CC;
	cpl_namespace["ds"].name = XML_NAMESPACE_DS;
	cpl_namespace["xs"].name = XML_NAMESPACE_XS;*/

	cpl2016::CompositionPlaylistType cpl(mData);
	cpl.setCreator(ImfXmlHelper::Convert(UserText(CREATOR_STRING)));
	cpl.setIssueDate(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	cpl2016::CompositionPlaylistType_SegmentListType segment_list;
	cpl2016::CompositionPlaylistType_SegmentListType::SegmentSequence &segment_sequence = segment_list.getSegment();
	//WR begin
	//Essence descriptor variables
	cpl2016::CompositionPlaylistType_EssenceDescriptorListType essence_descriptor_list;
	cpl2016::CompositionPlaylistType_EssenceDescriptorListType::EssenceDescriptorSequence &essence_descriptor_sequence = essence_descriptor_list.getEssenceDescriptor();
	essence_descriptor_sequence.clear();
	//List of UUIDs of resources with essence descriptor
	QStringList resourceIDs;
	//WR end
	for(int i = 0; i < mpCompositionGraphicsWidget->GetSegmentCount(); i++) {
		GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(i);
		if(p_segment) {
			cpl2016::SegmentType_SequenceListType sequence_list;
			cpl2016::SegmentType_SequenceListType::AnySequence &r_any_sequence(sequence_list.getAny());
			xercesc::DOMDocument &doc = sequence_list.getDomDocument();
			for(int ii = 0; ii < p_segment->GetSequenceCount(); ii++) {
				GraphicsWidgetSequence *p_sequence = p_segment->GetSequence(ii);
				if(p_sequence) {
					cpl2016::SequenceType_ResourceListType resource_list;
					cpl2016::SequenceType_ResourceListType::ResourceSequence &resource_sequence = resource_list.getResource();
					for(int iii = 0; iii < p_sequence->GetResourceCount(); iii++) {
						AbstractGraphicsWidgetResource *p_resource = p_sequence->GetResource(iii);
						if(p_resource) {
							std::auto_ptr<cpl2016::BaseResourceType> resource = p_resource->Write();
							//WR begin
							// Test if resource is a track file resource
							cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*resource));
							//Get corresponding AssetMxfTrack object
							QSharedPointer<AssetMxfTrack> mxffile = p_resource->GetAsset().objectCast<AssetMxfTrack>();
							if (p_file_resource && mxffile){
								//Set SourceEncoding in CPL
								p_file_resource->setSourceEncoding(ImfXmlHelper::Convert(mxffile->GetSourceEncoding()));
								if (!resourceIDs.contains(mxffile->GetId().toString())){
									//If not yet added:
									resourceIDs.append(mxffile->GetId().toString());
									//Push Essence Descriptor into CPL
									essence_descriptor_sequence.push_back(*(mxffile->GetEssenceDescriptor()));
								}
							}
							//WR end
							resource_sequence.push_back(resource);
						}
					}
					if(p_sequence->GetType() == MarkerSequence) {
						cpl2016::SequenceType sequence(ImfXmlHelper::Convert(p_sequence->GetId()), ImfXmlHelper::Convert(p_sequence->GetTrackId()), resource_list);
						sequence_list.setMarkerSequence(sequence);
					}
					else {
						xercesc::DOMElement* p_dom_element = NULL;
						switch(p_sequence->GetType()) {
							case AncillaryDataSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:AncillaryDataSequence").c_str()));
								break;
							case CommentarySequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:CommentarySequence").c_str()));
								break;
							case HearingImpairedCaptionsSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:HearingImpairedCaptionsSequence").c_str()));
								break;
							case KaraokeSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:KaraokeSequence").c_str()));
								break;
							case MainAudioSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:MainAudioSequence").c_str()));
								break;
							case MainImageSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:MainImageSequence").c_str()));
								break;
							case SubtitlesSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:SubtitlesSequence").c_str()));
								break;
							case VisuallyImpairedTextSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:VisuallyImpairedTextSequence").c_str()));
								break;
							case Unknown:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(p_sequence->property("namespace").toString().toStdString()).c_str(), (xsd::cxx::xml::string(p_sequence->property("localName").toString().toStdString()).c_str()));
								break;
							default:
								qWarning() << "Default case";
								break;
						}
						cpl2016::SequenceType sequence(ImfXmlHelper::Convert(p_sequence->GetId()), ImfXmlHelper::Convert(p_sequence->GetTrackId()), resource_list);
						if(p_dom_element && p_sequence->GetType() != Unknown) {
							// Import namespaces from map for wildcard content.
							for(xml_schema::NamespaceInfomap::iterator iter = cpl_namespace.begin(); iter != cpl_namespace.end(); ++iter) {
								QString ns("xmlns");
								if(iter->first.empty() == false) {
									ns.append(":");
									ns.append(iter->first.c_str());
								}
								p_dom_element->setAttributeNS(xsd::cxx::xml::string(XML_NAMESPACE_NS).c_str(), xsd::cxx::xml::string(ns.toStdString()).c_str(), xsd::cxx::xml::string(iter->second.name).c_str());
							}
							*p_dom_element << sequence;
							r_any_sequence.push_back(p_dom_element);
						}
					}
				}
			}
			cpl2016::SegmentType segment(ImfXmlHelper::Convert(p_segment->GetId()), sequence_list);
			if(p_segment->GetAnnotationText().IsEmpty() == false) segment.setAnnotation(ImfXmlHelper::Convert(p_segment->GetAnnotationText()));
			segment_sequence.push_back(segment);
		}
	}
	cpl.setSegmentList(segment_list);
	//WR begin
	cpl.setEssenceDescriptorList(essence_descriptor_list);
	//WR end
	QString destination(rDestination);
	if(destination.isEmpty() && mAssetCpl) {
		destination = mAssetCpl->GetPath().absoluteFilePath();
	}
	if(destination.isEmpty() == false) {
		XmlSerializationError serialization_error;
		std::ofstream cpl_ofs(destination.toStdString().c_str(), std::ofstream::out);
		try {
			cpl2016::serializeCompositionPlaylist(cpl_ofs, cpl, cpl_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
		}
		catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
		catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
		cpl_ofs.close();
		if(serialization_error.IsError() == true) {
			qDebug() << serialization_error;
			error = ImfError(serialization_error);
		}
	}
	else {
		error = ImfError(ImfError::DestinationFileUnspecified, tr("Couldn't write cpl!"));
	}
	if(!error) {
		mpUndoStack->clear();
		if(mAssetCpl) mAssetCpl->FileModified();
		//WR begin
		if(mAssetCpl) mAssetCpl->SetIsNewOrModified(true);
		//WR end
		qDebug() << "Write " << destination.toStdString().c_str();
	}
	return error;
}

ImfError WidgetComposition::WriteNew(const QString &rDestination /*= QString()*/) {

	ImfError error; // Reset last error.
	// namespace maps
	/*xml_schema::NamespaceInfomap cpl_namespace;
	cpl_namespace[""].name = XML_NAMESPACE_CPL;
	cpl_namespace["dcml"].name = XML_NAMESPACE_DCML;
	cpl_namespace["cc"].name = XML_NAMESPACE_CC;
	cpl_namespace["ds"].name = XML_NAMESPACE_DS;
	cpl_namespace["xs"].name = XML_NAMESPACE_XS;*/

	cpl2016::CompositionPlaylistType cpl(mData);
	cpl.setCreator(ImfXmlHelper::Convert(UserText(CREATOR_STRING)));
	cpl.setIssueDate(ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()));
	cpl2016::CompositionPlaylistType_SegmentListType segment_list;
	cpl2016::CompositionPlaylistType_SegmentListType::SegmentSequence &segment_sequence = segment_list.getSegment();
	//WR begin
	cpl2016::CompositionPlaylistType_EssenceDescriptorListType essence_descriptor_list;
	cpl2016::CompositionPlaylistType_EssenceDescriptorListType::EssenceDescriptorSequence &essence_descriptor_sequence = essence_descriptor_list.getEssenceDescriptor();
	essence_descriptor_sequence.clear();
	QStringList resourceIDs;
	//WR end

	//create for every existing Track ID a new Track ID
	QStringList oldTrackIDs;
	QList<QUuid> newTrackIDs;
	for(int i = 0; i < mpCompositionGraphicsWidget->GetSegmentCount(); i++){
		GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(i);
		for(int ii = 0; ii < p_segment->GetSequenceCount(); ii++){
			GraphicsWidgetSequence *p_sequence = p_segment->GetSequence(ii);
			oldTrackIDs.append(p_sequence->GetTrackId().toString());
		}
	}
	oldTrackIDs.removeDuplicates();
	for (int i = 0; i < oldTrackIDs.size(); i++){
		newTrackIDs.append(QUuid::createUuid());
	}

	for(int i = 0; i < mpCompositionGraphicsWidget->GetSegmentCount(); i++) {
		GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(i);
		if(p_segment) {
			cpl2016::SegmentType_SequenceListType sequence_list;
			cpl2016::SegmentType_SequenceListType::AnySequence &r_any_sequence(sequence_list.getAny());
			xercesc::DOMDocument &doc = sequence_list.getDomDocument();
			for(int ii = 0; ii < p_segment->GetSequenceCount(); ii++) {
				GraphicsWidgetSequence *p_sequence = p_segment->GetSequence(ii);
				if(p_sequence) {
					cpl2016::SequenceType_ResourceListType resource_list;
					cpl2016::SequenceType_ResourceListType::ResourceSequence &resource_sequence = resource_list.getResource();
					for(int iii = 0; iii < p_sequence->GetResourceCount(); iii++) {
						AbstractGraphicsWidgetResource *p_resource = p_sequence->GetResource(iii);
						if(p_resource) {
							std::auto_ptr<cpl2016::BaseResourceType> resource = p_resource->Write();
							// Create a new UUID for all resources
							resource->setId(ImfXmlHelper::Convert(QUuid::createUuid()));
							//WR begin
							// Test if resource is a track file resource
							cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*resource));
							//Get corresponding AssetMxfTrack object
							QSharedPointer<AssetMxfTrack> mxffile = p_resource->GetAsset().objectCast<AssetMxfTrack>();
							if (p_file_resource && mxffile){
								//Set SourceEncoding in CPL
								p_file_resource->setSourceEncoding(ImfXmlHelper::Convert(mxffile->GetSourceEncoding()));
								if (!resourceIDs.contains(mxffile->GetId().toString())){
									//If not yet added:
									resourceIDs.append(mxffile->GetId().toString());
									//Push Essence Descriptor into CPL
									essence_descriptor_sequence.push_back(*(mxffile->GetEssenceDescriptor()));
								}
							}
							//WR end

							resource_sequence.push_back(resource);
						}
					}
					if(p_sequence->GetType() == MarkerSequence) {

						//Track ID: compare p_sequence->GetTrackId with QStringList oldTrackIDs and get the index i. insert QList newTrackIDs.at(i)!
						cpl2016::SequenceType sequence(ImfXmlHelper::Convert(QUuid::createUuid()), ImfXmlHelper::Convert(newTrackIDs.at(oldTrackIDs.indexOf(p_sequence->GetTrackId().toString()))), resource_list);
						sequence_list.setMarkerSequence(sequence);
					}
					else {
						xercesc::DOMElement* p_dom_element = NULL;
						switch(p_sequence->GetType()) {
							case AncillaryDataSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:AncillaryDataSequence").c_str()));
								break;
							case CommentarySequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:CommentarySequence").c_str()));
								break;
							case HearingImpairedCaptionsSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:HearingImpairedCaptionsSequence").c_str()));
								break;
							case KaraokeSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:KaraokeSequence").c_str()));
								break;
							case MainAudioSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:MainAudioSequence").c_str()));
								break;
							case MainImageSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:MainImageSequence").c_str()));
								break;
							case SubtitlesSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:SubtitlesSequence").c_str()));
								break;
							case VisuallyImpairedTextSequence:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:VisuallyImpairedTextSequence").c_str()));
								break;
							case Unknown:
								p_dom_element = doc.createElementNS(xsd::cxx::xml::string(p_sequence->property("namespace").toString().toStdString()).c_str(), (xsd::cxx::xml::string(p_sequence->property("localName").toString().toStdString()).c_str()));
								break;
							default:
								qWarning() << "Default case";
								break;
						}
						//Track ID: compare p_sequence->GetTrackId with QStringList oldTrackIDs and get the index i. insert QList newTrackIDs.at(i)!
						cpl2016::SequenceType sequence(ImfXmlHelper::Convert(QUuid::createUuid()), ImfXmlHelper::Convert(newTrackIDs.at(oldTrackIDs.indexOf(p_sequence->GetTrackId().toString()))), resource_list);
						if(p_dom_element && p_sequence->GetType() != Unknown) {
							// Import namespaces from map for wildcard content.
							for(xml_schema::NamespaceInfomap::iterator iter = cpl_namespace.begin(); iter != cpl_namespace.end(); ++iter) {
								QString ns("xmlns");
								if(iter->first.empty() == false) {
									ns.append(":");
									ns.append(iter->first.c_str());
								}
								p_dom_element->setAttributeNS(xsd::cxx::xml::string(XML_NAMESPACE_NS).c_str(), xsd::cxx::xml::string(ns.toStdString()).c_str(), xsd::cxx::xml::string(iter->second.name).c_str());
							}
							*p_dom_element << sequence;
							r_any_sequence.push_back(p_dom_element);
						}
					}
				}
			}
			cpl2016::SegmentType segment(ImfXmlHelper::Convert(QUuid::createUuid()), sequence_list);
			if(p_segment->GetAnnotationText().IsEmpty() == false) segment.setAnnotation(ImfXmlHelper::Convert(p_segment->GetAnnotationText()));
			segment_sequence.push_back(segment);
		}
	}
	cpl.setSegmentList(segment_list);
//WR begin
	cpl.setEssenceDescriptorList(essence_descriptor_list);
//WR end
	QString destination(rDestination);
	if(destination.isEmpty() && mAssetCpl) {
		destination = mAssetCpl->GetPath().absoluteFilePath();
	}
	if(destination.isEmpty() == false) {
		XmlSerializationError serialization_error;
		std::ofstream cpl_ofs(destination.toStdString().c_str(), std::ofstream::out);
		try {
			cpl2016::serializeCompositionPlaylist(cpl_ofs, cpl, cpl_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
		}
		catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
		catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
		catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
		cpl_ofs.close();
		if(serialization_error.IsError() == true) {
			qDebug() << serialization_error;
			error = ImfError(serialization_error);
		}
	}
	else {
		error = ImfError(ImfError::DestinationFileUnspecified, tr("Couldn't write cpl!"));
	}
	if(!error) {
		if(mAssetCpl) mAssetCpl->FileModified();
		//WR begin
		if(mAssetCpl) mAssetCpl->SetIsNewOrModified(true);
		//WR end
		qDebug() << "WriteNew " << destination.toStdString().c_str();
	}
	return error;
}

ImfError WidgetComposition::ParseCpl() {

	ImfError error;
	XmlParsingError parse_error;
	ImageSequenceIndex = -1; // (k)

	// ---Parse Cpl---
	std::auto_ptr<cpl2016::CompositionPlaylistType> cpl;
	try {
		cpl = cpl2016::parseCompositionPlaylist(mAssetCpl->GetPath().absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize);
	}
	catch(const xml_schema::Parsing &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::ExpectedElement &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::UnexpectedElement &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::ExpectedAttribute &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::UnexpectedEnumerator &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::ExpectedTextContent &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::NoTypeInfo &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::NotDerived &e) { parse_error = XmlParsingError(e); }
	catch(const xml_schema::NoPrefixMapping &e) { parse_error = XmlParsingError(e); }
	catch(...) { parse_error = XmlParsingError(XmlParsingError::Unknown); }

	if(parse_error.IsError() == true) {  //probably a 2013 CPL ?
		XmlParsingError parse_error2;
		std::auto_ptr<cpl::CompositionPlaylistType> cpl2013;
		try {
			cpl2013 = cpl::parseCompositionPlaylist(mAssetCpl->GetPath().absoluteFilePath().toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize);
		}
		catch(const xml_schema::Parsing &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::ExpectedElement &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::UnexpectedElement &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::ExpectedAttribute &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::UnexpectedEnumerator &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::ExpectedTextContent &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::NoTypeInfo &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::NotDerived &e) { parse_error2 = XmlParsingError(e); }
		catch(const xml_schema::NoPrefixMapping &e) { parse_error2 = XmlParsingError(e); }
		catch(...) { parse_error2 = XmlParsingError(XmlParsingError::Unknown); }
		if (parse_error2.IsError() == false) {
			qDebug()  << "CPl 2013 detected!";
			QString tempFile = nullptr;
			bool conversionError = false;
			try {
				// This is a Q&D hack to convert ST 2067-3:2013 CPLs into ST 2067-3:2016 CPLs,
				// pending a sophisticated solution using proper XLS Transformation.
				// QTemporaryFile was causing issues under Windows, the delete cpl2016_file_tmp command is required for WIndows
				QTemporaryFile* cpl2016_file_tmp = new QTemporaryFile();
				if (!cpl2016_file_tmp->open()) {
					qDebug() << "Cant't create temp file at " << cpl2016_file_tmp->fileName();
					conversionError = true;
				}
				QFile f_in(mAssetCpl->GetPath().absoluteFilePath());
				if (!f_in.open(QFile::ReadOnly | QFile::Text)) conversionError = true;
				QTextStream in(&f_in);
				QString tempCPL = in.readAll();
				// Markers should remain in 2013 namespace
				tempCPL.replace("http://www.smpte-ra.org/schemas/2067-3/2013#standard-markers", "markersscope2013xx");
				// ContentKind should remain in 2013 namespace
				tempCPL.replace("http://www.smpte-ra.org/schemas/2067-3/2013#content-kind", "contentkind2013xx");
				tempCPL.replace("http://www.smpte-ra.org/schemas/2067-3/2013", "http://www.smpte-ra.org/schemas/2067-3/2016");
				tempCPL.replace("http://www.smpte-ra.org/schemas/2067-2/2013", "http://www.smpte-ra.org/schemas/2067-2/2016");
				tempCPL.replace("markersscope2013xx", "http://www.smpte-ra.org/schemas/2067-3/2013#standard-markers");
				tempCPL.replace("contentkind2013xx", "http://www.smpte-ra.org/schemas/2067-3/2013#content-kind");
				cpl2016_file_tmp->write(tempCPL.toUtf8());
				tempFile = cpl2016_file_tmp->fileName();
				cpl2016_file_tmp->setAutoRemove(false);
				if (!cpl2016_file_tmp->flush()) {
					qDebug() << "Cant't flush to temp file at " << cpl2016_file_tmp->fileName();
					conversionError = true;
				}
				cpl2016_file_tmp->close();
				delete cpl2016_file_tmp;
			}
			catch (...) { qDebug() << "Transformation of 2013 CPL to 2016 CPL failed"; conversionError = true; }
			if (!conversionError) {
				parse_error = XmlParsingError();
				try { cpl = cpl2016::parseCompositionPlaylist(tempFile.toStdString(), xml_schema::Flags::dont_validate | xml_schema::Flags::dont_initialize); }
				catch (const xml_schema::Parsing &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::ExpectedElement &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::UnexpectedElement &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::ExpectedAttribute &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::UnexpectedEnumerator &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::ExpectedTextContent &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::NoTypeInfo &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::NotDerived &e) { parse_error = XmlParsingError(e); }
				catch (const xml_schema::NoPrefixMapping &e) { parse_error = XmlParsingError(e); }
				catch (...) { qDebug() << "Parsing transformed 2016 CPL failed"; }
				qDebug() << parse_error;
				//cleanup
				try { QFile::remove(tempFile); }
				catch (...) {} // just ignore..
			}
		}

	}

	if(parse_error.IsError() == false) {
		mData = *cpl;
		mpCompositionScene->SetCplEditRate(GetEditRate());
		mpTimelineScene->SetCplEditRate(GetEditRate());
		// Iterate segments.
		for(unsigned int i = 0; i < mData.getSegmentList().getSegment().size(); i++) {
			cpl2016::CompositionPlaylistType_SegmentListType::SegmentType &r_segment = mData.getSegmentList().getSegment().at(i);
			// Add graphics segment and segment indicator
			GraphicsWidgetSegmentIndicator *p_graphics_segment_indicator = new GraphicsWidgetSegmentIndicator(mpTimelineGraphicsWidget, GraphicsHelper::GetSegmentColor(i), ImfXmlHelper::Convert(r_segment.getId()));
			GraphicsWidgetSegment *p_graphics_segment = new GraphicsWidgetSegment(mpCompositionGraphicsWidget, GraphicsHelper::GetSegmentColor(i, true), ImfXmlHelper::Convert(r_segment.getId()), ImfXmlHelper::Convert(r_segment.getAnnotation().present() ? r_segment.getAnnotation().get() : dcml::UserTextType()));
			connect(p_graphics_segment, SIGNAL(DurationChanged(const Duration&)), p_graphics_segment_indicator, SLOT(rSegmentDurationChange(const Duration&)));
			connect(p_graphics_segment_indicator, SIGNAL(HoverActive(bool)), p_graphics_segment, SLOT(rSegmentIndicatorHoverActive(bool)));
			mpTimelineGraphicsWidget->AddSegmentIndicator(p_graphics_segment_indicator, i);
			mpCompositionGraphicsWidget->AddSegment(p_graphics_segment, i);

			cpl2016::SegmentType::SequenceListType &r_sequence_list = r_segment.getSequenceList();
			cpl2016::SegmentType_SequenceListType::MarkerSequenceOptional &r_marker_sequence = r_sequence_list.getMarkerSequence();
			// Add marker sequence if present.
			if(r_marker_sequence.present() == true) {
				cpl2016::SequenceType &r_sequence(r_marker_sequence.get());
				AbstractWidgetTrackDetails *p_track_detail = GetTrackDetail(ImfXmlHelper::Convert(r_sequence.getTrackId()));
				GraphicsWidgetSequence *p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, MarkerSequence, ImfXmlHelper::Convert(r_sequence.getTrackId()), ImfXmlHelper::Convert(r_sequence.getId()));
				if(p_track_detail == NULL) {
					p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(r_sequence.getTrackId()), MarkerSequence, mpCompositionTracksWidget);
				}
				connect(p_track_detail, SIGNAL(HeightAboutToChange(qint32)), p_graphics_sequence, SLOT(SetHeight(qint32)));
				AddTrackDetail(p_track_detail, GetTrackDetailCount());
				p_graphics_segment->AddSequence(p_graphics_sequence, p_graphics_segment->GetSequenceCount());

				// Iterate marker resources.
				for(cpl2016::SequenceType_ResourceListType::ResourceIterator resource_iter(r_sequence.getResourceList().getResource().begin()); resource_iter != r_sequence.getResourceList().getResource().end(); ++resource_iter) {
					cpl2016::MarkerResourceType *p_marker_resource = dynamic_cast<cpl2016::MarkerResourceType*>(&(*resource_iter));
					if(p_marker_resource) {
						p_graphics_sequence->AddResource(new GraphicsWidgetMarkerResource(p_graphics_sequence, p_marker_resource->_clone()), p_graphics_sequence->GetResourceCount());
					}
					else {
						qWarning() << "Unknown BaseResourceType inheritance. Not complaint with SMPTE ST 2067-3:2013.";
						error = ImfError(ImfError::UnknownInheritance, QString("Unknown BaseResourceType inheritance. Not complaint with SMPTE ST 2067-3:2013."), true);
					}
				}
				if(r_sequence.getResourceList().getResource().size() == 0) {
					p_graphics_sequence->AddResource(new GraphicsWidgetMarkerResource(p_graphics_sequence), p_graphics_sequence->GetResourceCount());
				}
			}
			// Iterate "any" sequences.
			cpl2016::SegmentType_SequenceListType::AnySequence &r_any_sequence(r_sequence_list.getAny());
			for(cpl2016::SegmentType_SequenceListType::AnySequence::iterator sequence_iter(r_any_sequence.begin()); sequence_iter != r_any_sequence.end(); ++sequence_iter) {
				xercesc::DOMElement& element(*sequence_iter);
				cpl2016::SequenceType sequence(element);
				std::string name(xsd::cxx::xml::transcode<char>(element.getLocalName()));
				std::string name_space(xsd::cxx::xml::transcode<char>(element.getNamespaceURI()));
				AbstractWidgetTrackDetails *p_track_detail = GetTrackDetail(ImfXmlHelper::Convert(sequence.getTrackId()));
				// Add graphics sequence
				GraphicsWidgetSequence *p_graphics_sequence = NULL;
				if(name == "MainImageSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), MainImageSequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, MainImageSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "MainAudioSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetAudioTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, MainAudioSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "CommentarySequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), CommentarySequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, CommentarySequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "KaraokeSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), KaraokeSequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, KaraokeSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "SubtitlesSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), SubtitlesSequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, SubtitlesSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "VisuallyImpairedTextSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), VisuallyImpairedTextSequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, VisuallyImpairedTextSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "HearingImpairedCaptionsSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), HearingImpairedCaptionsSequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, HearingImpairedCaptionsSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else if(name == "AncillaryDataSequence") {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), AncillaryDataSequence, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, AncillaryDataSequence, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
				}
				else {
					if(p_track_detail == NULL) p_track_detail = new WidgetTrackDetails(ImfXmlHelper::Convert(sequence.getTrackId()), Unknown, mpCompositionTracksWidget);
					p_graphics_sequence = new GraphicsWidgetSequence(p_graphics_segment, Unknown, ImfXmlHelper::Convert(sequence.getTrackId()), ImfXmlHelper::Convert(sequence.getId()));
					p_graphics_sequence->setProperty("localName", QVariant(QString(name.c_str())));
					p_graphics_segment->setProperty("namespace", QVariant(QString(name_space.c_str())));
					qWarning() << "Unknown sequence type found [" << name.c_str() << ", " << name_space.c_str() << "]: " << p_graphics_sequence->GetId();
				}
				connect(p_track_detail, SIGNAL(HeightAboutToChange(qint32)), p_graphics_sequence, SLOT(SetHeight(qint32)));
				AddTrackDetail(p_track_detail, GetTrackDetailCount());
				p_graphics_segment->AddSequence(p_graphics_sequence, p_graphics_segment->GetSequenceCount());

				// Iterate resources.
				for(cpl2016::SequenceType_ResourceListType::ResourceIterator resource_iter(sequence.getResourceList().getResource().begin()); resource_iter != sequence.getResourceList().getResource().end(); ++resource_iter) {
					cpl2016::TrackFileResourceType *p_file_resource = dynamic_cast<cpl2016::TrackFileResourceType*>(&(*resource_iter));
					if(p_file_resource) {
						switch(p_graphics_sequence->GetType()) {
							case MainImageSequence: 
								ImageSequenceIndex++;
								if (mImp) p_graphics_sequence->AddResource(new GraphicsWidgetVideoResource(p_graphics_sequence, p_file_resource->_clone(), mImp->GetAsset(ImfXmlHelper::Convert(p_file_resource->getTrackFileId())).objectCast<AssetMxfTrack>(), ImageSequenceIndex), p_graphics_sequence->GetResourceCount());
								else p_graphics_sequence->AddResource(new GraphicsWidgetVideoResource(p_graphics_sequence, p_file_resource->_clone()), p_graphics_sequence->GetResourceCount());
								break;
							case MainAudioSequence:
								if(mImp) p_graphics_sequence->AddResource(new GraphicsWidgetAudioResource(p_graphics_sequence, p_file_resource->_clone(), mImp->GetAsset(ImfXmlHelper::Convert(p_file_resource->getTrackFileId())).objectCast<AssetMxfTrack>()), p_graphics_sequence->GetResourceCount());
								else p_graphics_sequence->AddResource(new GraphicsWidgetAudioResource(p_graphics_sequence, p_file_resource->_clone()), p_graphics_sequence->GetResourceCount());
								break;
							case CommentarySequence:
							case HearingImpairedCaptionsSequence:
							case KaraokeSequence:
							case SubtitlesSequence:
							case VisuallyImpairedTextSequence:
								if(mImp) p_graphics_sequence->AddResource(new GraphicsWidgetTimedTextResource(p_graphics_sequence, p_file_resource->_clone(), mImp->GetAsset(ImfXmlHelper::Convert(p_file_resource->getTrackFileId())).objectCast<AssetMxfTrack>()), p_graphics_sequence->GetResourceCount());
								else p_graphics_sequence->AddResource(new GraphicsWidgetTimedTextResource(p_graphics_sequence, p_file_resource->_clone()), p_graphics_sequence->GetResourceCount());
								break;
							case AncillaryDataSequence:
								if(mImp) p_graphics_sequence->AddResource(new GraphicsWidgetAncillaryDataResource(p_graphics_sequence, p_file_resource->_clone(), mImp->GetAsset(ImfXmlHelper::Convert(p_file_resource->getTrackFileId())).objectCast<AssetMxfTrack>()), p_graphics_sequence->GetResourceCount());
								else p_graphics_sequence->AddResource(new GraphicsWidgetAncillaryDataResource(p_graphics_sequence, p_file_resource->_clone()), p_graphics_sequence->GetResourceCount());
								break;
							case Unknown:
								qDebug() << "A generic file resource will be added to unknown sequence.";
								if(mImp) p_graphics_sequence->AddResource(new GraphicsWidgetFileResource(p_graphics_sequence, p_file_resource->_clone(), mImp->GetAsset(ImfXmlHelper::Convert(p_file_resource->getTrackFileId())).objectCast<AssetMxfTrack>()), p_graphics_sequence->GetResourceCount());
								else p_graphics_sequence->AddResource(new GraphicsWidgetFileResource(p_graphics_sequence, p_file_resource->_clone()), p_graphics_sequence->GetResourceCount());
								break;
							default:
								qWarning() << "Default case";
								break;
						}
					}
					else {
						qWarning() << "Unknown BaseResourceType inheritance. Not complaint with SMPTE ST 2067-3:2013.";
						error = ImfError(ImfError::UnknownInheritance, QString("Unknown BaseResourceType inheritance. Not complaint with SMPTE ST 2067-3:2013."), true);
					}
				}
			}
		}
	}
	else {
		qDebug() << parse_error;
		error = ImfError(parse_error);
	}
	return error;
}

void WidgetComposition::FitCompositionSceneRect() {

	mpCompositionView->scene()->setSceneRect(mpCompositionGraphicsWidget->boundingRect());
}

AbstractWidgetTrackDetails* WidgetComposition::GetTrackDetail(const QUuid &rId) const {

	for(int i = 0; i < mpTrackSplitter->count(); i++) {
		AbstractWidgetTrackDetails *p_track_detail = qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(i));
		if(p_track_detail && p_track_detail->GetId() == rId) return p_track_detail;
	}
	return NULL;
}

AbstractWidgetTrackDetails* WidgetComposition::GetTrackDetail(int trackIndex) const {

	return qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(trackIndex));
}

void WidgetComposition::AddTrackDetail(AbstractWidgetTrackDetails* pTrack, int TrackIndex) {

	WidgetAudioTrackDetails *p_audio_track_details = qobject_cast<WidgetAudioTrackDetails*>(pTrack);
	mpTrackSplitter->insertWidget(TrackIndex, pTrack);
	connect(pTrack, SIGNAL(DeleteClicked(const QUuid&)), this, SLOT(DeleteTrackRequest(const QUuid&)));
}

void WidgetComposition::RemoveTrackDetail(AbstractWidgetTrackDetails *pTrack) {

	WidgetAudioTrackDetails *p_audio_track_details = qobject_cast<WidgetAudioTrackDetails*>(pTrack);
	disconnect(pTrack, SIGNAL(DeleteClicked(const QUuid&)), this, SLOT(DeleteTrackRequest(const QUuid&)));
	pTrack->setParent(NULL);
}

void WidgetComposition::MoveTrackDetail(AbstractWidgetTrackDetails* pTrack, int NewTrackIndex) {

	RemoveTrackDetail(pTrack);
	AddTrackDetail(pTrack, NewTrackIndex);
}

int WidgetComposition::GetTrackDetailCount() const {

	int track_count = mpTrackSplitter->count();
	if(track_count >= 0) return track_count;
	return 0;
}

int WidgetComposition::GetTrackDetailIndex(const AbstractWidgetTrackDetails *pTrackDetails) const {

	for(int i = 0; i < mpTrackSplitter->count(); i++) {
		AbstractWidgetTrackDetails *p_track_detail = qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(i));
		if(p_track_detail == pTrackDetails) return i;
	}
	return -1;
}

void WidgetComposition::rPushCommand(QUndoCommand *pCommand) {

	mpUndoStack->push(pCommand);
}

void WidgetComposition::rSplitterMoved(int pos, int index) {

	QSplitter *p_splitter = qobject_cast<QSplitter*>(sender());
	if(p_splitter && p_splitter == mpLeftInnerSplitter) mpRightInnerSplitter->setSizes(mpLeftInnerSplitter->sizes());
	else if(p_splitter && p_splitter == mpRightInnerSplitter) mpLeftInnerSplitter->setSizes(mpRightInnerSplitter->sizes());
}

void WidgetComposition::InitStyle() {

	mpTimelineView->setObjectName("TimelineView");
	mpTimelineView->setStyleSheet(QString(
		"QFrame#%1 {"
		"background-color: %2;"
		"border-width: 1px;"
		"border-top-width: 0px;"
		"border-right-width: 0px;"
		"border-left-width: 0px;"
		"border-color: %3;"
		"border-style: solid;"
		"border-radius: 0px;"
		"}").arg(mpTimelineView->objectName()).arg(QColor(CPL_COLOR_BACKGROUND).name()).arg(QColor(CPL_BORDER_COLOR).name()));

	mpCompositionView->setObjectName("CompositionView");
	mpCompositionView->setStyleSheet(QString(
		"QFrame#%1 {"
		"background-color: %2;"
		"border-width: 1px;"
		"border-bottom-width: 0px;"
		"border-right-width: 0px;"
		"border-left-width: 0px;"
		"border-color: %3;"
		"border-style: solid;"
		"border-radius: 0px;"
		"}").arg(mpCompositionView->objectName()).arg(QColor(CPL_COLOR_BACKGROUND).name()).arg(QColor(CPL_BORDER_COLOR).name()));

	mpTrackSplitter->setObjectName("TrackSplitter");
	mpTrackSplitter->setStyleSheet(QString(
		"QFrame#%1 {"
		"border-width: 1px;"
		"border-bottom-width: 0px;"
		"border-right-width: 0px;"
		"border-left-width: 0px;"
		"border-color: %3;"
		"border-style: solid;"
		"border-radius: 0px;"
		"}").arg(mpTrackSplitter->objectName()).arg(QColor(CPL_BORDER_COLOR).name()));
}

void WidgetComposition::InitToolbar() {

	QToolButton *p_button_add_track = new QToolButton(NULL);
	p_button_add_track->setIcon(QIcon(":/add.png"));
	p_button_add_track->setText(tr("Add Track"));
	p_button_add_track->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	p_button_add_track->setPopupMode(QToolButton::InstantPopup);
	QMenu *p_add_track_menu = new QMenu(tr("Add Track"), this);
	mpAddMainAudioTrackAction = p_add_track_menu->addAction(QIcon(":/sound.png"), tr("main audio track"));
	mpAddSubtitlesTrackAction = p_add_track_menu->addAction(QIcon(":/text.png"), tr("subtitles track"));
	p_add_track_menu->addSeparator();
	mpAddMarkerTrackAction = p_add_track_menu->addAction(QIcon(":/marker.png"), tr("marker track"));
	p_button_add_track->setMenu(p_add_track_menu);
	mpToolBar->addWidget(p_button_add_track);
	QAction *p_action = mpToolBar->addAction(QIcon(":/cutter.png"), tr("Edit"), mpCompositionScene, SLOT(SetEditRequest()));
	p_action->setShortcut(Qt::Key_E);
	p_action->setAutoRepeat(false);
	connect(p_add_track_menu, SIGNAL(aboutToShow()), this, SLOT(rAddTrackMenuAboutToShow()));
	connect(p_add_track_menu, SIGNAL(triggered(QAction*)), this, SLOT(rAddTrackMenuActionTriggered(QAction*)));
}

void WidgetComposition::AddNewSegmentRequest(int segmentIndex) {

	QUndoCommand *p_root = new QUndoCommand(NULL);
	QUuid uuid(QUuid::createUuid());
	GraphicsWidgetSegment *p_segment = new GraphicsWidgetSegment(mpCompositionGraphicsWidget, GraphicsHelper::GetSegmentColor(mpCompositionGraphicsWidget->GetSegmentCount(), true), uuid);
	GraphicsWidgetSegmentIndicator *p_segment_indicator = new GraphicsWidgetSegmentIndicator(mpTimelineGraphicsWidget, GraphicsHelper::GetSegmentColor(mpCompositionGraphicsWidget->GetSegmentCount()), uuid);
	AddSegmentCommand *p_add_segment = new AddSegmentCommand(p_segment, p_segment_indicator, segmentIndex, mpCompositionGraphicsWidget, mpTimelineGraphicsWidget, p_root);

	for(int i = 0; i < GetTrackDetailCount(); i++) {
		AbstractWidgetTrackDetails *p = GetTrackDetail(i);
		GraphicsWidgetSequence *p_sequence = new GraphicsWidgetSequence(p_segment, GetTrackDetail(i)->GetType(), GetTrackDetail(i)->GetId());
		p_sequence->SetHeight(GetTrackDetail(i)->height());
		connect(GetTrackDetail(i), SIGNAL(HeightAboutToChange(qint32)), p_sequence, SLOT(SetHeight(qint32)));
		AddSequenceCommand *p_add_sequence = new AddSequenceCommand(p_sequence, i, p_segment, p_root);
		if(p_sequence->GetType() == MarkerSequence) {
			AddResourceCommand *p_add_resource = new AddResourceCommand(new GraphicsWidgetMarkerResource(p_sequence), 0, p_sequence, p_root);
		}
	}
	mpUndoStack->push(p_root);
	p_segment->SetDuration(60); // TODO: size depending on zoom level.
	mpCompositionGraphicsWidget->layout()->activate();
	mpTimelineGraphicsWidget->layout()->activate();
}

void WidgetComposition::DeleteSegmentRequest(const QUuid &rId) {

	GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(rId);
	GraphicsWidgetSegmentIndicator *p_segment_indicator = mpTimelineGraphicsWidget->GetSegmentIndicator(rId);
	if(p_segment && p_segment_indicator) {
		mpUndoStack->push(new RemoveSegmentCommand(p_segment, p_segment_indicator, mpCompositionGraphicsWidget, mpTimelineGraphicsWidget));
	}
}

void WidgetComposition::DeleteSegmentRequest(int segmentIndex) {

	GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(segmentIndex);
	GraphicsWidgetSegmentIndicator *p_segment_indicator = mpTimelineGraphicsWidget->GetSegmentIndicator(segmentIndex);
	if(p_segment && p_segment_indicator) {
		mpUndoStack->push(new RemoveSegmentCommand(p_segment, p_segment_indicator, mpCompositionGraphicsWidget, mpTimelineGraphicsWidget));
	}
}

void WidgetComposition::MoveSegmentRequest(const QUuid &rId, const QUuid &rTargetSegmentId) {

	int new_index = mpCompositionGraphicsWidget->GetSegmentIndex(mpCompositionGraphicsWidget->GetSegment(rTargetSegmentId));
	MoveSegmentRequest(rId, new_index);
}

void WidgetComposition::MoveSegmentRequest(const QUuid &rId, int targetIndex) {

	GraphicsWidgetSegment *p_segment = mpCompositionGraphicsWidget->GetSegment(rId);
	GraphicsWidgetSegmentIndicator *p_segment_indicator = mpTimelineGraphicsWidget->GetSegmentIndicator(rId);
	if(p_segment && p_segment_indicator && targetIndex >= 0) {
		mpUndoStack->push(new MoveSegmentCommand(p_segment, p_segment_indicator, targetIndex, mpCompositionGraphicsWidget, mpTimelineGraphicsWidget));
	}
}

int WidgetComposition::GetTrackCount(eSequenceType type) const {

	int count = 0;
	for(int i = 0; i < mpTrackSplitter->count(); i++) {
		AbstractWidgetTrackDetails *p_track_detail = qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(i));
		if(p_track_detail && p_track_detail->GetType() == type) count++;
	}
	return count;
}

void WidgetComposition::rAddTrackMenuAboutToShow() {

	if(GetTrackCount(MarkerSequence) < 1) mpAddMarkerTrackAction->setEnabled(true);
	else mpAddMarkerTrackAction->setDisabled(true);

	//if(GetTrackCount(MainAudioSequence) < 1) mpAddMainAudioTrackAction->setEnabled(true);
	//else mpAddMainAudioTrackAction->setDisabled(true);
}



void WidgetComposition::rToolBarActionTriggered(QAction *pAction) {

}

void WidgetComposition::rAddTrackMenuActionTriggered(QAction *pAction) {

	if(pAction == mpAddMarkerTrackAction) AddNewTrackRequest(MarkerSequence);
	else if(pAction == mpAddMainAudioTrackAction) AddNewTrackRequest(MainAudioSequence);
	else if(pAction == mpAddSubtitlesTrackAction) AddNewTrackRequest(SubtitlesSequence);
	else AddNewTrackRequest(Unknown);
}

int WidgetComposition::GetLastTrackDetailIndexForType(eSequenceType type) const {

	int index = -1;
	for(int i = 0; i < mpTrackSplitter->count(); i++) {
		AbstractWidgetTrackDetails *p_track_detail = qobject_cast<AbstractWidgetTrackDetails*>(mpTrackSplitter->widget(i));
		if(p_track_detail && p_track_detail->GetType() == type) index = i;
	}
	return index;
}

void WidgetComposition::rCurrentFrameChanged(const Timecode &rCplTimecode) {

	lastPosition = rCplTimecode; // (k) save last position
	//qDebug() << "xpos" << rCplTimecode.AsPositiveDuration().GetCount();

	GraphicsWidgetSegment *p_segment = mpCompositionScene->GetSegmentAt(rCplTimecode);
	if (p_segment) {
		QUuid audio_track_id;
		QUuid video_track_id;
		QUuid ttml_track_id; // (k)
		for (int i = 0; i < GetTrackDetailCount(); i++) {
			AbstractWidgetTrackDetails *p_track_details = GetTrackDetail(i);
			if (p_track_details) {
				//if(p_track_details->GetType() == MainAudioSequence) {
				//	if(WidgetAudioTrackDetails *p_audio_track = static_cast<WidgetAudioTrackDetails*>(p_track_details)) {
				//		if(p_audio_track->GetSoloButton()->isChecked() == true) audio_track_id = p_audio_track->GetId();
				//	}
				//}
				if (p_track_details->GetType() == MainImageSequence) {
					video_track_id = p_track_details->GetId();
				}
				else if (p_track_details->GetType() == SubtitlesSequence) {
					ttml_track_id = p_track_details->GetId();
				}
			}
		}

		QList<AbstractGraphicsWidgetResource*> resources_list = mpCompositionScene->GetResourcesAt(rCplTimecode, SubtitlesSequence | MainAudioSequence | MainImageSequence);
		for (int i = 0; i < resources_list.size(); i++) {
			GraphicsWidgetSequence *p_seq = dynamic_cast<GraphicsWidgetSequence*>(resources_list.at(i)->GetSequence());
			if (p_seq) {
				//if(p_seq->GetTrackId() == audio_track_id) {
				//	AbstractGraphicsWidgetResource *p_resource = resources_list.at(i);
				//	emit CurrentAudioChanged(p_resource->GetAsset(), (p_resource->MapToCplTimeline(Timecode()) - rCplTimecode).AsPositiveDuration(), rCplTimecode);
				//}
				if (p_seq->GetTrackId() == video_track_id) {
					AbstractGraphicsWidgetResource *p_resource = resources_list.at(i);
					if (p_resource->GetLastVisibleFrame().GetOverallFrames() > -1) { // avoids wrong signals near segment transitions
						qint64 assetPosition = (p_resource->MapToCplTimeline(Timecode()) - rCplTimecode).AsPositiveDuration().GetCount();
						assetPosition = p_resource->GetEntryPoint().GetCount() + (assetPosition - p_resource->GetEntryPoint().GetCount());// % p_resource->GetSourceDuration().GetCount());
						//emit CurrentVideoChanged(p_resource->GetAsset(), (p_resource->MapToCplTimeline(Timecode()) - rCplTimecode).AsPositiveDuration().GetCount(), rCplTimecode, p_resource->timline_index);
						emit CurrentVideoChanged(p_resource->GetAsset(), assetPosition, rCplTimecode, p_resource->timline_index);
						return;
					}
				}
				else if (p_seq->GetTrackId() == ttml_track_id && video_track_id.isNull()) { // (k)
					AbstractGraphicsWidgetResource *p_resource = resources_list.at(i);
					emit CurrentVideoChanged(p_resource->GetAsset(), (p_resource->MapToCplTimeline(Timecode()) - rCplTimecode).AsPositiveDuration().GetCount(), rCplTimecode, p_resource->timline_index);
					return;
				}
			}
		}
	}
}

XmlSerializationError WidgetComposition::WriteMinimal(const QString &rDestination, const QUuid &rId, const EditRate &rEditRate, const UserText &rContentTitle, const UserText &rIssuer /*= UserText()*/, const UserText &rContentOriginator /*= UserText()*/) {

	xml_schema::NamespaceInfomap cpl_namespace;
	cpl_namespace[""].name = XML_NAMESPACE_CPL;
	cpl_namespace["dcml"].name = XML_NAMESPACE_DCML;
	cpl_namespace["cc"].name = XML_NAMESPACE_CC;
	cpl_namespace["ds"].name = XML_NAMESPACE_DS;
	cpl_namespace["xs"].name = XML_NAMESPACE_XS;

	cpl2016::CompositionPlaylistType_SegmentListType segment_list;
	cpl2016::CompositionPlaylistType_SegmentListType::SegmentSequence &segment_sequence = segment_list.getSegment();

	cpl2016::SegmentType_SequenceListType sequence_list;
	cpl2016::SegmentType_SequenceListType::AnySequence &r_any_sequence(sequence_list.getAny());
	xercesc::DOMDocument &doc = sequence_list.getDomDocument();
	xercesc::DOMElement* p_dom_element = NULL;
	p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(), (xsd::cxx::xml::string("cc:MainImageSequence").c_str()));
	if(p_dom_element) {
		// Import namespaces from map for wildcard content.
		for(xml_schema::NamespaceInfomap::iterator iter = cpl_namespace.begin(); iter != cpl_namespace.end(); ++iter) {
			QString ns("xmlns");
			if(iter->first.empty() == false) {
				ns.append(":");
				ns.append(iter->first.c_str());
			}
			p_dom_element->setAttributeNS(xsd::cxx::xml::string(XML_NAMESPACE_NS).c_str(), xsd::cxx::xml::string(ns.toStdString()).c_str(), xsd::cxx::xml::string(iter->second.name).c_str());
		}
		cpl2016::SequenceType::ResourceListType resources;
		cpl2016::SequenceType sequence(ImfXmlHelper::Convert(QUuid::createUuid()), ImfXmlHelper::Convert(QUuid::createUuid()), resources);
		*p_dom_element << sequence;
		r_any_sequence.push_back(p_dom_element);
	}
	cpl2016::SegmentType segment(ImfXmlHelper::Convert(QUuid::createUuid()), sequence_list);
	segment_sequence.push_back(segment);

	cpl2016::CompositionPlaylistType cpl(ImfXmlHelper::Convert(rId), ImfXmlHelper::Convert(QDateTime::currentDateTimeUtc()), ImfXmlHelper::Convert(rContentTitle), ImfXmlHelper::Convert(rEditRate), segment_list);
	cpl.setCreator(ImfXmlHelper::Convert(UserText(CREATOR_STRING)));
	if(rIssuer.IsEmpty() == false) cpl.setIssuer(ImfXmlHelper::Convert(rIssuer));
	if(rContentOriginator.IsEmpty() == false) cpl.setContentOriginator(ImfXmlHelper::Convert(rIssuer));

	XmlSerializationError serialization_error;
	std::ofstream cpl_ofs(rDestination.toStdString().c_str(), std::ofstream::out);
	try {
		cpl2016::serializeCompositionPlaylist(cpl_ofs, cpl, cpl_namespace, "UTF-8", xml_schema::Flags::dont_initialize);
	}
	catch(xml_schema::Serialization &e) { serialization_error = XmlSerializationError(e); }
	catch(xml_schema::UnexpectedElement &e) { serialization_error = XmlSerializationError(e); }
	catch(xml_schema::NoTypeInfo &e) { serialization_error = XmlSerializationError(e); }
	catch(...) { serialization_error = XmlSerializationError(XmlSerializationError::Unknown); }
	cpl_ofs.close();
	if(serialization_error.IsError() == true) {
		qDebug() << serialization_error;
	}
	return serialization_error;
}


UserText WidgetComposition::GetApplicationIdentification() const {
	if (mData.getExtensionProperties().present()) {
		cpl2016::CompositionPlaylistType_ExtensionPropertiesType sequence_list = mData.getExtensionProperties().get();
		cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence &r_any_sequence(sequence_list.getAny());
		return UserText(XMLString::transcode(r_any_sequence.front().getFirstChild()->getNodeValue()));
	} else {
		return UserText();
	}
}

void WidgetComposition::SetApplicationIdentification(const UserText &rApplicationIdentification) {
	if (!mData.getExtensionProperties().present()) {
		cpl2016::CompositionPlaylistType_ExtensionPropertiesType exProp;
		mData.setExtensionProperties(exProp);
	}
	cpl2016::CompositionPlaylistType_ExtensionPropertiesType sequence_list = mData.getExtensionProperties().get();
	cpl2016::CompositionPlaylistType_ExtensionPropertiesType::AnySequence &r_any_sequence(sequence_list.getAny());
	if ( !r_any_sequence.empty() ) {
		try {
			r_any_sequence.front().getFirstChild()->setNodeValue(XMLString::transcode(rApplicationIdentification.first.toStdString().c_str()));
			sequence_list.setAny(r_any_sequence);
			mData.setExtensionProperties(sequence_list);
		} catch (...) {
			qDebug() << "Error setting ApplicationIdentification!";
		}
	} else {
		try {
			xercesc::DOMDocument &doc = sequence_list.getDomDocument();
			xercesc::DOMElement* p_dom_element = NULL;
			p_dom_element = doc.createElementNS(xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str(),
					(xsd::cxx::xml::string("cc:ApplicationIdentification").c_str())
					);
			QString ns("xmlns:");
			ns.append(cpl_namespace.find("cc")->first.c_str());
			p_dom_element->setAttributeNS(xsd::cxx::xml::string(XML_NAMESPACE_NS).c_str(), xsd::cxx::xml::string(ns.toStdString()).c_str(), xsd::cxx::xml::string(cpl_namespace.find("cc")->second.name).c_str());
			p_dom_element->setTextContent(XMLString::transcode(rApplicationIdentification.first.toStdString().c_str()));
			r_any_sequence.push_back(p_dom_element);
			sequence_list.setAny(r_any_sequence);
			mData.setExtensionProperties(sequence_list);
		} catch (...) {
			qDebug() << "Error setting ApplicationIdentification!";
		}
	}

}


ImprovedSplitter::ImprovedSplitter(QWidget *pParent /*= NULL*/) :
QSplitter(pParent), mpDummyWidget(NULL) {

	mpDummyWidget = new QWidget(this);
	addWidget(mpDummyWidget);
	setStretchFactor(indexOf(mpDummyWidget), 999);
	connect(this, SIGNAL(splitterMoved(int, int)), this, SLOT(rSplitterMoved(int, int)));
	refresh();
	mOldSplitterSizes = sizes();
}

ImprovedSplitter::ImprovedSplitter(Qt::Orientation orientation, QWidget *pPparent /*= NULL*/) :
QSplitter(orientation, pPparent), mpDummyWidget(NULL) {

	mpDummyWidget = new QWidget(this);
	addWidget(mpDummyWidget);
	setStretchFactor(indexOf(mpDummyWidget), 999);
	connect(this, SIGNAL(splitterMoved(int, int)), this, SLOT(rSplitterMoved(int, int)));
	refresh();
	if(parentWidget())setFixedHeight(parentWidget()->height());
	mOldSplitterSizes = sizes();
}

void ImprovedSplitter::rSplitterMoved(int pos, int index) {

	// The splitter handle with index is hidden. So index should never be <= 0.
	if(index > 0) {
		mOldSplitterSizes[index - 1] = sizes().at(index - 1);
		int sum = 0;
		for(int i = 0; i < mOldSplitterSizes.size() - 1; i++) {
			sum += mOldSplitterSizes.at(i);
		}
		if(parentWidget())setFixedHeight(sum + parentWidget()->height());
		setSizes(mOldSplitterSizes);
		emit SizeChanged();


		// 		int sum_pos = 0;
		// 		for(int i = 0; i < index - 1 && i < sizes().size(); i++) {
		// 			sum_pos += sizes().at(i);
		// 		}
		// 		mOldSplitterSizes[index - 1] = pos - sum_pos;
		// 		int sum = 0;
		// 		for(int i = 0; i < mOldSplitterSizes.size() - 1; i++) {
		// 			sum += mOldSplitterSizes.at(i);
		// 		}
		// 		setFixedHeight(sum + 1);
		// 		setSizes(mOldSplitterSizes);
		// 		mOldSplitterSizes = sizes();
		// 		QWidget *p_parent = qobject_cast<QWidget *>(parentWidget());
		// 		int h = this->height();
		// 		if(p_parent) h = p_parent->height();
	}
}

void ImprovedSplitter::addWidget(QWidget *pWidget) {

	QSplitter::addWidget(pWidget);
	QSplitter::addWidget(mpDummyWidget);
	QSplitterHandle *p_handle = handle(indexOf(pWidget));
	if(p_handle)p_handle->installEventFilter(this);
	refresh();
	mOldSplitterSizes = sizes();
}

void ImprovedSplitter::insertWidget(int index, QWidget *pWidget) {

	QSplitter::insertWidget(index, pWidget);
	QSplitter::addWidget(mpDummyWidget);
	QSplitterHandle *p_handle = handle(indexOf(pWidget));
	if(p_handle)p_handle->installEventFilter(this);
	refresh();
	mOldSplitterSizes = sizes();
}

int ImprovedSplitter::count() {

	return QSplitter::count() - 1;
}

bool ImprovedSplitter::eventFilter(QObject *pObject, QEvent *pEvent) {

	if(pEvent->type() == QEvent::MouseButtonDblClick) {
		// unused
	}
	return false;
}

void ImprovedSplitter::childEvent(QChildEvent *pEvent) {

	QSplitter::childEvent(pEvent);
	if(pEvent->child()->isWidgetType()) {
		QWidget *p_widget = static_cast<QWidget *>(pEvent->child());
		if(p_widget->isWindow()) return;
		if(pEvent->type() == QEvent::ChildRemoved) {
			mOldSplitterSizes = sizes();
			mOldSplitterSizes.removeAt(indexOf(p_widget));
			int sum = 0;
			for(int i = 0; i < mOldSplitterSizes.size() - 1; i++) {
				sum += mOldSplitterSizes.at(i);
			}
			if(parentWidget())setFixedHeight(sum + parentWidget()->height());
		}
	}
}
