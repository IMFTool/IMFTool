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
#include "Error.h"
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QRunnable>
#include <QVariant>
#include <QAtomicInteger>


class AbstractJob;

class JobQueue : public QThread {

	Q_OBJECT

public:
	JobQueue(QObject *pParent = NULL);
	virtual ~JobQueue();
	bool IsQueueRunning() const { return isRunning(); }
	void AddJob(AbstractJob *pJob);
	//! The Job Queue stops if an error occurred if JobQueue::GetInterruptIfError returns true (not the default).
	void SetInterruptIfError(bool interrupt) { mInterruptIfError = interrupt; }
	bool GetInterruptIfError() const { return mInterruptIfError; }
	int GetQueueSize();
	//! Waits for Queue interruption, removes (and deletes if AbstractJob::SetAutoDelete() is set) all jobs and errors.
	void FlushQueue();
	QList<Error> GetErrors();
	static JobQueue* GetGlobalInstance();

signals:
	void Progress(int progress);
	void NextJobStarted(const QString &rDescription);

	public slots:
	void StartQueue();
	//! Does not wait for interruption. Use JobQueue::wait() if you want to wait.
	void InterruptQueue();

	private slots:
	void JobProgress(int progress);

protected:
	virtual void run();

private:
	Q_DISABLE_COPY(JobQueue);

	QQueue<AbstractJob*> mQueue;
	QMutex mMutex;
	QList<Error> mErrors;
	QAtomicInteger<int> mMaxProgress;
	QAtomicInteger<int> mJobProgress;
	QAtomicInteger<int> mCurrentProgress;
	QAtomicInteger<int> mInterruptIfError;
};


//! The Job Queue or Thread pool delete the Job automatically if Job::autoDelete returns true (the default). Note that this flag must be set before the JobQueue::AddJob() or QThreadPool::start().
class AbstractJob : public QObject, public QRunnable {

	Q_OBJECT

public:
	AbstractJob(const QString &rDescription);
	virtual ~AbstractJob() {}
	void SetIdentifier(const QVariant &rIdentifier);
	QVariant GetIdentifier();
	QString GetDescription() const { return mDescription; }
	//! Returns the last error. If no error occurred during execution Error::IsError() returns false. This method is thread safe.
	Error GetLastError();
	//! Don't reimplement this method or make sure that base class implementation is invoked.
	virtual void run();
	Error PerformRun();

signals:
	void Progress(int progress);
	void Finished(bool success, QPrivateSignal);
	void Success(QPrivateSignal);
	void Failure(QPrivateSignal);

protected:
	/*! \brief
	Implement the task that should run asynchronously. You should emit Job::Progress() regulary to inform the JobQueue about the current progress.
	Additionally you should check QThread::currentThread().isInterruptionRequested() regularry and return Job::Execute() prematurely if interruption is requested.
	Return empty error if everything went fine. Otherwise return filled error.
	*/
	virtual Error Execute() = 0;

private:
	Q_DISABLE_COPY(AbstractJob);
	void SetParent(QObject *pParent) {}

	QMutex mMutex;
	Error mError;
	QString mDescription;
	QVariant mIdentifier;
};
