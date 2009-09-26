/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2009 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#ifndef __THREADENGINE_PTHREAD__
#define __THREADENGINE_PTHREAD__

#include <pthread.h>
#include "inspircd_config.h"
#include "base.h"

class Thread;

/** The ThreadEngine class has the responsibility of initialising
 * Thread derived classes. It does this by creating operating system
 * level threads which are then associated with the class transparently.
 * This allows Thread classes to be derived without needing to know how
 * the OS implements threads. You should ensure that any sections of code
 * that use threads are threadsafe and do not interact with any other
 * parts of the code which are NOT known threadsafe! If you really MUST
 * access non-threadsafe code from a Thread, use the Mutex class to wrap
 * access to the code carefully.
 */
class CoreExport ThreadEngine
{
 public:

	/** Constructor.
	 * @param Instance Creator object
	 */
	ThreadEngine();

	/** Destructor
	 */
	virtual ~ThreadEngine();

	/** Create a new thread. This takes an already allocated
	 * Thread* pointer and initializes it to use this threading
	 * engine. On failure, this function may throw a CoreException.
	 * @param thread_to_init Pointer to a newly allocated Thread
	 * derived object.
	 */
	void Start(Thread* thread_to_init);

	/** Returns the thread engine's name for display purposes
	 * @return The thread engine name
	 */
	const std::string GetName()
	{
		return "posix-thread";
	}
};

class CoreExport ThreadData
{
 public:
	pthread_t pthread_id;
	void FreeThread(Thread* toFree);
};

/** The Mutex class represents a mutex, which can be used to keep threads
 * properly synchronised. Use mutexes sparingly, as they are a good source
 * of thread deadlocks etc, and should be avoided except where absolutely
 * neccessary. Note that the internal behaviour of the mutex varies from OS
 * to OS depending on the thread engine, for example in windows a Mutex
 * in InspIRCd uses critical sections, as they are faster and simpler to
 * manage.
 */
class CoreExport Mutex
{
 private:
	pthread_mutex_t putex;
 public:
	/** Constructor.
	 */
	Mutex()
	{
		pthread_mutex_init(&putex, NULL);
	}
	/** Enter/enable the mutex lock.
	 */
	void Lock()
	{
		pthread_mutex_lock(&putex);
	}
	/** Leave/disable the mutex lock.
	 */
	void Unlock()
	{
		pthread_mutex_unlock(&putex);
	}
	/** Destructor
	 */
	~Mutex()
	{
		pthread_mutex_destroy(&putex);
	}
};

class ThreadQueueData
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
 public:
	ThreadQueueData()
	{
		pthread_mutex_init(&mutex, NULL);
		pthread_cond_init(&cond, NULL);
	}

	~ThreadQueueData()
	{
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&cond);
	}

	void Lock()
	{
		pthread_mutex_lock(&mutex);
	}

	void Unlock()
	{
		pthread_mutex_unlock(&mutex);
	}

	void Wakeup()
	{
		pthread_cond_signal(&cond);
	}

	void Wait()
	{
		pthread_cond_wait(&cond, &mutex);
	}
};

class ThreadSignalSocket;
class ThreadSignalData
{
 public:
	ThreadSignalSocket* sock;
};


#endif
