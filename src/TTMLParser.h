
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
#include <QDebug>
#include <QVector>
#include "Error.h"

class TTMLParser;
class elem;
class TTMLFns;

class TTMLFns {

protected:
	float ConvertTimingQStringtoDouble(QString string_time, float fr, int tr);
	QMap<QString, QString> mergeCss(QMap<QString, QString>, QMap<QString, QString>); // elements from qm1 are preferred over qm2!!!
	QString serializeCss(QMap<QString, QString>);
};

class TTMLParser : TTMLFns {

public:
	TTMLParser();
	~TTMLParser() {
		for (int i = 0; i < elems.length(); i++) {
			elems.remove(i);
		}
	};

	QVector<elem*> elems;
	Error open(const QString &rSourceFile, TTMLtimelineResource &ttml_segment, bool rIsWrapped);
	void parse(std::string xml);
	QPair<QString, QMap<QString, QString>> parseStyle(xercesc::DOMNode *node);

	TTMLtimelineResource *this_segment; // current timeline segment

	float seq_timing_total_offset = 0;
	int tickrate;
	float framerate;
	bool is_wrapped; // wrapped in .mxf or not?
	float timeline_in; // in-point on timeline
	float timeline_out; // out-point on timeline;
	QDir baseDir; // dir where anc data is stored (relevant for non-wrapped .ttml)

	// resources
	QMap<Kumu::UUID, QImage> anc_resources;
	QMap<QString, QMap<QString, QString>> styles;
	QMap<QString, TTMLRegion> regions;
	QMap<QString, QString> cssAttr{
		{ "tts:backgroundColor" , "background-color" },
		{ "tts:content" , "content" },
		{ "tts:color" , "color" },
		{ "tts:direction" , "direction" },
		{ "tts:fontFamily" , "font-family" },
		{ "tts:fontSize" , "font-size" },
		{ "tts:fontStyle" , "font-style" },
		{ "tts:fontWeight" , "font-weight" },
		{ "tts:lineHeight" , "line-height" },
		{ "tts:padding" , "padding" },
		{ "tts:textAlign" , "text-align" },
		{ "tts:textDecoration" , "text-decoration" },
		{ "tts:unicodeBidi" , "unicode-bidi" },
		{ "tts:opacity" , "opacity" }
	}; // all supported css values

	TTMLRegion nullregion;

private:

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
	QMap<QString, QString> CSS {
		{ "color" , "white" } // default for text-profile
	};

	QList<QString> tt_tags {"div", "p", "span"};

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
