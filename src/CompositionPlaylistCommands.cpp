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
#include "CompositionPlaylistCommands.h"
#include "global.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetResources.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetTimeline.h"
#include "WidgetTrackDedails.h"
#include <WidgetComposition.h>


SetEntryPointCommand::SetEntryPointCommand(AbstractGraphicsWidgetResource *pResource, const Duration &rOldEntryPoint, const Duration &rNewEntryPoint, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpResource(pResource), mOldEntryPoint(rOldEntryPoint), mNewEntryPoint(rNewEntryPoint) {

}

void SetEntryPointCommand::undo() {

	mpResource->SetEntryPoint(mOldEntryPoint);
}

void SetEntryPointCommand::redo() {

	mpResource->SetEntryPoint(mNewEntryPoint);
}

SetSourceDurationCommand::SetSourceDurationCommand(AbstractGraphicsWidgetResource *pResource, const Duration &rOldSourceDuration, const Duration &rNewSourceDuration, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpResource(pResource), mOldSourceDuration(rOldSourceDuration), mNewSourceDuration(rNewSourceDuration) {

}

void SetSourceDurationCommand::undo() {

	mpResource->SetSourceDuration(mOldSourceDuration);
}

void SetSourceDurationCommand::redo() {

	mpResource->SetSourceDuration(mNewSourceDuration);
}

SetIntrinsicDurationCommand::SetIntrinsicDurationCommand(GraphicsWidgetMarkerResource *pResource, const Duration &rOldIntrinsicDuration, const Duration &rNewIntrinsicDuration, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpResource(pResource), mOldIntrinsicDuration(rOldIntrinsicDuration), mNewIntrinsicDuration(rNewIntrinsicDuration) {

}

void SetIntrinsicDurationCommand::undo() {

	mpResource->SetIntrinsicDuaration(mOldIntrinsicDuration);
}

void SetIntrinsicDurationCommand::redo() {

	mpResource->SetIntrinsicDuaration(mNewIntrinsicDuration);
}

AddResourceCommand::AddResourceCommand(AbstractGraphicsWidgetResource *pResource, int resourceIndex, GraphicsWidgetSequence *pSequence, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpResource(pResource), mpSequence(pSequence), mResourceIndexBackup(resourceIndex), mIsRedone(false) {

}

AddResourceCommand::~AddResourceCommand() {

	if(mIsRedone == false && mpResource) mpResource->deleteLater(); // TODO: Is this safe.
}

void AddResourceCommand::undo() {

	if(mResourceIndexBackup >= 0) {
		mpSequence->RemoveResource(mpResource);
		mpResource->hide();
		mpResource->setParentItem(NULL); // Gets top level item. Deleted by scene.
		mIsRedone = false;
		mpSequence->layout()->activate();
	}
}

void AddResourceCommand::redo() {

	if(mResourceIndexBackup >= 0) {
		mpSequence->AddResource(mpResource, mResourceIndexBackup);
		mpResource->show();
		mIsRedone = true;
		mpSequence->layout()->activate();
	}
}

RemoveResourceCommand::RemoveResourceCommand(AbstractGraphicsWidgetResource *pResource, GraphicsWidgetSequence *pSequence, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpResource(pResource), mpSequence(pSequence), mResourceIndexBackup(-1), mIsRedone(false) {

}

RemoveResourceCommand::~RemoveResourceCommand() {

	if(mIsRedone == true && mpResource) mpResource->deleteLater(); // TODO: Is this safe.
}

void RemoveResourceCommand::undo() {

	if(mResourceIndexBackup >= 0) {
		mpSequence->AddResource(mpResource, mResourceIndexBackup);
		mpResource->show();
		mIsRedone = false;
		mpSequence->layout()->activate();
	}
}

void RemoveResourceCommand::redo() {

	mResourceIndexBackup = mpSequence->GetResourceIndex(mpResource);
	if(mResourceIndexBackup >= 0) {
		mpSequence->RemoveResource(mpResource);
		mpResource->hide();
		mpResource->setParentItem(NULL); // Gets top level item. Deleted by scene.
		mIsRedone = true;
		mpSequence->layout()->activate();
	}
}

AddSequenceCommand::AddSequenceCommand(GraphicsWidgetSequence *pSequence, int sequenceIndex, GraphicsWidgetSegment *pSegment, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpSequence(pSequence), mpSegment(pSegment), mSequenceIndexBackup(sequenceIndex), mIsRedone(false) {

}

AddSequenceCommand::~AddSequenceCommand() {

	if(mIsRedone == false && mpSequence) mpSequence->deleteLater(); // TODO: Is this safe.
}

void AddSequenceCommand::undo() {

	if(mSequenceIndexBackup >= 0) {
		mpSequence->hide();
		mpSegment->RemoveSequence(mpSequence);
		mpSequence->setParentItem(NULL); // Gets top level item. Deleted by scene.
		mIsRedone = false;
		mpSegment->layout()->activate();
	}
}

void AddSequenceCommand::redo() {

	if(mSequenceIndexBackup >= 0) {
		mpSegment->AddSequence(mpSequence, mSequenceIndexBackup);
		mpSequence->show();
		mIsRedone = true;
		mpSegment->layout()->activate();
	}
}

RemoveSequenceCommand::RemoveSequenceCommand(GraphicsWidgetSequence *pSequence, GraphicsWidgetSegment *pSegment, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpSequence(pSequence), mpSegment(pSegment), mSequenceIndexBackup(-1), mIsRedone(false) {

}

RemoveSequenceCommand::~RemoveSequenceCommand() {

	if(mIsRedone == true && mpSequence) mpSequence->deleteLater(); // TODO: Is this safe.
}

void RemoveSequenceCommand::undo() {

	if(mSequenceIndexBackup >= 0) {
		mpSegment->AddSequence(mpSequence, mSequenceIndexBackup);
		mpSequence->show();
		mIsRedone = false;
		mpSegment->layout()->activate();
	}
}

void RemoveSequenceCommand::redo() {

	mSequenceIndexBackup = mpSegment->GetSequenceIndex(mpSequence);
	if(mSequenceIndexBackup >= 0) {
		mpSequence->hide();
		mpSegment->RemoveSequence(mpSequence);
		mpSequence->setParentItem(NULL); // Gets top level item. Deleted by scene.
		mIsRedone = true;
		mpSegment->layout()->activate();
	}
}

AddSegmentCommand::AddSegmentCommand(GraphicsWidgetSegment *pSegment, GraphicsWidgetSegmentIndicator *pSegmentIndicator,
																		 int segmentIndex, GraphicsWidgetComposition *pComposition, GraphicsWidgetTimeline *pTimeline,
																		 QUndoCommand *pParent /*= NULL*/) :
																		 QUndoCommand(pParent), mpSegment(pSegment), mpSegmentIndicator(pSegmentIndicator), mpComposition(pComposition), mpTimeline(pTimeline), mSegmentIndexBackup(segmentIndex),
																		 mIsRedone(false) {

}

AddSegmentCommand::~AddSegmentCommand() {

	if(mIsRedone == false && mpSegment) mpSegment->deleteLater(); // TODO: Is this safe.
}

void AddSegmentCommand::undo() {

	if(mSegmentIndexBackup >= 0) {
		QObject::disconnect(mpSegment, SIGNAL(DurationChanged(const Duration&)), mpSegmentIndicator, SLOT(rSegmentDurationChange(const Duration&)));
		QObject::disconnect(mpSegmentIndicator, SIGNAL(HoverActive(bool)), mpSegment, SLOT(rSegmentIndicatorHoverActive(bool)));
		mpSegmentIndicator->hide();
		mpSegment->hide();
		mpComposition->RemoveSegment(mpSegment);
		mpTimeline->RemoveSegmentIndicator(mpSegmentIndicator);
		mpSegment->setParentItem(NULL); // Gets top level item. Deleted by scene.
		mpSegmentIndicator->setParentItem(NULL);
		mIsRedone = false;
		mpComposition->layout()->activate();
		mpTimeline->layout()->activate();
	}
}

void AddSegmentCommand::redo() {

	if(mSegmentIndexBackup >= 0) {
		QObject::connect(mpSegment, SIGNAL(DurationChanged(const Duration&)), mpSegmentIndicator, SLOT(rSegmentDurationChange(const Duration&)));
		QObject::connect(mpSegmentIndicator, SIGNAL(HoverActive(bool)), mpSegment, SLOT(rSegmentIndicatorHoverActive(bool)));
		mpTimeline->AddSegmentIndicator(mpSegmentIndicator, mSegmentIndexBackup);
		mpComposition->AddSegment(mpSegment, mSegmentIndexBackup);
		mpSegment->show();
		mpSegmentIndicator->show();
		mIsRedone = true;
		mpComposition->layout()->activate();
		mpTimeline->layout()->activate();
	}
}

MoveSegmentCommand::MoveSegmentCommand(GraphicsWidgetSegment *pSegment, GraphicsWidgetSegmentIndicator *pSegmentIndicator,
																			 int NewSegmentIndex, GraphicsWidgetComposition *pComposition, GraphicsWidgetTimeline *pTimeline,
																			 QUndoCommand *pParent /*= NULL*/) :
																			 QUndoCommand(pParent), mpSegment(pSegment), mpSegmentIndicator(pSegmentIndicator), mpComposition(pComposition), mpTimeline(pTimeline), mNewSegmentIndex(NewSegmentIndex), mOldSegmentIndex(-1) {

}

void MoveSegmentCommand::undo() {

	if(mOldSegmentIndex >= 0) {
		mpTimeline->MoveSegmentIndicator(mpSegmentIndicator, mOldSegmentIndex);
		mpComposition->MoveSegment(mpSegment, mOldSegmentIndex);
		mpComposition->layout()->activate();
	}
}

void MoveSegmentCommand::redo() {

	mOldSegmentIndex = mpComposition->GetSegmentIndex(mpSegment);
	if(mNewSegmentIndex >= 0) {
		mpTimeline->MoveSegmentIndicator(mpSegmentIndicator, mNewSegmentIndex);
		mpComposition->MoveSegment(mpSegment, mNewSegmentIndex);
		mpComposition->layout()->activate();
	}
}

RemoveSegmentCommand::RemoveSegmentCommand(GraphicsWidgetSegment *pSegment, GraphicsWidgetSegmentIndicator *pSegmentIndicator,
																					 GraphicsWidgetComposition *pComposition, GraphicsWidgetTimeline *pTimeline,
																					 QUndoCommand *pParent /*= NULL*/) :
																					 QUndoCommand(pParent), mpSegment(pSegment), mpSegmentIndicator(pSegmentIndicator), mpComposition(pComposition), mpTimeline(pTimeline), mSegmentIndexBackup(-1),
																					 mIsRedone(false) {

}

RemoveSegmentCommand::~RemoveSegmentCommand() {

	if(mIsRedone == true && mpSegment) mpSegment->deleteLater(); // TODO: Is this safe.
}

void RemoveSegmentCommand::undo() {

	if(mSegmentIndexBackup >= 0) {
		QObject::connect(mpSegment, SIGNAL(DurationChanged(const Duration&)), mpSegmentIndicator, SLOT(rSegmentDurationChange(const Duration&)));
		QObject::connect(mpSegmentIndicator, SIGNAL(HoverActive(bool)), mpSegment, SLOT(rSegmentIndicatorHoverActive(bool)));
		mpTimeline->AddSegmentIndicator(mpSegmentIndicator, mSegmentIndexBackup);
		mpComposition->AddSegment(mpSegment, mSegmentIndexBackup);
		mpSegment->show();
		mpSegmentIndicator->show();
		mIsRedone = false;
		mpComposition->layout()->activate();
		mpTimeline->layout()->activate();
	}
}

void RemoveSegmentCommand::redo() {

	mSegmentIndexBackup = mpComposition->GetSegmentIndex(mpSegment);
	if(mSegmentIndexBackup >= 0) {
		QObject::disconnect(mpSegment, SIGNAL(DurationChanged(const Duration&)), mpSegmentIndicator, SLOT(rSegmentDurationChange(const Duration&)));
		QObject::disconnect(mpSegmentIndicator, SIGNAL(HoverActive(bool)), mpSegment, SLOT(rSegmentIndicatorHoverActive(bool)));
		mpSegmentIndicator->hide();
		mpSegment->hide();
		mpComposition->RemoveSegment(mpSegment);
		mpTimeline->RemoveSegmentIndicator(mpSegmentIndicator);
		mpSegment->setParentItem(NULL); // Gets top level item. Deleted by scene.
		mpSegmentIndicator->setParentItem(NULL);
		mIsRedone = true;
		mpComposition->layout()->activate();
		mpTimeline->layout()->activate();
	}
}

MoveMarkerCommand::MoveMarkerCommand(GraphicsObjectVerticalIndicator *pMarker, const QPointF &rNewPosition, const QPointF &rOldPosition, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpMarker(pMarker), mOldPosition(rOldPosition), mNewPosition(rNewPosition) {

}

void MoveMarkerCommand::undo() {

	mpMarker->setPos(mOldPosition);
}

void MoveMarkerCommand::redo() {

	mpMarker->setPos(mNewPosition);
}

AddMarkerCommand::AddMarkerCommand(GraphicsObjectVerticalIndicator *pMarker, const QPointF &rPosition, GraphicsWidgetMarkerResource *pResource, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpMarker(pMarker), mpResource(pResource), mPosition(rPosition), mIsRedone(false) {

}

AddMarkerCommand::~AddMarkerCommand() {

	if(mIsRedone == false && mpMarker) mpMarker->deleteLater(); // TODO: Is this safe.
}

void AddMarkerCommand::undo() {

	mpMarker->hide();
	mpMarker->setParentItem(NULL); // Gets top level item. Deleted by scene.
	mIsRedone = false;
}

void AddMarkerCommand::redo() {

	mpMarker->setParentItem(mpResource);
	mpMarker->setPos(mPosition);
	mpMarker->show();
	mIsRedone = true;
}

RemoveMarkerCommand::RemoveMarkerCommand(GraphicsObjectVerticalIndicator *pMarker, GraphicsWidgetMarkerResource *pResource, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpMarker(pMarker), mpResource(pResource), mPosition(pMarker->pos()), mIsRedone(false) {

}

RemoveMarkerCommand::~RemoveMarkerCommand() {

	if(mIsRedone == true && mpMarker) mpMarker->deleteLater(); // TODO: Is this safe.
}

void RemoveMarkerCommand::undo() {

	mpMarker->setParentItem(mpResource);
	mpMarker->setPos(mPosition);
	mpMarker->show();
	mIsRedone = false;
}

void RemoveMarkerCommand::redo() {

	mPosition = mpMarker->pos();
	mpMarker->hide();
	mpMarker->setParentItem(NULL); // Gets top level item. Deleted by scene.
	mIsRedone = true;
}

AddTrackDetailsCommand::AddTrackDetailsCommand(AbstractWidgetTrackDetails *pTrackDetails, WidgetComposition *pComposition, int trackIndex, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpTrackDetails(pTrackDetails), mpComposition(pComposition), mIndex(trackIndex), mIsRedone(false) {

}

AddTrackDetailsCommand::~AddTrackDetailsCommand() {

	if(mIsRedone == false && mpTrackDetails) mpTrackDetails->deleteLater(); // TODO: Is this safe.
}

void AddTrackDetailsCommand::undo() {

	if(mIndex >= 0) {
		mpTrackDetails->hide();
		mpComposition->RemoveTrackDetail(mpTrackDetails);
		mIsRedone = false;
	}
}

void AddTrackDetailsCommand::redo() {

	if(mIndex >= 0) {
		mpComposition->AddTrackDetail(mpTrackDetails, mIndex);
		mpTrackDetails->show();
		mIsRedone = true;
	}
}

RemoveTrackDetailsCommand::RemoveTrackDetailsCommand(AbstractWidgetTrackDetails *pTrackDetails, WidgetComposition *pComposition, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpTrackDetails(pTrackDetails), mpComposition(pComposition), mIndex(-1), mIsRedone(false) {

}

RemoveTrackDetailsCommand::~RemoveTrackDetailsCommand() {

	if(mIsRedone == true && mpTrackDetails) mpTrackDetails->deleteLater(); // TODO: Is this safe.
}

void RemoveTrackDetailsCommand::undo() {

	if(mIndex >= 0) {
		mpComposition->AddTrackDetail(mpTrackDetails, mIndex);
		mpTrackDetails->show();
		mIsRedone = false;
	}
}

void RemoveTrackDetailsCommand::redo() {

	mIndex = mpComposition->GetTrackDetailIndex(mpTrackDetails);
	if(mIndex >= 0) {
		mpTrackDetails->hide();
		mpComposition->RemoveTrackDetail(mpTrackDetails);
		mIsRedone = true;
	}
}

EditCommand::EditCommand(AbstractGraphicsWidgetResource *pOriginResource, const Duration &rOldOriginResourceSourceDuration, const Duration &rNewOriginResourceSourceDuration, 
												 AbstractGraphicsWidgetResource *pCloneResource, const Duration &rOldCloneResourceSourceDuration, const Duration &rNewCloneResourceSourceDuration, 
												 const Duration &rOldCloneResourceEntryPoint, const Duration &rNewCloneResourceEntryPoint, int clonedResourceSourceIndex, GraphicsWidgetSequence *pTargetSequence,
												 QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent) {

	SetSourceDurationCommand *p_set_source_dur = new SetSourceDurationCommand(pOriginResource, rOldOriginResourceSourceDuration, rNewOriginResourceSourceDuration, this);
	SetEntryPointCommand *p_entry_command = new SetEntryPointCommand(pCloneResource, rOldCloneResourceEntryPoint, rNewCloneResourceEntryPoint, this);
	SetSourceDurationCommand *p_set_source_dur_two = new SetSourceDurationCommand(pCloneResource, rOldCloneResourceSourceDuration, rNewCloneResourceSourceDuration, this);
	AddResourceCommand *p_add_resource_command = new AddResourceCommand(pCloneResource, clonedResourceSourceIndex, pTargetSequence, this);
}

MoveResourceCommand::MoveResourceCommand(AbstractGraphicsWidgetResource *pResource, int newResourceIndex, GraphicsWidgetSequence *pNewSequence, GraphicsWidgetSequence *pOldSequence, QUndoCommand *pParent /*= NULL*/) :
QUndoCommand(pParent), mpResource(pResource), mpOldSequence(pOldSequence), mpNewSequence(pNewSequence), mNewResourceIndex(newResourceIndex), mOldResourceIndex(-1) {

}

void MoveResourceCommand::undo() {

	if(mOldResourceIndex >= 0) {
		mpResource->hide();
		mpNewSequence->RemoveResource(mpResource);
		mpResource->setParentItem(NULL);
		mpOldSequence->AddResource(mpResource, mOldResourceIndex);
		mpResource->show();
		mpOldSequence->layout()->activate();
		mpNewSequence->layout()->activate();
	}
}

void MoveResourceCommand::redo() {

	mOldResourceIndex = mpOldSequence->GetResourceIndex(mpResource);
	if(mNewResourceIndex >= 0) {
		mpResource->hide();
		mpOldSequence->RemoveResource(mpResource);
		mpResource->setParentItem(NULL);
		mpNewSequence->AddResource(mpResource, mNewResourceIndex);
		mpResource->show();
		mpOldSequence->layout()->activate();
		mpNewSequence->layout()->activate();
	}
}
