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
#include "MainWindow.h"
#include "KMQtLogSink.h"
#include "ImfPackage.h"
#include "MetadataExtractor.h"
#include "CustomProxyStyle.h"
#include "WizardResourceGenerator.h"
#ifdef Q_OS_WIN32
#include <qt_windows.h> // we need this for OutputDebugString()
#endif // Q_OS_WIN32
#include <QtWidgets/QApplication>
#include <QFile>
#include <QDir>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QMessageBox>
#include <QTextStream>
#include <QDateTime>
#include <QProcessEnvironment>
#include <QSettings>
#include <iostream>
#include <memory>
//WR
#include <QCommandLineParser>


namespace
{
	QFile log_file(get_app_data_location().absolutePath().append("/" DEBUG_FILE_NAME));
	QMutex mutex;
}

static void fatal_dbug_msg_handler(QtMsgType type, const QMessageLogContext &rContext, const QString &rMessage) {

	QMutexLocker mutex_locker(&mutex);

	if(type == QtFatalMsg) {
		if(QApplication::instance()->thread() == QThread::currentThread()) {
			QMessageBox msgBox;
			QString msgBoxString = QString(rMessage);
			msgBoxString.append("\nApplication will be terminated due to Fatal-Error.");
			msgBox.setText(msgBoxString);
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.exec();
		}
		QApplication::instance()->exit(1);
	}
}

static void dbug_msg_handler(QtMsgType type, const QMessageLogContext &rContext, const QString &rMessage) {

	QMutexLocker mutex_locker(&mutex);

	QString text = QString("[%1]").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss"));

	switch(type) {
		case QtDebugMsg:
#ifdef NDEBUG
			text += QString(" DEBUG    : %1").arg(rMessage);
#else
			text += QString(" DEBUG    (%1: %2): %3").arg(rContext.file).arg(rContext.line).arg(rMessage);
#endif // NDEBUG
			break;

		case QtWarningMsg:
#ifdef NDEBUG
			text += QString(" WARNING  : %1").arg(rMessage);
#else
			text += QString(" WARNING  (%1: %2): %3").arg(rContext.file).arg(rContext.line).arg(rMessage);
#endif
			break;

		case QtCriticalMsg:
#ifdef NDEBUG
			text += QString(" CRITICAL : %1").arg(rMessage);
#else
			text += QString(" CRITICAL (%1: %2): %3").arg(rContext.file).arg(rContext.line).arg(rMessage);
#endif
			break;

		case QtFatalMsg:
#ifdef NDEBUG
			text += QString(" FATAL    : %1").arg(rMessage);
#else
			text += QString(" FATAL    (%1: %2): %3").arg(rContext.file).arg(rContext.line).arg(rMessage);
#endif
			text += "\nApplication will be terminated due to Fatal-Error.";
			break;
		default:
			break;
	}

	QTextStream tStream(&log_file);
	tStream << text << "\n";

#ifdef Q_OS_WIN32
	OutputDebugString(reinterpret_cast<const wchar_t *>(text.append("\n").utf16()));
#elif defined Q_OS_LINUX || defined Q_OS_MAC
	std::string txt = text.append("\n").toStdString();
	fputs(txt.c_str(), stderr);
	fflush(stderr);
#endif // Q_OS_WIN32

	if(type == QtFatalMsg) {
		if(QApplication::instance()->thread() == QThread::currentThread()) {
			QMessageBox msgBox;
			QString msgBoxString = QString(rMessage);
			msgBoxString.append("\nApplication will be terminated due to Fatal-Error.");
			msgBox.setText(msgBoxString);
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.exec();
		}
		QApplication::instance()->exit(1);
	}
}

int main(int argc, char *argv[]) {

	QApplication a(argc, argv);
	a.setApplicationName(PROJECT_NAME);
	a.setOrganizationName("hsrm");
	a.setOrganizationDomain("hsrm.de");
	a.setApplicationVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH));
	a.setWindowIcon(QIcon(":/icon1.ico"));
	a.setStyle(new CustomProxyStyle);
	//WR
	QCommandLineParser parser;
	parser.setApplicationDescription(PROJECT_NAME);
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption startupDirectoryOption(QStringList() << "i" << "imp-directory",
			QCoreApplication::translate("main", "Open IMP in <directory> upon startup."),
			QCoreApplication::translate("main", "directory"));
	parser.addOption(startupDirectoryOption);

	QCommandLineOption openOption(QStringList() << "a" << "open-all-cpls",
			QCoreApplication::translate("main", "Open all CPLs from <directory>, as provided by option \"--imp-directory\", in Timeline View."));
	parser.addOption(openOption);

	// Process the actual command line arguments given by the user
	parser.process(a);
	QString startupDirectory = parser.value(startupDirectoryOption);
	bool openAllCpls = parser.isSet(openOption);
	//WR
	QSettings::setDefaultFormat(QSettings::IniFormat); // Set default settings format to init format.
	// load qt style sheet
	QFile style_file(":/style/stylesheet.qss");
	bool success = style_file.open(QFile::ReadOnly | QIODevice::Text);
	if(success) {
		a.setStyleSheet(QString::fromUtf8(style_file.readAll()));
		style_file.close();
	}
	else {
		qWarning() << "Couldn't load stylesheet: " << style_file.fileName();
	}

	// open log file
	if(log_file.size() > MAX_DEBUG_FILE_SIZE) {
		log_file.resize(0);
	}
	success = log_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
	if(success) {
		qInstallMessageHandler(dbug_msg_handler);
	}
	else {
		/* WR QMessageBox msgBox;
		QString msgBoxString = QString("\nCouldn't write Debug-Message to file (insufficient rights).\nDebug-Messages will not be saved");
		msgBox.setText(msgBoxString);
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();*/
		qInstallMessageHandler(fatal_dbug_msg_handler);
	}

	qDebug() << "**********************" PROJECT_NAME " starting up**********************";
	qDebug() << "asdcplib version: " << ASDCP::Version();
	qDebug() << "IMF Tool version: " << a.applicationVersion();
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
	qRegisterMetaType<QList<QStringList> >("DeliveryCheckResults");

	xercesc::XMLPlatformUtils::Initialize();
	MainWindow w;
	if (!startupDirectory.isEmpty()) {
		qDebug() << "Opening IMP at: " << startupDirectory;
		w.setStartupDirectory(startupDirectory, openAllCpls);
	}
	w.showMaximized();
	int ret = a.exec();
	//xercesc::XMLPlatformUtils::Terminate();
	return ret;
}
