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
        DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(Xuni("Core"));

        if (impl != NULL)
        {
            try
            {
                DOMDocument* doc = impl->createDocument(
                		Xuni("http://www.w3.org/ns/ttml"),					// root element namespace URI.
                               Xuni("tt"),									// root element name
                               0);										// document type object (DTD).

                DOMElement* rootElem = doc->getDocumentElement();

                rootElem->setAttribute(Xuni("xmlns:ittp"), Xuni("http://www.w3.org/ns/ttml/profile/imsc1#parameter"));
                rootElem->setAttribute(Xuni("xmlns:ittm"), Xuni("http://www.w3.org/ns/ttml/profile/imsc1#metadata"));
                rootElem->setAttribute(Xuni("xmlns:ttm"), IMSC1_NS_TTM);
                rootElem->setAttribute(Xuni("xmlns:ttp"), IMSC1_NS_TTP);

                rootElem->setAttribute(Xuni("xml:lang"), Xuni("en"));
                rootElem->setAttribute(Xuni("ttp:profile"), Xuni("http://www.w3.org/ns/ttml/profile/imsc1/text"));
                //WR
                bool isFractional = (mEditRate.GetQuotient() != round(mEditRate.GetQuotient()));
                rootElem->setAttribute(Xuni("ttp:frameRate"), Xuni(mEditRate.GetRoundedName().toLatin1().data()));
                if (isFractional) {
                	rootElem->setAttribute(Xuni("ttp:frameRateMultiplier"), Xuni("1000 1001"));
                }
                //WR

                // <head>
                DOMElement* headElem = doc->createElementNS(IMSC1_NS_TT, Xuni("head"));
                rootElem->appendChild(headElem);

                DOMElement* metaElem = doc->createElementNS(IMSC1_NS_TT, Xuni("metadata"));
                headElem->appendChild(metaElem);

                DOMElement* titleElem = doc->createElementNS(IMSC1_NS_TTM, Xuni("title"));
                metaElem->appendChild(titleElem);
                DOMText* titleDataVal = doc->createTextNode(Xuni("Empty imsc1 document"));
                titleElem->appendChild(titleDataVal);

                DOMElement* descElem = doc->createElementNS(IMSC1_NS_TTM, Xuni("desc"));
                metaElem->appendChild(descElem);
                DOMText* descDataVal = doc->createTextNode(Xuni("this document can be used to fill gaps in the IMF CPL"));
                descElem->appendChild(descDataVal);

                // <body>
                DOMElement* bodyElem = doc->createElementNS(IMSC1_NS_TT, Xuni("body"));
                rootElem->appendChild(bodyElem);

                DOMElement* divElem = doc->createElementNS(IMSC1_NS_TT, Xuni("div"));
                bodyElem->appendChild(divElem);

                divElem->setAttribute(Xuni("begin"), Xuni("0f"));
                divElem->setAttribute(Xuni("end"), Xuni(mpDur->toStdString().c_str()));

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

	DOMImplementation *implementation = DOMImplementationRegistry::getDOMImplementation(Xuni("LS"));
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

