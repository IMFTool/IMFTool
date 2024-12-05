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
#include "global.h"
#include "info.h"
#include "QtApplicationBase.h"
#include "MainWindow.h"
#include "KMQtLogSink.h"
#include "ImfPackage.h"
#include "MetadataExtractor.h"
#include "CustomProxyStyle.h"
#include "WizardResourceGenerator.h"
#include <QtWidgets/QApplication>
#include <QFile>
#include <QThread>
#include <QMessageBox>
#include <QTextStream>
#include <QProcessEnvironment>
#include <iostream>
#include <QCommandLineParser>


int main(int argc, char *argv[]) {

	QCommandLineParser parser;
	QCommandLineOption startupDirectoryOption(QStringList() << "i" << "imp-directory",
			QCoreApplication::translate("main", "Open IMP in <directory> upon startup."),
			QCoreApplication::translate("main", "directory"));
	parser.addOption(startupDirectoryOption);

	QCommandLineOption openOption(QStringList() << "a" << "open-all-cpls",
			QCoreApplication::translate("main", "Open all CPLs from <directory>, as provided by option \"--imp-directory\", in Timeline View."));
	parser.addOption(openOption);

	QtApplicationBase<QApplication>::setCmdParser(&parser);
	QtApplicationBase<QApplication> app(argc, argv);
	QApplication::setWindowIcon(QIcon(":/icon1.ico"));
	QApplication::setStyle(new CustomProxyStyle);

	QString startupDirectory = parser.value(startupDirectoryOption);
	bool openAllCpls = parser.isSet(openOption);

	// load qt style sheet
	QFile style_file(":/style/stylesheet.qss");
	bool success = style_file.open(QFile::ReadOnly | QIODevice::Text);
	if(success) {
		app.setStyleSheet(QString::fromUtf8(style_file.readAll()));
		style_file.close();
	}
	else {
		qWarning() << "Couldn't load stylesheet: " << style_file.fileName();
	}

	qInfo() << "asdcplib version:" << ASDCP::Version();

	// catch libasdcpmod debug messages
	Kumu::KMQtLogSink qt_kumu_log_sinc;
	Kumu::SetDefaultLogSink(&qt_kumu_log_sinc);

	//--- register Qt metatypes here ---
	qRegisterMetaType<SoundfieldGroup>("SoundfieldGroup");
	qRegisterMetaType<Metadata>("Metadata");
	qRegisterMetaType<EditRate>("EditRate");
	qRegisterMetaType<Timecode>("Timecode");
	qRegisterMetaType<Duration>("Duration");
	qRegisterMetaType<WizardResourceGenerator::eMode>("WizardResourceGenerator::eMode");

	xercesc::XMLPlatformUtils::Initialize();

	MainWindow window;
	if (!startupDirectory.isEmpty()) {
		qDebug() << "Opening IMP at: " << startupDirectory;
		window.setStartupDirectory(startupDirectory, openAllCpls);
	}
	window.showMaximized();

	const int ret = app.start();

	xercesc::XMLPlatformUtils::Terminate();

	return ret;
}
