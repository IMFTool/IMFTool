#pragma once
#include <QObject>
#include <QDebug>
#include "GraphicsCommon.h"
#include "ImfPackage.h"
#include <QMap>
#include <QVector>
#include "TTMLParser.h" 

class TimelineParser : public QObject {
	Q_OBJECT
public:
	TimelineParser() {};
	~TimelineParser() {};

	GraphicsWidgetComposition *composition;
	QVector<VideoResource> *playlist; // (k) make private?
	QVector<TTMLtimelineResource> *ttmls; // (K) make private?
	QMap<quint8, QUuid> *mMGASADMTracks; // A map of Track numbers and Track IDs

public slots:
	void run();
signals:
	void PlaylistFinished();
};
