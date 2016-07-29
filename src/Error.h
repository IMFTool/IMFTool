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
#include "AS_02.h"
#include "Metadata.h"
#include <vector>
#include <QObject>



class Error {

public:
	enum eError {

		None = 0,
		Kumu,
		SourceFilesMissing,
		SourceFileOpenError,
		UnsupportedEssence,
		UnknownDuration,
		WorkerInterruptionRequest,
		ChannelCountMismatch,
		SoundfieldGroupIncomplete,
		MCAStringDecoding,
		HashCalculation,
		WorkerRunning,
		OpenCLError,
		XMLSchemeError,
		Unknown
	};
	//! Constructs empty error (IsError returns false).
	Error() : mErrorType(None), mErrorDescription(""), mRecoverable(true) {}
	//! Constructs Kumu error.
	Error(const ASDCP::Result_t &rResult) : mErrorType(ASDCP_SUCCESS(rResult) ? Error::None : Error::Kumu), mErrorDescription(rResult.Label()), mRecoverable(ASDCP_SUCCESS(rResult) ? true : false) {}
	Error(eError error, const QString &rDescription = QString(), bool recoverable = false) : mErrorType(error), mErrorDescription(rDescription), mRecoverable(recoverable) {}
	~Error() {}
	bool IsError() const { return (mErrorType != None && mRecoverable == false); }
	bool IsRecoverableError() const { return (mErrorType != None && mRecoverable == true); }
	void AppendErrorDescription(const QString &rAppend) { mErrorDescription.append(rAppend); }
	QString GetErrorMsg() const {
		QString ret;
		switch(mErrorType) {
			case None:
				ret = QObject::tr("No error"); break;
			case Kumu:
				ret = QObject::tr("Kumu error"); break;
			case SourceFilesMissing:
				ret = QObject::tr("No source file(s) found"); break;
			case SourceFileOpenError:
				ret = QObject::tr("Couldn't open source file(s)."); break;
			case UnsupportedEssence:
				ret = QObject::tr("The file contains an unsupported essence"); break;
			case UnknownDuration:
				ret = QObject::tr("Unable to determine file duration"); break;
			case WorkerInterruptionRequest:
				ret = QObject::tr("The worker thread was interrupted due to user request"); break;
			case ChannelCountMismatch:
				ret = QObject::tr("Soundfield group channel count differs from essence stream channel count"); break;
			case SoundfieldGroupIncomplete:
				ret = QObject::tr("Soundfield group is incomplete"); break;
			case MCAStringDecoding:
				ret = QObject::tr("MCA string decoding failed"); break;
			case HashCalculation:
				ret = QObject::tr("Hash claculation failed"); break;
			case WorkerRunning:
				ret = QObject::tr("The worker thread is running"); break;
			case OpenCLError:
				ret = QObject::tr("OpenCL Error"); break;
			case XMLSchemeError:
				ret = QObject::tr("XML Schema Error"); break;
			case Unknown:
				ret = QObject::tr("Unknown error"); break;
			default:
				ret = QObject::tr("Unknown error"); break;
		}
		return ret;
	}
	QString GetErrorDescription() const { return mErrorDescription; }

private:
	eError		mErrorType;
	QString 	mErrorDescription;
	bool		mRecoverable;
};


QDebug operator<< (QDebug dbg, const Error &rError);
