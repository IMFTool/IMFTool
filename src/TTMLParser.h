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
#include "ImfPackageCommon.h"
#include <QObject>
#include <QDebug>
#include <QFileInfo>
#include <QVector>
#include "Error.h"

class TTMLParser;
class elem;
class TTMLFns;

class TTMLFns {

public:
	float DurationExtractor(xercesc::DOMDocument *dom_doc, float fr, int tr);
	float GetElementDuration(xercesc::DOMElement* eleDom, float fr, int tr);
	float ConvertTimingQStringtoDouble(QString string_time, float fr, int tr);
};

class TTMLParser : public TTMLFns {
public:
	TTMLParser();
	~TTMLParser() {
		for (int i = 0; i < elems.length(); i++) {
			delete elems[i]; // delete elem instance
		}
	};

	QVector<elem*> elems;
	Error open(const QString &rSourceFile, TTMLtimelineSegment &ttml_segment, bool rIsWrapped);
	void parse(std::string xml);

	TTMLtimelineSegment *this_segment; // current timeline segment

	float seq_timing_total_offset = 0; // temp?
	int tickrate;
	float framerate;
	bool is_wrapped; // wrapped in .mxf or not?
	float timeline_in; // in-point on timeline
	float timeline_out; // out-point on timeline
	QDir baseDir; // dir where anc data is stored (relevant for non-wrapped .ttml)

	// resources
	QMap<Kumu::UUID, QImage> anc_resources;
	QMap<QString, QString> styles;
	QMap<QString, TTMLRegion> regions;
	QMap<QString, QString> cssAttr; // all known css values
	QImage error_image;
	TTMLRegion nullregion;

private:

	/*
	typedef struct {
		int type; // 0 : unknown, 1 : par, 2 : seq
		float last_offset;
		float offset;
		bool duration_set;
		float current_dur;
		float total_dur;
		float end;
		float end_set;
	} TTMLTiming;
	*/

	void getMetadata(xercesc::DOMDocument *rDiv);
	void readAncilleryData();

	void RloopElements(elem *el);
	
	
	
	QVector<QColor> region_colors;
	
	
	QString SourceFilePath;

	// ttml reader
	AS_02::TimedText::MXFReader reader;

	// metadata
	float extent[2];


	TTMLRegion parseRegion(xercesc::DOMNode *el);
	QVector<QString> attrs2Css(xercesc::DOMNode *node);
	
	void Save2File(std::string xml); // temp method (for testing purposes)
	void print2Console(const QVector<TTMLelem> &ttmls);
};


class elem : public TTMLFns {
public:
	elem(xercesc::DOMNode *rNode, TTMLParser *parser, elem *rParent);

	bool process();

	TTMLParser *parser;
	elem *parent;
	bool stop = false; // discontinue processing children

	// time Container
	int timeContainer = 0; // 0 : unknown, 1 : par, 2 : seq
	bool is_timed = false; // has timing attributes?

	// seq timing
	//float local_offset = 0; // offset within parent container

	float end = 0;
	bool end_set = false;

	float dur = 0;
	float dur_used = 0;
	bool dur_set = false;
	bool hasBegEndDur = false;

	// timing
	float timing_begin = 0;
	float timing_end = 0;
	float timing_dur = 0; // amount of time the element is visible

	// region
	QString regionName;
	QString css; // resolved attributes

	xercesc::DOMElement *el;
	xercesc::DOMNode *node;

private:

	QString serializeTT(xercesc::DOMElement *rEl);
	bool GetStartEndTimes();
	void GetTimeContainer();
	void GetRegion();
	void GetStyle();
	bool processTimedElement();
	xercesc::DOMElement* processTimedElementStyle(xercesc::DOMElement *rEl, bool HasTiming);
};
