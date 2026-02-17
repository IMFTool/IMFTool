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
#include "JobQueue.h"
#include <QGlobalStatic>
#include <QMutexLocker>
#include <QPointer>


Q_GLOBAL_STATIC(JobQueue, theInstance)

AbstractJob::AbstractJob(const QString &rDescription) :
QObject(NULL), mMutex(), mError(), mDescription(rDescription), mIdentifier() {

}

Error AbstractJob::PerformRun() {

	Error error;
	emit Progress(0);
	error = Execute();
	emit Progress(100);
	mMutex.lock();
	mError = error;
	mMutex.unlock();
	emit Finished(!error.IsError(), QPrivateSignal());
	if(error.IsError() == false) emit Success(QPrivateSignal());
	else emit Failure(QPrivateSignal());
	return error;
}

Error AbstractJob::GetLastError() {

	QMutexLocker locker(&mMutex);
	return mError;
}

void AbstractJob::run() {

	PerformRun();
}

void AbstractJob::SetIdentifier(const QVariant &rIdentifier) {

	mMutex.lock();
	mIdentifier = rIdentifier;
	mMutex.unlock();
}

QVariant AbstractJob::GetIdentifier() {

	QMutexLocker locker(&mMutex);
	return mIdentifier;
}

JobQueue::JobQueue(QObject *pParent /*= NULL*/) :
QThread(pParent), mMutex(), mQueue(), mErrors(), mMaxProgress(0), mJobProgress(0), mCurrentProgress(0), mInterruptIfError(false) {

}

JobQueue::~JobQueue() {

	requestInterruption();
	wait();
	for(int i = 0; i < mQueue.size(); i++) {
		AbstractJob *p_job = mQueue.at(i);
		if(p_job && p_job->autoDelete() == true) p_job->deleteLater();
	}
}

void JobQueue::run() {

	emit Progress(0);
	mMutex.lock();
	bool stop = false;

	while(mQueue.empty() == false) {
		QPointer<AbstractJob> p_job(mQueue.dequeue());
		mMutex.unlock();
		connect(p_job.data(), SIGNAL(Progress(int)), this, SLOT(JobProgress(int)));

		if(p_job.isNull() == false) {
			emit NextJobStarted(p_job->GetDescription());
			bool auto_delete = p_job->autoDelete();
			Error error = p_job->PerformRun();
			if(error.IsError() == true) {
				mMutex.lock();
				mErrors.push_back(p_job->GetLastError());
				mMutex.unlock();
				if(mInterruptIfError == 1) stop = true;
			}
			if(auto_delete == true && p_job.isNull() == false) {
				p_job->deleteLater();
			}
		}

		mMutex.lock();

		if(isInterruptionRequested() == true || stop == true) break;
	}

	mMutex.unlock();
	emit Progress(100);
}

void JobQueue::StartQueue() {

	if(isRunning() == false) {
		mMaxProgress = mQueue.size() * 100;
		mJobProgress = 0;
		mCurrentProgress = 0;
		start(LowPriority);
	}
}

void JobQueue::InterruptQueue() {

	requestInterruption();
}

void JobQueue::AddJob(AbstractJob *pJob) {

	mMutex.lock();
	mQueue.enqueue(pJob);
	mMaxProgress += 100;
	mMutex.unlock();
}

void JobQueue::JobProgress(int progress) {

	if(progress == 0) mJobProgress = 0;
	mCurrentProgress += progress - mJobProgress;
	mJobProgress = progress;
	if(mMaxProgress > 0) emit Progress(mCurrentProgress * 100 / mMaxProgress);
}

JobQueue* JobQueue::GetGlobalInstance() {

	return theInstance();
}

QList<Error> JobQueue::GetErrors() {

	QMutexLocker lock(&mMutex);
	return mErrors;
}

void JobQueue::FlushQueue() {

	requestInterruption();
	wait();
	for(int i = 0; i < mQueue.size(); i++) {
		AbstractJob *p_job = mQueue.at(i);
		if(p_job && p_job->autoDelete() == true) p_job->deleteLater();
	}
	mMutex.lock();
	mQueue.clear();
	mErrors.clear();
	mMutex.unlock();
}

int JobQueue::GetQueueSize() {

	QMutexLocker lock(&mMutex);
	return mQueue.size();
}
