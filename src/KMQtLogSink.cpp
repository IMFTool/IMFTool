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
#include "KMQtLogSink.h"
#include "KM_mutex.h"
#include "global.h"


Kumu::KMQtLogSink::KMQtLogSink() : ILogSink() {

	m_options = LOG_OPTION_TYPE;
}

void Kumu::KMQtLogSink::WriteEntry(const LogEntry &Entry) {

	std::string buf;
	AutoMutex L(m_lock);
	WriteEntryToListeners(Entry);

	if(Entry.TestFilter(m_filter)) {
		Entry.CreateStringWithOptions(buf, m_options);
		switch(Entry.Type) {
			case LOG_DEBUG || LOG_INFO:
				qDebug() << "KUMU" << buf.c_str();
				break;
			case LOG_WARN:
				qWarning() << "KUMU" << buf.c_str();
				break;
			case LOG_ERROR:
				qCritical() << "KUMU" << buf.c_str();
				break;
			default:
				qDebug() << "KUMU" << buf.c_str();
				break;
		}
	}
}
