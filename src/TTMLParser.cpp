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
#include "TTMLParser.h"
#include "AS_DCP_internal.h"
#include "AS_02.h"
#include "PCMParserList.h"
#include <KM_fileio.h>
#include <cmath>
#include <QtCore>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

using namespace xercesc;
//#define DEBUG_TTML


//!this class helps to convert char* to XMLString by using X(char*) instead of XMLString::transcode(char*).
class XStr
{
public:
	XStr(const char* const toTranscode) { 
		fUnicodeForm = XMLString::transcode(toTranscode); 
	}
	~XStr() { XMLString::release(&fUnicodeForm); }
	const XMLCh* unicodeForm() const { return fUnicodeForm; }

private:
	XMLCh*   fUnicodeForm;
};
#define X(str) XStr(str).unicodeForm()

elem::elem(xercesc::DOMNode *rNode, TTMLParser *rParser, elem *rParent) {

	parser = rParser;
	parent = rParent;
	node = rNode;
	el = dynamic_cast<DOMElement*>(rNode);
}

bool elem::process() {

	if (el) {

		// check if is metadata -> ignore
		if (XMLString::compareIString(XMLString::transcode(el->getTagName()), "metadata") == 0) {
			dur = 2;
			stop = true;
			return stop;
		}

		// check for region attribute
		GetRegion();

		// check for timing
		if (GetStartEndTimes()) {
			is_timed = true;
			stop = true;

			processTimedElement();
		}
		else { // no timing attributes!
			GetTimeContainer(); // check for timeContainer attribute
			GetStyle();
		}
	}

	return stop;
}

void elem::GetTimeContainer() {

	// check if parent has timing -> pass on values if this is the case
	if (parent && parent->is_timed) { 
		is_timed = true;
		timeContainer = parent->timeContainer;
	}

	// check for timing attributes
	QString timeContainerVal = XMLString::transcode(el->getAttribute(XMLString::transcode("timeContainer")));
	if (timeContainerVal.length() > 0) { // found timeContainer!

		is_timed = true;

		if (timeContainerVal == "seq") {
			timeContainer = 2;
		}
		else if (timeContainerVal == "par") {
			timeContainer = 1;
		}
	}

	// timing is either par or seq
	if (is_timed) {

		QString durVal = XMLString::transcode(el->getAttribute(XMLString::transcode("dur")));
		QString endVal = XMLString::transcode(el->getAttribute(XMLString::transcode("end")));

		if (durVal.length() > 0) { // dur is set!
			
			dur = ConvertTimingQStringtoDouble(durVal, parser->framerate, parser->tickrate);
			dur_set = true;

			if (parent && parent->dur_set) { // parent dur is set
				if (parent->dur_used < parent->dur) { // there is at least some duration left
					
					if (dur >= (parent->dur - parent->dur_used)) {
						dur = parent->dur - parent->dur_used; // use whatever is left over from parent dur
					}

					// check if parent is par or seq
					if (parent->timeContainer == 2) { // seq
						parent->dur_used += dur;
					} // else: parent is par

				}
				else {
					stop = true; // discontinue processing children
				}
			}
		}
		else if (parent && parent->dur_set) {
			dur_set = true;
			dur = 0;
		}

		if (endVal.length() > 0 && durVal.length() == 0) { // end is set -> use as dur!
			// use end only if dur is undefined?
			dur = ConvertTimingQStringtoDouble(endVal, parser->framerate, parser->tickrate);
			dur_set = true;
		}
	}
}

void elem::GetRegion() {

	if (parent && parent->regionName.length() > 0) { // parent element has region -> pass it on!
		regionName = parent->regionName;
#ifdef DEBUG_TTML
		qDebug() << "parent region found:" << regionName;
#endif
	}
	else { // check if current element has region
		QString regVal = XMLString::transcode(el->getAttribute(XMLString::transcode("region")));
		if (regVal.length() > 0) { // found region!
#ifdef DEBUG_TTML
			qDebug() << "region used" << regVal;
#endif
			regionName = regVal;
		}
		else if(is_timed) {
#ifdef DEBUG_TTML
			qDebug() << "no region found";
#endif
		}
	}
}

void elem::GetStyle() {

	// get style from parent
	if (parent && parent->CSS.count() > 0) {
		CSS = mergeCss(CSS, parent->CSS);
	}

	// look for style attribute
	QString styleVal = XMLString::transcode(el->getAttribute(XMLString::transcode("style")));
	if (!styleVal.isEmpty()) {
		CSS = mergeCss(CSS, parser->styles[styleVal]);
		el->removeAttribute(XMLString::transcode("style"));
	}

	// loop attributes
	if (el->hasAttributes()) { // loop them
		for (int z = 0; z < el->getAttributes()->getLength(); z++) {

			DOMAttr *attr = dynamic_cast<DOMAttr*>(el->getAttributes()->item(z));

			QString name = XMLString::transcode(attr->getNodeName());
			if (parser->cssAttr.contains(name)) { // known parameter!

				//if (css.count(parser->cssAttr[name]) > 0) { // css param already exists through style
				//	css.remove(QRegExp(QString("%1:[a-zA-Z0-9#]*;").arg(parser->cssAttr[name]))); // remove it
				//}
				CSS[parser->cssAttr[name]] = XMLString::transcode(attr->getNodeValue());
				//el->removeAttribute(attr->getNodeName()); // remove current attr
			}
		}
	}
}

bool elem::processTimedElement() {

	// is element between in- and -out point?
	if ((timing_begin < parser->timeline_in && timing_begin > parser->timeline_out)
		|| (timing_end < parser->timeline_in && timing_end > parser->timeline_out)) {
		return false;
	}

	// was specified parent-block duration exeeded?
	if (parent && parent->dur_set && (parent->dur_used + dur) > parent->dur) {
		return false;
	}

	// create new timed element
	TTMLelem ttml_timed;
	ttml_timed.beg = (timing_begin + parser->seq_timing_total_offset);
	ttml_timed.end = (timing_end + parser->seq_timing_total_offset);

	// add offset from parent (if available)
	if (parent) { 
		ttml_timed.beg += parent->dur_used;
		ttml_timed.end += parent->dur_used;
	}

	if (regionName.length() > 0 && is_timed) { // region is defined -> resolve it!

		TTMLRegion new_region;
		new_region = parser->regions[regionName]; // resolve region
		new_region.valid = true;
		ttml_timed.region = new_region;
	}

	QString image = XMLString::transcode(el->getAttribute(XMLString::transcode("smpte:backgroundImage")));
	if (!image.isEmpty()) {

		ttml_timed.type = 1; // set type : image

		if (parser->is_wrapped) {
			// create UUID from file name e.g. track5-frag0-sample1-subs4.png -> 0c209959-84e2-5a63-86ad-bd5406b068d1
			Kumu::UUID id = AS_02::TimedText::CreatePNGNameId(image.toStdString());
			if (!parser->anc_resources[id].isNull()) { // anc asset found!
				ttml_timed.bgImage = parser->anc_resources[id];
			}
			else { // no anc asset found -> use default
				ttml_timed.bgImage = QImage(":/ttml_bg_not_found.png"); // ERROR
				ttml_timed.error = true; // set error flag
			}
		}
		else {
			// look for asset in base directory
			QString dirCopyPath = QDir::currentPath();
			QDir::setCurrent(parser->baseDir.absolutePath());

			ttml_timed.bgImage = QImage(image);
			if (ttml_timed.bgImage.isNull()) {
				ttml_timed.bgImage = QImage(":/ttml_bg_not_found.png");
			}
			QDir::setCurrent(dirCopyPath); // reset dir change
		}
	}
	else {
		ttml_timed.type = 0; // set type : text
	}

	ttml_timed.text = serializeTT(processTimedElementStyle(el, true));

	// set processed style map in timed struct
	ttml_timed.CSS = CSS;

	// add timed item to list
	parser->this_segment->items.append(ttml_timed); 

	// add visible element duration to parent block duration if parent timing is seq
	if (parent && parent->timeContainer == 2) {
		parent->dur_used += dur;
	}

	return true;
}

DOMElement* elem::processTimedElementStyle(DOMElement *rEl, bool HasTiming) {

	// timed element: get styles from parent
	if (HasTiming) {

		// get style from region
		if (regionName.length() > 0) {
			CSS = mergeCss(CSS,parser->regions[regionName].CSS);
		}

		// get style from parent
		if (parent && parent->CSS.count() > 0) {
			CSS = mergeCss(CSS, parent->CSS);
		}
	}

	// look for style attribute
	QString style = XMLString::transcode(rEl->getAttribute(XMLString::transcode("style")));
	if (!style.isEmpty()) {
		CSS = mergeCss(parser->styles[style], CSS);
		rEl->removeAttribute(XMLString::transcode("style"));
	}

	// loop attributes
	if (rEl->hasAttributes()) { // loop them
		for (int z = 0; z < rEl->getAttributes()->getLength(); z++) {

			DOMAttr *attr = dynamic_cast<DOMAttr*>(rEl->getAttributes()->item(z));

			QString name = XMLString::transcode(attr->getNodeName());
			if (parser->cssAttr.contains(name)) { // known parameter!
				CSS[parser->cssAttr[name]] = XMLString::transcode(attr->getNodeValue());
			}
		}
	}

	if (CSS.count() > 0) { // set serialized CSS in timed element
		rEl->setAttribute(XMLString::transcode("style"), XMLString::transcode(serializeCss(CSS).toStdString().c_str()));
	}

	// loop children (if available...)
	if (rEl->getChildNodes()) {
		for (int i = 0; i < rEl->getChildNodes()->getLength(); i++) {
			DOMElement* child = dynamic_cast<DOMElement*>(rEl->getChildNodes()->item(i)->cloneNode(true));
			if (child) {
				DOMNode *processed_child = dynamic_cast<DOMNode*>(processTimedElementStyle(child, false));
				rEl->replaceChild(processed_child, rEl->getChildNodes()->item(i));
			}
		}
	}

	return rEl;
}

bool elem::GetStartEndTimes() {

	float duration = 0, end = 0, beg = 0, current_dur = 0;
	bool end_found = false, beg_found = false, dur_found = false;

	if (!el) return false;

	QString tc_string = XMLString::transcode(el->getAttribute(XMLString::transcode("timeContainer")));
	if (!tc_string.isEmpty()) {
		return false; // abort!
	}

	QString end_string = XMLString::transcode(el->getAttribute(XMLString::transcode("end")));
	if (!end_string.isEmpty()) {
		end = ConvertTimingQStringtoDouble(end_string, parser->framerate, parser->tickrate);
		end_found = true;
	}

	QString beg_string = XMLString::transcode(el->getAttribute(XMLString::transcode("begin")));
	if (!beg_string.isEmpty()) {
		beg = ConvertTimingQStringtoDouble(beg_string, parser->framerate, parser->tickrate);
		beg_found = true;
	}

	QString dur_string = XMLString::transcode(el->getAttribute(XMLString::transcode("dur")));
	if (!dur_string.isEmpty()) {
		current_dur = ConvertTimingQStringtoDouble(dur_string, parser->framerate, parser->tickrate);
		dur_found = true;
	}

	if (end_string.isEmpty()) {
		end = beg + current_dur;
	}

	dur = end;

	if (dur_found || (beg_found && end_found) || (beg_found && dur_found)) {
		hasBegEndDur = true;
		timing_begin = beg;
		timing_end = end;
		timing_dur = end - beg;

		// dur
		if (parent && (parent->dur_set || parent->end_set)) {
			//parent->dur_used += end; // add current duration to total duration used in the block
		}

		return true;
	}
	else if(dur_found || beg_found || end_found){ // invisible element
		hasBegEndDur = true;
		// dur
		if (parent && (parent->dur_set || parent->end_set)) {
			parent->dur_used += end; // add current duration to total duration used in the block
		}
	}

	return false;
}

QString elem::serializeTT(DOMElement *rEl) {

	DOMImplementation *implementation = DOMImplementationRegistry::getDOMImplementation(X("LS"));
	DOMLSSerializer *serializer = ((DOMImplementationLS*)implementation)->createLSSerializer();
	return QString(XMLString::transcode(serializer->writeToString(rEl)));
}


// --------------------------------------------------------------------------------------------

TTMLParser::TTMLParser() {
	
	nullregion.valid = false;
}

Error TTMLParser::open(const QString &rSourceFile, TTMLtimelineResource &ttml_segment, bool rIsWrapped) {

	Error error;
	
	this_segment = &ttml_segment;
	timeline_in = ttml_segment.in;
	timeline_out = ttml_segment.out;
	is_wrapped = rIsWrapped;
	baseDir = QFileInfo(rSourceFile).absoluteDir();
	SourceFilePath = rSourceFile;

	TimedText::FrameBuffer buff(2 * Kumu::Megabyte);
	AS_02::TimedText::TimedTextDescriptor TDesc;
	std::string XMLDoc;

	if (is_wrapped) {
		AS_02::Result_t result = reader.OpenRead(rSourceFile.toStdString()); // "D:/Master/Thesis/TTML/track5-frag2-sample1-subs0.mxf"
		if (ASDCP_SUCCESS(result)) {

			result = reader.ReadTimedTextResource(XMLDoc); // read TTML XML
			if (ASDCP_SUCCESS(result)) {
				readAncilleryData();
				parse(XMLDoc);
			}
		}
		else {
			error = Error(result);
			return error;
		}
	}
	else { // read no-yet-wrapped .ttml file
		parse(XMLDoc); // XMLDoc : empty
	}

	return error;
}


void TTMLParser::readAncilleryData() {

	// get ancillary data
	AS_02::TimedText::TimedTextDescriptor TDesc;
	AS_02::Result_t result = reader.FillTimedTextDescriptor(TDesc);
	TimedText::FrameBuffer buffer;
	buffer.Capacity(2 * Kumu::Megabyte);
	
	if (ASDCP_SUCCESS(result)) {
		AS_02::TimedText::ResourceList_t::const_iterator ri;

		for (ri = TDesc.ResourceList.begin(); ri != TDesc.ResourceList.end() && ASDCP_SUCCESS(result); ri++) {

			Kumu::UUID id;
			id.Set((*ri).ResourceID);

			result = reader.ReadAncillaryResource((*ri).ResourceID, buffer);

			if (ASDCP_SUCCESS(result)) {
#ifdef DEBUG_TTML
				qDebug() << "successfully read anc data";
#endif
				QPixmap pix;
				pix.loadFromData(buffer.RoData(), buffer.Size(), "png");
				anc_resources[id] = pix.toImage();
			}
			else {
#ifdef DEBUG_TTML
				qDebug() << "error reading anc";
#endif
			}
		}
	}
	else {
#ifdef DEBUG_TTML
		qDebug() << "no resource list found";
#endif
	}
}

void TTMLParser::parse(std::string xml) {

	// SETUP XERCES
	XercesDOMParser*   parser = NULL;
	ErrorHandler*      errorHandler = NULL;

	if (!parser)
	{
		XMLPlatformUtils::Initialize();
		parser = new XercesDOMParser();
	}
	
	if (is_wrapped) {
		xercesc::MemBufInputSource memBuffInput((const XMLByte*)xml.c_str(), xml.size(), "dummy");
		parser->parse(memBuffInput);
	}
	else {
		try {
			parser->parse(SourceFilePath.toLocal8Bit());
		}
		catch (...) {
#ifdef DEBUG_TTML
			qDebug() << "ERROR parsing" << SourceFilePath;
#endif
		}
	}
	
	DOMDocument *dom_doc = parser->getDocument();															

	// get metadata
	getMetadata(dom_doc);

	// set framerate & doc in TTMLtimelineSegment
	this_segment->frameRate = framerate;
	this_segment->doc = QString(xml.c_str());

	// loop styles
	DOMNodeList	*styleList = dom_doc->getElementsByTagName(XMLString::transcode("style"));
	for (int i = 0; i < styleList->getLength(); i++) {
		QPair<QString, QMap<QString, QString>> style = parseStyle(styleList->item(i));
		styles[style.first] = style.second; // e.g. ["white_8"] = "CSS"
#ifdef DEBUG_TTML
		qDebug() << "created style" << style.first;
#endif
	}
	
	// loop regions
	DOMNodeList	*regionList = dom_doc->getElementsByTagName(XMLString::transcode("region"));
	for (int i = 0; i < regionList->getLength(); i++) {
		TTMLRegion region = parseRegion(regionList->item(i));
		regions[ region.id ] = region; // e.g. ["region1"] = TTMLRegion
#ifdef DEBUG_TTML
		qDebug() << "created region" << region.id;
#endif
	}

	// check if body has styling information
	DOMNode *body_node = dom_doc->getElementsByTagName(XMLString::transcode("body"))->item(0);
	DOMElement *body_el = dynamic_cast<DOMElement*>(body_node);

	QString body_region;
	QMap<QString, QString> body_style;

	if (body_node->hasAttributes()) {
		for (int z = 0; z < body_node->getAttributes()->getLength(); z++) {

			DOMAttr *attr = dynamic_cast<DOMAttr*>(body_node->getAttributes()->item(z));
			QString name = XMLString::transcode(attr->getNodeName());
			if (cssAttr.contains(name)) { // known parameter!
				body_style[cssAttr[name]] = XMLString::transcode(attr->getNodeValue());
			}
		}

		// get style
		QString styleVal = XMLString::transcode(body_el->getAttribute(XMLString::transcode("style")));
		if (!styleVal.isEmpty()) { // found style
			body_style = mergeCss(body_style, styles[styleVal]);
		}

		// get region
		QString regVal = XMLString::transcode(body_el->getAttribute(XMLString::transcode("region")));
		if (regVal.length() > 0) { // found region!
			body_region = regVal;
		}
	}

	// loop all children of <body>
	DOMNodeList	*body = dom_doc->getElementsByTagName(XMLString::transcode("body"))->item(0)->getChildNodes();
	for (int i = 0; i < body->getLength(); i++) {
		
		elem *newEl = new elem(body->item(i), this, NULL); // pass current parser & parent element
		newEl->regionName = body_region;
		newEl->CSS = body_style;

		if (newEl->process() == false) { // continue processing children
			elems.append(newEl);
			RloopElements(newEl);
		}
		else {
			delete newEl; // delete instance
		}
	}

#ifdef DEBUG_TTML
	print2Console(this_segment->items);
#endif

	parser->~XercesDOMParser(); // delete DOM parser
}

TTMLRegion TTMLParser::parseRegion(DOMNode *node) {

	TTMLRegion region;

	DOMElement *el = dynamic_cast<DOMElement*>(node);

	// get id
	QString idVal = XMLString::transcode(el->getAttribute(XMLString::transcode("xml:id")));
	if (!idVal.isEmpty()) { // found id
		region.id = idVal;
		el->removeAttribute(XMLString::transcode("xml:id"));
	}

	// get style
	QString styleVal = XMLString::transcode(el->getAttribute(XMLString::transcode("style")));
	if (!styleVal.isEmpty()) { // found style
		region.CSS = mergeCss(region.CSS,styles[styleVal]);
		el->removeAttribute(XMLString::transcode("style"));
	}

	// get origin e.g. tts:origin='12.88% 0.417%'
	QString originVal = XMLString::transcode(el->getAttribute(XMLString::transcode("tts:origin")));
	if (!originVal.isEmpty()) { // found origin
		if (originVal.count("%") > 0) { // %
			originVal.replace("%", ""); // remove '%'
			QStringList values = originVal.split(" ");
			region.origin[0] = values[0].toFloat();
			region.origin[1] = values[1].toFloat();
		}
		else { // px
			originVal.replace("px", ""); // remove 'px'
			QStringList values = originVal.split(" ");
			region.origin[0] = (values[0].toFloat() / extent[0]) * 100;
			region.origin[1] = (values[1].toFloat() / extent[1]) * 100;
		}
		el->removeAttribute(XMLString::transcode("tts:origin")); // remove attr
	}

	// get extent e.g. tts:extent='580px 200px'
	QString extentVal = XMLString::transcode(el->getAttribute(XMLString::transcode("tts:extent")));
	if (!extentVal.isEmpty()) { // found extent
		if (extentVal.count("%") > 0) { // %
			extentVal.replace("%", ""); // remove '%'
			QStringList values = extentVal.split(" ");
			region.extent[0] = values[0].toFloat();
			region.extent[1] = values[1].toFloat();
		}
		else { // px
			extentVal.replace("px", ""); // remove 'px'
			QStringList values = extentVal.split(" ");
			region.extent[0] = (values[0].toFloat() / extent[0]) * 100;
			region.extent[1] = (values[1].toFloat() / extent[1]) * 100;
		}
		el->removeAttribute(XMLString::transcode("tts:extent")); // remove attr
	}

	// check when region is active
	QString showVal = XMLString::transcode(el->getAttribute(XMLString::transcode("tts:showBackground")));
	if (!showVal.isEmpty()) { // found showBackground
		if (showVal == "always") {
			region.alwaysActive = true;
		}
		else {
			region.alwaysActive = false;
		}
		el->removeAttribute(XMLString::transcode("tts:showBackground")); // remove attr
	}

	// loop remaining attributes
	if (el->hasAttributes()) { // loop them
		for (int z = 0; z < el->getAttributes()->getLength(); z++) {

			DOMAttr *attr = dynamic_cast<DOMAttr*>(el->getAttributes()->item(z));

			QString name = XMLString::transcode(attr->getNodeName());
			if (cssAttr.contains(name)) { // known parameter!
				region.CSS[cssAttr[name]] = XMLString::transcode(attr->getNodeValue());
			} // else : not supported parameter
		}
	}

	// check if region has a backround-color
	if (region.CSS.contains("background-color")) { // use it
		region.bgColor.setNamedColor(region.CSS["background-color"]); // orange, #ff8000, #ff6 ...
	}
	else { // default region color
		region.bgColor = QColor(120, 0, 244, 100); // default color
	}

	// check if region has opacity
	if (region.CSS.contains("opacity")) {
		region.bgColor.setAlpha((int)(region.CSS["opacity"].toFloat() * 255)); // use it
	}
	else { // default opacity (50%)
		region.bgColor.setAlpha(127);
	}

	return region;
}


QPair<QString, QMap<QString, QString>> TTMLParser::parseStyle(DOMNode *el) {

	QPair<QString, QMap<QString, QString>> result;
	
	// get attributes
	if (el->hasAttributes()) { // loop them
		for (int z = 0; z < el->getAttributes()->getLength(); z++) {

			DOMNode *attr = el->getAttributes()->item(z);

			QString name = XMLString::transcode(attr->getNodeName());
			if (name == "xml:id") {
				result.first = XMLString::transcode(attr->getNodeValue()); // ID
			}else if (cssAttr.contains(name)) { // known parameter!
				result.second[cssAttr[name]] = XMLString::transcode(attr->getNodeValue());
			}
		}
	}

	return result;
}

void TTMLParser::RloopElements(elem *el) {


	if (el->node) {

		DOMNodeList *children = el->node->getChildNodes();

		// recursively loop children
		for (int z = 0; z < children->getLength(); z++) {

			elem *newEl = new elem(children->item(z), this, el); // pass current parser & parent element
			if (newEl->process() == false) { // continue one step deeper
				elems.append(newEl);
				RloopElements(newEl);

				if (newEl->parent->timeContainer == 2 && newEl->hasBegEndDur == false) {
					seq_timing_total_offset += newEl->dur; // add block duration (if sequential) to total offset
				}
			}
			else {
				delete newEl; // destroy elem instance
			}
		}
	}
}

// Code by Denis Manthey
void TTMLParser::getMetadata(DOMDocument *rDom) {

	DOMNodeList *ttitem = rDom->getElementsByTagName(XMLString::transcode("tt"));
	//Check if <tt> is present
	if (ttitem->getLength() == 0) {
#ifdef DEBUG_TTML
		qDebug() << "ERROR - no tt element found!";
#endif
		return;
	}
	//write <tt> from DOMNodelist to DOMElement
	DOMElement* tteleDom = dynamic_cast<DOMElement*>(ttitem->item(0));

	//Frame Rate Multiplier Extractor
	QString mult = XMLString::transcode(tteleDom->getAttribute(XMLString::transcode("ttp:frameRateMultiplier")));
	float num = 1;
	float den = 1;
	if (!mult.isEmpty()) {
		num = mult.section(" ", 0, 0).toInt();
		den = mult.section(" ", 1, 1).toInt();
	}

	//Frame Rate Extractor
	int subFrameRate = 1;
	QString fr = XMLString::transcode(tteleDom->getAttribute(XMLString::transcode("ttp:frameRate")));
	int editrate = 30;					//editrate is for the metadata object (expects editrate*numerator, denominator)
	framerate = 30 * (num / den);		//framerate is for calculating the duration, we need the fractal editrate!
	if (!fr.isEmpty()) {
		framerate = fr.toFloat()*(num / den);
		editrate = fr.toInt();
	}

	//Tick Rate Extractor
	tickrate = 1; //TTML1 section 6.2.10
	QString tr = XMLString::transcode(tteleDom->getAttribute(XMLString::transcode("ttp:tickRate")));
	if (!tr.isEmpty())
		tickrate = tr.toInt();
	else if (!fr.isEmpty())  //TTML1 section 6.2.10
		tickrate = ceil(framerate * subFrameRate);


	//Duration Extractor
	float duration;
	duration = DurationExtractor(rDom, framerate, tickrate);
	if (duration == 0) {
#ifdef DEBUG_TTML
		qDebug() << "Unknown duration!";
#endif
	}

	// get extent (e.g. tts:extent='854px 480px')
	QString extentVal = XMLString::transcode(tteleDom->getAttribute(XMLString::transcode("tts:extent")));
	if (!extentVal.isEmpty()) {
		if (extentVal.count("px") > 0) {
			extentVal.replace("px", ""); // remove 'px'
			QStringList values = extentVal.split(" ");
			extent[0] = values[0].toFloat();
			extent[1] = values[1].toFloat();
		}
	}
}


void TTMLParser::print2Console(const QVector<TTMLelem> &ttmls) {

	qDebug() << "~~~~~~~~~~~~~~~~~~~~~~~~~ START printing TTML ~~~~~~~~~~~~~~~~~~~~~~~~~~~";
	
	for (int i = 0; i < ttmls.length(); i++) {

		if (ttmls[i].type == 0) { // text
			qDebug() << ttmls[i].beg << " -> " << ttmls[i].end << "TEXT:" << ttmls[i].text;
		}
		else if(ttmls[i].type == 1){ // image
			qDebug() << ttmls[i].beg << " -> " << ttmls[i].end << "IMAGE:" << ttmls[i].bgImage.byteCount();
		}
		else { // unknown
			qDebug() << "ERROR: unknown type";
		}
	}

	qDebug() << "~~~~~~~~~~~~~~~~~~~~~~~~~ END printing TTML ~~~~~~~~~~~~~~~~~~~~~~~~~~~";
}

// ###################################### functions

QString TTMLFns::serializeCss(QMap<QString, QString> map) {

	QString result;

	// iterate over map
	QMap<QString, QString>::iterator i;
	for (i = map.begin(); i != map.end(); ++i) {
		result.append(QString("%1:%2;").arg(i.key()).arg(i.value()));
	}

	return result;
}

QMap<QString, QString> TTMLFns::mergeCss(QMap<QString, QString> qm1, QMap<QString, QString> qm2) {
	// elements from qm1 are preferred over qm2!!!

	// iterate over qm2
	QMap<QString, QString>::iterator i;
	for (i = qm2.begin(); i != qm2.end(); ++i) {
		if (!qm1.contains(i.key())) {
			qm1[i.key()] = i.value();
		}
	}

	return qm1;
}

// Code by Denis Manthey
float TTMLFns::DurationExtractor(DOMDocument *dom_doc, float fr, int tr) {

	float eleduration = 0, divduration = 0, div2duration = 0, pduration = 0, p2duration = 0, spanduration = 0, duration = 0;

	DOMNodeList	*bodyitems = dom_doc->getElementsByTagName(XMLString::transcode("body"));
	DOMElement* bodyeleDom = dynamic_cast<DOMElement*>(bodyitems->item(0));
	QString bodytime = XMLString::transcode(bodyeleDom->getAttribute(XMLString::transcode("timeContainer")));  //bodytime: par/seq

																											   //---start with div childelements of body---
	DOMNodeList	*divitems = dom_doc->getElementsByTagName(XMLString::transcode("div"));
	divduration = 0;
	for (int i = 0; i < divitems->getLength(); i++) {

		DOMElement* diveleDom = dynamic_cast<DOMElement*>(divitems->item(i));

		QString comp = XMLString::transcode(diveleDom->getParentNode()->getNodeName());			//look for body-child div!
		if (comp != "div") {
			QString divtime = XMLString::transcode(diveleDom->getAttribute(XMLString::transcode("timeContainer")));	//divtime: par/seq
			eleduration = GetElementDuration(diveleDom, fr, tr);
			div2duration = 0;
			pduration = 0;

			if (eleduration == 0) {
				DOMNodeList *div2items = diveleDom->getElementsByTagName(XMLString::transcode("div"));
				div2duration = 0;
				for (int j = 0; j < div2items->getLength(); j++) {

					DOMElement* div2eleDom = dynamic_cast<DOMElement*>(div2items->item(j));
					QString div2time = XMLString::transcode(div2eleDom->getAttribute(XMLString::transcode("timeContainer"))); //div2time: par/seq

					eleduration = GetElementDuration(div2eleDom, fr, tr);
					p2duration = 0;

					if (eleduration == 0) {
						DOMNodeList *p2items = div2eleDom->getElementsByTagName(XMLString::transcode("p"));
						p2duration = 0;
						for (int k = 0; k < p2items->getLength(); k++) {

							DOMElement* p2eleDom = dynamic_cast<DOMElement*>(p2items->item(k));
							QString p2time = XMLString::transcode(p2eleDom->getAttribute(XMLString::transcode("timeContainer"))); //p2time: par/seq

							eleduration = GetElementDuration(p2eleDom, fr, tr);
							spanduration = 0;

							if (eleduration == 0) {
								DOMNodeList *spanitems = p2eleDom->getElementsByTagName(XMLString::transcode("span"));

								for (int l = 0; l < spanitems->getLength(); l++) {

									DOMElement* spaneleDom = dynamic_cast<DOMElement*>(spanitems->item(l));

									eleduration = GetElementDuration(spaneleDom, fr, tr);
									if (p2time == "seq")
										spanduration = spanduration + eleduration;
									else {
										if (eleduration > spanduration)
											spanduration = eleduration;
									}
								}
								eleduration = 0;
							}

							if (div2time == "seq")
								p2duration = p2duration + eleduration + spanduration;
							else {
								if (spanduration > p2duration)
									p2duration = spanduration;

								if (eleduration > p2duration)
									p2duration = eleduration;
							}
						}
						eleduration = 0;
					}

					if (divtime == "seq")
						div2duration = div2duration + eleduration + p2duration;
					else {
						if (p2duration > div2duration)
							div2duration = p2duration;

						if (eleduration > div2duration)
							div2duration = eleduration;
					}
				}
				eleduration = 0;

				DOMNodeList *pitems = diveleDom->getElementsByTagName(XMLString::transcode("p"));
				for (int m = 0; m < pitems->getLength(); m++) {
					DOMElement* peleDom = dynamic_cast<DOMElement*>(pitems->item(m));
					QString comp2 = XMLString::transcode(peleDom->getParentNode()->getParentNode()->getNodeName());
					if (comp2 == "body") {
						QString ptime = XMLString::transcode(peleDom->getAttribute(XMLString::transcode("timeContainer")));	//ptime: par/seq
						eleduration = GetElementDuration(peleDom, fr, tr);
						spanduration = 0;

						if (eleduration == 0) {
							DOMNodeList *spanitems = peleDom->getElementsByTagName(XMLString::transcode("span"));

							for (int n = 0; n < spanitems->getLength(); n++) {

								DOMElement* spaneleDom = dynamic_cast<DOMElement*>(spanitems->item(n));

								eleduration = GetElementDuration(spaneleDom, fr, tr);
								if (ptime == "seq")
									spanduration = spanduration + eleduration;
								else {
									if (eleduration > spanduration)
										spanduration = eleduration;
								}
							}
							eleduration = 0;
						}

						if (divtime == "seq")
							pduration = pduration + eleduration + spanduration;
						else {
							if (spanduration > pduration)
								pduration = spanduration;

							if (eleduration > pduration)
								pduration = eleduration;
						}
					}
					eleduration = 0;
				}

				if (divtime == "seq")
					divduration = divduration + pduration + div2duration;
				else {
					if (div2duration > pduration)
						divduration = div2duration;
					else
						divduration = pduration;
				}
			}

			if (bodytime == "seq")
				duration = duration + eleduration + divduration;
			else {
				if (divduration > eleduration)
					duration = divduration;

				else
					duration = eleduration;
			}
		}
	}
	return duration;
}

float TTMLFns::GetElementDuration(DOMElement* eleDom, float fr, int tr) {

	float duration = 0, end = 0, beg = 0, dur = 0;
	QString end_string = XMLString::transcode(eleDom->getAttribute(XMLString::transcode("end")));
	if (!end_string.isEmpty())
		end = ConvertTimingQStringtoDouble(end_string, fr, tr);

	QString beg_string = XMLString::transcode(eleDom->getAttribute(XMLString::transcode("begin")));
	if (!beg_string.isEmpty())
		beg = ConvertTimingQStringtoDouble(beg_string, fr, tr);

	QString dur_string = XMLString::transcode(eleDom->getAttribute(XMLString::transcode("dur")));
	if (!dur_string.isEmpty())
		dur = ConvertTimingQStringtoDouble(dur_string, fr, tr);

	if (!end_string.isEmpty())
		duration = end;
	else
		duration = beg + dur;

	if (dur == 0 && end == 0)
		duration = 0;

	return duration;
}

float TTMLFns::ConvertTimingQStringtoDouble(QString string_time, float fr, int tr) {

	float time, h, min, sec, msec;

	if (string_time.right(2) == "ms")
		time = string_time.remove(QChar('m'), Qt::CaseInsensitive).remove(QChar('s'), Qt::CaseInsensitive).toFloat() / 1000;

	else if (string_time.right(1) == "s")
		time = string_time.remove(QChar('s'), Qt::CaseInsensitive).toFloat();

	else if (string_time.right(1) == "m")
		time = string_time.remove(QChar('m'), Qt::CaseInsensitive).toFloat() * 60;

	else if (string_time.right(1) == "h")
		time = string_time.remove(QChar('h'), Qt::CaseInsensitive).toFloat() * 3600;

	else if (string_time.right(1) == "f")
		time = string_time.remove(QChar('f'), Qt::CaseInsensitive).toFloat() / fr;

	else if (string_time.right(1) == "t")
		time = string_time.remove(QChar('t'), Qt::CaseInsensitive).toFloat() / tr;

	else if (string_time.left(9).right(1) == ".") { // Time expression with fractions of seconds, e.g. 00:00:20.1
		h = string_time.left(2).toFloat();
		min = string_time.left(5).right(2).toFloat();
		sec = string_time.left(8).right(2).toFloat();
		msec = string_time.remove(0, 7).replace(0, 2, "0.").toFloat();
		time = (h * 60 * 60) + (min * 60) + sec + msec;
	}
	else {  // Time expression with frames e.g. 00:00:00:15
		h = string_time.left(2).toFloat();
		min = string_time.left(5).right(2).toFloat();
		sec = string_time.left(8).right(2).toFloat() + (string_time.remove(0, 9).toFloat() / fr);
		time = (h * 60 * 60) + (min * 60) + sec;
	}
	return time;
}

