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
#include "WizardEssenceDescriptor.h"
#include "global.h"
#include "ImfCommon.h"
#include "MetadataExtractor.h"
#include <QTextEdit>
#include <QVBoxLayout>
#include "EmptyTimedTextGenerator.h" // defines Xuni
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include <xercesc/dom/DOMLSOutput.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>


WizardEssenceDescriptor::WizardEssenceDescriptor(QWidget *pParent /*= NULL*/, QSharedPointer<AssetMxfTrack> rAsset /* = 0 */) :
QWizard(pParent) {

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Essence Descriptor view"));
	setWizardStyle(QWizard::ModernStyle);
	setStyleSheet("QWizard QPushButton {min-width: 60 px;}");
	mAsset = rAsset;
	if (mAsset == NULL) return;
	InitLayout();
}

QSize WizardEssenceDescriptor::sizeHint() const {

	return QSize(900, 600);
}

void WizardEssenceDescriptor::InitLayout() {

	WizardEssenceDescriptorPage *p_wizard_page = new WizardEssenceDescriptorPage(this, mAsset);
	mPageId = addPage(p_wizard_page);
	QList<QWizard::WizardButton> layout;
	layout << QWizard::Stretch << QWizard::CancelButton;
	setButtonLayout(layout);
}

WizardEssenceDescriptorPage::WizardEssenceDescriptorPage(QWidget *pParent /*= NULL*/, QSharedPointer<AssetMxfTrack> rAsset /* = 0 */) :
		mAsset(rAsset) {
	InitLayout();
};

void WizardEssenceDescriptorPage::InitLayout() {

	QVBoxLayout *vbox_layout = new QVBoxLayout(this);

	if(mAsset) {
		QTextEdit *essence_descriptor_view = new QTextEdit();
		essence_descriptor_view->setFontFamily("Courier New");
		essence_descriptor_view->setAttribute(Qt::WA_DeleteOnClose, true);

		const char* essence_descriptor;
		try {
			DOMImplementation *implementation = DOMImplementationRegistry::getDOMImplementation(Xuni("LS"));
			DOMLSSerializer *serializer = ((DOMImplementationLS*)implementation)->createLSSerializer();

			if (serializer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
				serializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

			serializer->setNewLine(XMLString::transcode("\n"));
			DOMLSOutput *pLSOutput = ((DOMImplementationLS*)implementation)->createLSOutput();
			XMLFormatTarget* pMemTarget = new MemBufFormatTarget();
			pLSOutput->setByteStream(pMemTarget);
			if (mAsset->GetEssenceDescriptor()->getDomDocument().getFirstChild())
				serializer->write(mAsset->GetEssenceDescriptor()->getDomDocument().getFirstChild(), pLSOutput);
			else return;
			int bytesLen = (int)((MemBufFormatTarget*)pMemTarget)->getLen();
			essence_descriptor = (char*)((MemBufFormatTarget*)pMemTarget)->getRawBuffer();
			essence_descriptor_view->setText(QString(essence_descriptor));
			essence_descriptor_view->setReadOnly(true);
		}
		catch (...) {
			  return;
		}

		vbox_layout->addWidget(essence_descriptor_view);
	}

};
