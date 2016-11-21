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

#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QtCore>
#include <QDebug>
#include <cstring>
#include <QMessageBox>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/dom/DOMLSOutput.hpp>

//WR
#include "ImfCommon.h"
//WR
using namespace xercesc;

//!this class helps to convert char* to XMLString by using X(char*) instead of XMLString::transcode(char*).
class XStr
{
public :
    XStr(const char* const toTranscode) { fUnicodeForm = XMLString::transcode(toTranscode); }
    ~XStr() { XMLString::release(&fUnicodeForm); }
    const XMLCh* unicodeForm() const { return fUnicodeForm; }

private :
    XMLCh*   fUnicodeForm;
};
#define X(str) XStr(str).unicodeForm()


class EmptyTimedTextGenerator : public QObject {

	Q_OBJECT

public:
	EmptyTimedTextGenerator();
	EmptyTimedTextGenerator(QString filePath, QString dur, EditRate rEditRate);
	virtual ~EmptyTimedTextGenerator();

	Q_SIGNALS:
	void FileComplete(const QStringList &files);

private:
	Q_DISABLE_COPY(EmptyTimedTextGenerator);

	int GenerateEmptyXml();
	void OutputXML(DOMDocument* pmyDOMDocument);

	QString *mpFilePath;
	QString *mpDur;
	EditRate mEditRate;
};

