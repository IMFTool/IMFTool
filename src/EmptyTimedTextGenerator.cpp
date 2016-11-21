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
#include "EmptyTimedTextGenerator.h"
#include <QFile>



EmptyTimedTextGenerator::EmptyTimedTextGenerator(QString filePath, QString dur, EditRate rEditRate){

	int error;
	mpFilePath = &filePath;
	mpDur = &dur;
	mEditRate = rEditRate;
	error = GenerateEmptyXml();
	if (error > 0)
		QFile::remove(mpFilePath->at(0));
}

EmptyTimedTextGenerator::~EmptyTimedTextGenerator(){
	qDebug("Terminated");
}

int EmptyTimedTextGenerator::GenerateEmptyXml()
{
    try
    {
        XMLPlatformUtils::Initialize();
    }
    catch(const XMLException& e)
    {
        char* message = XMLString::transcode(e.getMessage());
        qDebug() << "Error Message: " << message << "\n";
        XMLString::release(&message);
        return 1;
    }

    int error = 0;
    {
        DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));

        if (impl != NULL)
        {
            try
            {
                DOMDocument* doc = impl->createDocument(
                		X("http://www.w3.org/ns/ttml"),					// root element namespace URI.
                               X("tt"),									// root element name
                               0);										// document type object (DTD).

                DOMElement* rootElem = doc->getDocumentElement();

                rootElem->setAttribute(X("xmlns:ittp"), X("http://www.w3.org/ns/ttml/profile/imsc1#parameter"));
                rootElem->setAttribute(X("xmlns:ittm"), X("http://www.w3.org/ns/ttml/profile/imsc1#metadata"));
                rootElem->setAttribute(X("xmlns:ttm"), X("http://www.w3.org/ns/ttml#metadata"));
                rootElem->setAttribute(X("xmlns:ttp"), X("http://www.w3.org/ns/ttml#parameter"));

                rootElem->setAttribute(X("xml:lang"), X("en"));
                rootElem->setAttribute(X("ttp:profile"), X("http://www.w3.org/ns/ttml/profile/imsc1/text"));
                //WR
                bool isFractional = (mEditRate.GetQuotient() != round(mEditRate.GetQuotient()));
                rootElem->setAttribute(X("ttp:frameRate"), X(mEditRate.GetRoundedName().toLatin1().data()));
                if (isFractional) {
                	rootElem->setAttribute(X("ttp:frameRateMultiplier"), X("1000 1001"));
                }
                //WR

                // <head>
                DOMElement* headElem = doc->createElement(X("head"));
                rootElem->appendChild(headElem);

                DOMElement* metaElem = doc->createElement(X("metadata"));
                headElem->appendChild(metaElem);

                DOMElement* titleElem = doc->createElement(X("ttm:title"));
                metaElem->appendChild(titleElem);
                DOMText* titleDataVal = doc->createTextNode(X("Empty imsc1 document"));
                titleElem->appendChild(titleDataVal);

                DOMElement* descElem = doc->createElement(X("ttm:desc"));
                metaElem->appendChild(descElem);
                DOMText* descDataVal = doc->createTextNode(X("this document can be used to fill gaps in the IMF CPL"));
                descElem->appendChild(descDataVal);

                // <body>
                DOMElement* bodyElem = doc->createElement(X("body"));
                rootElem->appendChild(bodyElem);

                DOMElement* divElem = doc->createElement(X("div"));
                bodyElem->appendChild(divElem);

                divElem->setAttribute(X("begin"), X("0f"));
                divElem->setAttribute(X("end"), X(mpDur->toStdString().c_str()));

                OutputXML(doc);

                doc->release();
            }
            catch (const DOMException& e)
            {
                qDebug() << "DOMException code is:  " << e.code << endl;
                error = 2;
            }
            catch(const XMLException& e)
            {
                char* message = XMLString::transcode(e.getMessage());
                qDebug() << "Error Message: " << message << endl;
                XMLString::release(&message);
                return 1;
            }
            catch (...)
            {
            	qDebug() << "An error occurred creating the document" <<  endl;
                error = 3;
            }
       }
       else
       {
    	   qDebug() << "Requested implementation is not supported" <<  endl;
           error = 4;
       }
    }

    XMLPlatformUtils::Terminate();

    return error;
}

void EmptyTimedTextGenerator::OutputXML(DOMDocument* pmyDOMDocument)
{
	QTemporaryFile file;

	DOMImplementation *implementation = DOMImplementationRegistry::getDOMImplementation(X("LS"));
    DOMLSSerializer *serializer = ((DOMImplementationLS*)implementation)->createLSSerializer();

    if (serializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
        serializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

    serializer->setNewLine(XMLString::transcode("\r\n"));
    XMLCh *tempFilePath = XMLString::transcode(mpFilePath->toStdString().c_str());
    XMLFormatTarget *formatTarget = new LocalFileFormatTarget(tempFilePath);
    DOMLSOutput *output = ((DOMImplementationLS*)implementation)->createLSOutput();

    output->setByteStream(formatTarget);
    serializer->write(pmyDOMDocument, output);

    serializer->release();
    XMLString::release(&tempFilePath);
    delete formatTarget;
    output->release();
}

