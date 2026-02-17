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
#include "ImfCommon.h"
#include "ImfPackageCommon.h"
#include <QUndoCommand>


class AbstractGraphicsWidgetResource;
class GraphicsWidgetAudioResource;
class GraphicsWidgetMarkerResource;
class GraphicsObjectVerticalIndicator;
class GraphicsWidgetSegmentIndicator;
class GraphicsWidgetSequence;
class GraphicsWidgetSegment;
class GraphicsWidgetComposition;
class GraphicsWidgetTimeline;
class AbstractWidgetTrackDetails;
class WidgetComposition;

class SetEntryPointCommand : public QUndoCommand {

public:
	SetEntryPointCommand(AbstractGraphicsWidgetResource *pResource, const Duration &rOldEntryPoint, const Duration &rNewEntryPoint, QUndoCommand *pParent = NULL);
	virtual ~SetEntryPointCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(SetEntryPointCommand);
	AbstractGraphicsWidgetResource *mpResource;
	Duration mOldEntryPoint;
	Duration mNewEntryPoint;
};


class SetEntryPointSamplesCommand : public QUndoCommand {

public:
	SetEntryPointSamplesCommand(GraphicsWidgetAudioResource *pResource, const Duration &rOldEntryPoint, const Duration &rNewEntryPoint, QUndoCommand *pParent = NULL);
	virtual ~SetEntryPointSamplesCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(SetEntryPointSamplesCommand);
	GraphicsWidgetAudioResource *mpResource;
	Duration mOldEntryPoint;
	Duration mNewEntryPoint;
};


class SetSourceDurationCommand : public QUndoCommand {

public:
	SetSourceDurationCommand(AbstractGraphicsWidgetResource *pResource, const Duration &rOldSourceDuration, const Duration &rNewSourceDuration, QUndoCommand *pParent = NULL);
	virtual ~SetSourceDurationCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(SetSourceDurationCommand);
	AbstractGraphicsWidgetResource *mpResource;
	Duration mOldSourceDuration;
	Duration mNewSourceDuration;
};


class SetSourceDurationSamplesCommand : public QUndoCommand {

public:
	SetSourceDurationSamplesCommand(GraphicsWidgetAudioResource *pResource, const Duration &rOldSourceDuration, const Duration &rNewSourceDuration, QUndoCommand *pParent = NULL);
	virtual ~SetSourceDurationSamplesCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(SetSourceDurationSamplesCommand);
	GraphicsWidgetAudioResource *mpResource;
	Duration mOldSourceDuration;
	Duration mNewSourceDuration;
};


class SetIntrinsicDurationCommand : public QUndoCommand {

public:
	SetIntrinsicDurationCommand(GraphicsWidgetMarkerResource *pResource, const Duration &rOldIntrinsicDuration, const Duration &rNewIntrinsicDuration, QUndoCommand *pParent = NULL);
	virtual ~SetIntrinsicDurationCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(SetIntrinsicDurationCommand);
	GraphicsWidgetMarkerResource *mpResource;
	Duration mOldIntrinsicDuration;
	Duration mNewIntrinsicDuration;
};


class AddResourceCommand : public QUndoCommand {

public:
	AddResourceCommand(AbstractGraphicsWidgetResource *pResource, int resourceIndex, GraphicsWidgetSequence *pSequence, QUndoCommand *pParent = NULL);
	virtual ~AddResourceCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddResourceCommand);
	AbstractGraphicsWidgetResource *mpResource;
	GraphicsWidgetSequence *mpSequence;
	int mResourceIndexBackup;
	bool mIsRedone;
};


class MoveResourceCommand : public QUndoCommand {

public:
	MoveResourceCommand(AbstractGraphicsWidgetResource *pResource, int newResourceIndex, GraphicsWidgetSequence *pNewSequence, GraphicsWidgetSequence *pOldSequence, QUndoCommand *pParent = NULL);
	virtual ~MoveResourceCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(MoveResourceCommand);
	AbstractGraphicsWidgetResource *mpResource;
	GraphicsWidgetSequence *mpOldSequence;
	GraphicsWidgetSequence *mpNewSequence;
	int mNewResourceIndex;
	int mOldResourceIndex;
};


class RemoveResourceCommand : public QUndoCommand {

public:
	RemoveResourceCommand(AbstractGraphicsWidgetResource *pResource, GraphicsWidgetSequence *pSequence, QUndoCommand *pParent = NULL);
	virtual ~RemoveResourceCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveResourceCommand);
	AbstractGraphicsWidgetResource *mpResource;
	GraphicsWidgetSequence *mpSequence;
	int mResourceIndexBackup;
	bool mIsRedone;
};


class AddSequenceCommand : public QUndoCommand {

public:
	AddSequenceCommand(GraphicsWidgetSequence *pSequence, int sequenceIndex, GraphicsWidgetSegment *pSegment, QUndoCommand *pParent = NULL);
	virtual ~AddSequenceCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddSequenceCommand);
	GraphicsWidgetSequence *mpSequence;
	GraphicsWidgetSegment *mpSegment;
	int mSequenceIndexBackup;
	bool mIsRedone;
};


class RemoveSequenceCommand : public QUndoCommand {

public:
	RemoveSequenceCommand(GraphicsWidgetSequence *pSequence, GraphicsWidgetSegment *pSegment, QUndoCommand *pParent = NULL);
	virtual ~RemoveSequenceCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveSequenceCommand);
	GraphicsWidgetSequence *mpSequence;
	GraphicsWidgetSegment *mpSegment;
	int mSequenceIndexBackup;
	bool mIsRedone;
};


class AddSegmentCommand : public QUndoCommand {

public:
	AddSegmentCommand(GraphicsWidgetSegment *pSegment, GraphicsWidgetSegmentIndicator *pSegmentIndicator, 
										int segmentIndex, GraphicsWidgetComposition *pComposition, GraphicsWidgetTimeline *pTimeline, 
										QUndoCommand *pParent = NULL);
	virtual ~AddSegmentCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddSegmentCommand);
	GraphicsWidgetSegment *mpSegment;
	GraphicsWidgetSegmentIndicator *mpSegmentIndicator;
	GraphicsWidgetComposition *mpComposition;
	GraphicsWidgetTimeline *mpTimeline;
	int mSegmentIndexBackup;
	bool mIsRedone;
};


class MoveSegmentCommand : public QUndoCommand {

public:
	MoveSegmentCommand(GraphicsWidgetSegment *pSegment, GraphicsWidgetSegmentIndicator *pSegmentIndicator,
										int NewSegmentIndex, GraphicsWidgetComposition *pComposition, GraphicsWidgetTimeline *pTimeline,
										QUndoCommand *pParent = NULL);
	virtual ~MoveSegmentCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(MoveSegmentCommand);
	GraphicsWidgetSegment *mpSegment;
	GraphicsWidgetSegmentIndicator *mpSegmentIndicator;
	GraphicsWidgetComposition *mpComposition;
	GraphicsWidgetTimeline *mpTimeline;
	int mNewSegmentIndex;
	int mOldSegmentIndex;
};


class RemoveSegmentCommand : public QUndoCommand {

public:
	RemoveSegmentCommand(GraphicsWidgetSegment *pSegment, GraphicsWidgetSegmentIndicator *pSegmentIndicator,
											 GraphicsWidgetComposition *pComposition, GraphicsWidgetTimeline *pTimeline,
											 QUndoCommand *pParent = NULL);
	virtual ~RemoveSegmentCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveSegmentCommand);
	GraphicsWidgetSegment *mpSegment;
	GraphicsWidgetSegmentIndicator *mpSegmentIndicator;
	GraphicsWidgetComposition *mpComposition;
	GraphicsWidgetTimeline *mpTimeline;
	int mSegmentIndexBackup;
	bool mIsRedone;
};


class MoveMarkerCommand : public QUndoCommand {

public:
	MoveMarkerCommand(GraphicsObjectVerticalIndicator *pMarker, const QPointF &rNewPosition, const QPointF &rOldPosition, QUndoCommand *pParent = NULL);
	virtual ~MoveMarkerCommand() {}
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(MoveMarkerCommand);
	GraphicsObjectVerticalIndicator *mpMarker;
	QPointF mOldPosition;
	QPointF mNewPosition;
};


class AddMarkerCommand : public QUndoCommand {

public:
	AddMarkerCommand(GraphicsObjectVerticalIndicator *pMarker, const QPointF &rPosition, GraphicsWidgetMarkerResource *pResource, QUndoCommand *pParent = NULL);
	virtual ~AddMarkerCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddMarkerCommand);
	GraphicsObjectVerticalIndicator *mpMarker;
	GraphicsWidgetMarkerResource *mpResource;
	QPointF mPosition;
	bool mIsRedone;
};


class RemoveMarkerCommand : public QUndoCommand {

public:
	RemoveMarkerCommand(GraphicsObjectVerticalIndicator *pMarker, GraphicsWidgetMarkerResource *pResource, QUndoCommand *pParent = NULL);
	virtual ~RemoveMarkerCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveMarkerCommand);
	GraphicsObjectVerticalIndicator *mpMarker;
	GraphicsWidgetMarkerResource *mpResource;
	QPointF mPosition;
	bool mIsRedone;
};


class EditMarkerAnnotationCommand : public QUndoCommand {

public:
	EditMarkerAnnotationCommand(GraphicsWidgetMarkerResource *pResource, QPointF &rPos, int &rIndex, UserText rOldAnnotation, UserText rNewAnnotation, QUndoCommand *pParent = NULL);
	virtual ~EditMarkerAnnotationCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(EditMarkerAnnotationCommand);
	GraphicsWidgetMarkerResource *mpResource;
	QPointF mPos;
	int mIndex;
	UserText mOldAnnotation;
	UserText mNewAnnotation;
	bool mIsRedone;
};


class AddTrackDetailsCommand : public QUndoCommand {

public:
	AddTrackDetailsCommand(AbstractWidgetTrackDetails *pTrackDetails, WidgetComposition *pComposition, int trackIndex, QUndoCommand *pParent = NULL);
	virtual ~AddTrackDetailsCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(AddTrackDetailsCommand);
	AbstractWidgetTrackDetails *mpTrackDetails;
	WidgetComposition *mpComposition;
	int mIndex;
	bool mIsRedone;
};


class RemoveTrackDetailsCommand : public QUndoCommand {

public:
	RemoveTrackDetailsCommand(AbstractWidgetTrackDetails *pTrackDetails, WidgetComposition *pComposition, QUndoCommand *pParent = NULL);
	virtual ~RemoveTrackDetailsCommand();
	virtual void undo();
	//! Called once when pushed on Undo Stack.
	virtual void redo();

private:
	Q_DISABLE_COPY(RemoveTrackDetailsCommand);
	AbstractWidgetTrackDetails *mpTrackDetails;
	WidgetComposition *mpComposition;
	int mIndex;
	bool mIsRedone;
};


class EditCommand : public QUndoCommand {

public:
	EditCommand(AbstractGraphicsWidgetResource *pOriginResource, const Duration &rOldOriginResourceSourceDuration, const Duration &rNewOriginResourceSourceDuration,
							AbstractGraphicsWidgetResource *pCloneResource, const Duration &rOldCloneResourceSourceDuration, const Duration &rNewCloneResourceSourceDuration,
							const Duration &rOldCloneResourceEntryPoint, const Duration &rNewCloneResourceEntryPoint, 
							int clonedResourceSourceIndex, GraphicsWidgetSequence *pTargetSequence, QUndoCommand *pParent = NULL);
	virtual ~EditCommand() {}

private:
	Q_DISABLE_COPY(EditCommand);
};
