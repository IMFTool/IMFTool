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
#include "global.h"
#include "ImfCommon.h"
#include <QEvent>


class EventCplEditRateChange : public QEvent {

public:
	EventCplEditRateChange(const EditRate &rOldEditRate, const EditRate &rNewEditRate) : 
		QEvent(static_cast<QEvent::Type>(UserEventCplEditRateChange)), mOldEditRate(rOldEditRate), mNewEditRate(rNewEditRate) {}
	virtual ~EventCplEditRateChange() {}
	EditRate GetOldEditRate() const { return mOldEditRate; }
	EditRate GetNewEditRate() const { return mNewEditRate; }

private:
	EditRate mOldEditRate;
	EditRate mNewEditRate;
};