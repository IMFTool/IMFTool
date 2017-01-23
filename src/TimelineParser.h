#pragma once
#include <QObject>
#include <QDebug>
#include "GraphicsCommon.h"
#include "ImfPackage.h"
#include <QVector>
#include "TTMLParser.h" 

class TimelineParser : public QObject {
	Q_OBJECT
public:
	TimelineParser() {};
	~TimelineParser() {};

	GraphicsWidgetComposition *composition;
	QVector<PlayListElement> *playlist; // (k) make private?
	QVector<TTMLtimelineSegment> *ttmls; // (K) make private?

public slots:
	void run();
signals:
	void PlaylistFinished();
};