/* Implementation file thread.cpp
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidGround framework.

	SolidGround is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SolidGround is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidGround.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "timespec.hpp"

#include "thread.hpp"
#include "debug.hpp"
#include "common.hpp"
#include "mutexpool.hpp"
#include <cerrno>

struct Cleaner{
	~Cleaner(){
		Thread::cleanup();
	}
};

enum ThreadInfo{
	FirstSpecificId = 0
};

pthread_key_t                   Thread::crtthread_key = 0;
Condition                       Thread::gcon;
Mutex                           Thread::gmut;
ulong                           Thread::thcnt = 0;
FastMutexPool<Thread::MutexPoolSize>    Thread::mutexpool;
pthread_once_t                  Thread::once_key = PTHREAD_ONCE_INIT;
Cleaner             			cleaner;
//static unsigned 				crtspecid = 0;
//*************************************************************************
int Condition::wait(Mutex &_mut, const TimeSpec &_ts){
	return pthread_cond_timedwait(&cond,&_mut.mut, &_ts);
}
//*************************************************************************
#ifndef UINLINES
#include "timespec.ipp"
#endif
//*************************************************************************
#ifndef UINLINES
#include "mutex.ipp"
#endif
//*************************************************************************
#ifndef UINLINES
#include "condition.ipp"
#endif
//*************************************************************************
#ifndef UINLINES
#include "synchronization.ipp"
#endif
//-------------------------------------------------------------------------

//TODO: use TimeSpec
int Mutex::timedLock(unsigned long _ms){
	timespec lts;
	lts.tv_sec=_ms/1000;
	lts.tv_nsec=(_ms%1000)*1000000;
	return pthread_mutex_timedlock(&mut,&lts);  
}
//-------------------------------------------------------------------------
int Mutex::reinit(Type _type){
	if(locked()) return -1;
	pthread_mutex_destroy(&mut);
	pthread_mutexattr_t att;
	pthread_mutexattr_init(&att);
	pthread_mutexattr_settype(&att, (int)_type);
	return pthread_mutex_init(&mut,&att);
}
//*************************************************************************
void Thread::init(){
	if(pthread_key_create(&Thread::crtthread_key, NULL)) throw -1;
	Thread::current(NULL);
}
//-------------------------------------------------------------------------
void Thread::cleanup(){
	idbg("");
	pthread_key_delete(Thread::crtthread_key);
}
//-------------------------------------------------------------------------
inline void Thread::enter(){
    gmut.lock();
    ++thcnt; gcon.broadcast();
    gmut.unlock();
}
//-------------------------------------------------------------------------
inline void Thread::exit(){
    gmut.lock();
    --thcnt; gcon.broadcast();
    gmut.unlock();
}
//-------------------------------------------------------------------------
Thread::Thread():dtchd(true),pcndpair(NULL){
	th = 0;
}
//-------------------------------------------------------------------------
Thread::~Thread(){
	for(SpecVecTp::iterator it(specvec.begin()); it != specvec.end(); ++it){
		if(it->first){
			cassert(it->second);
			(*it->second)(it->first);
		}
	}
}
//-------------------------------------------------------------------------
void Thread::dummySpecificDestroy(void*){
}
//-------------------------------------------------------------------------
/*static*/ unsigned Thread::processorCount(){
	return get_nprocs();
}
//-------------------------------------------------------------------------
int Thread::join(){
	if(pthread_equal(th, pthread_self())) return NOK;
	if(detached()) return NOK;
	int rcode =  pthread_join(this->th, NULL);
	return rcode;
}
//-------------------------------------------------------------------------
int Thread::detached() const{
	//Mutex::Locker lock(mutex());
	return dtchd;
}
//-------------------------------------------------------------------------
int Thread::detach(){
	Mutex::Locker lock(mutex());
	if(detached()) return OK;
	int rcode = pthread_detach(this->th);
	if(rcode == OK)	dtchd = 1;
	return rcode;
}
//-------------------------------------------------------------------------
unsigned Thread::specificId(){
	static unsigned sid = FirstSpecificId - 1;
	return ++sid;
}
//-------------------------------------------------------------------------
unsigned Thread::specific(unsigned _pos, void *_psd, SpecificFncTp _pf){
	cassert(current());
	//cassert(_pos < current()->specvec.size());
	if(_pos >= current()->specvec.size()) current()->specvec.resize(_pos + 4);
	current()->specvec[_pos] = SpecPairTp(_psd, _pf);
	return _pos;
}
//-------------------------------------------------------------------------
// unsigned Thread::specific(void *_psd){
// 	cassert(current());
// 	current()->specvec.push_back(_psd);
// 	return current()->specvec.size() - 1;
// }
//-------------------------------------------------------------------------
void* Thread::specific(unsigned _pos){
	cassert(current() && _pos < current()->specvec.size());
	return current()->specvec[_pos].first;
}
Mutex& Thread::gmutex(){
	return gmut;
}
//-------------------------------------------------------------------------
int Thread::current(Thread *_ptb){
	pthread_setspecific(Thread::crtthread_key, _ptb);
	return OK;
}

//-------------------------------------------------------------------------
int Thread::start(int _wait, int _detached, ulong _stacksz){	
	Mutex::Locker locker(mutex());
	idbg("");
	if(th) return BAD;
	idbg("");
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(_detached){
		if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)){
			pthread_attr_destroy(&attr);
			idbg("could not made thread detached");
			return BAD;
		}
	}
	if(_stacksz){
		if(pthread_attr_setstacksize(&attr, _stacksz)){
			pthread_attr_destroy(&attr);
			idbg("could not set staksize");
			return BAD;
		}
	}
	ConditionPairTp cndpair;
	cndpair.second = 1;
	if(_wait){
		gmutex().lock();
		pcndpair = &cndpair;
	}
	if(pthread_create(&th,&attr,&Thread::th_run,this)){
		pthread_attr_destroy(&attr);
		idbg("could not create thread");
		return BAD;
	}
	//NOTE: DO not access any thread member from now - the thread may be detached
	if(_wait){
		while(cndpair.second){
			cndpair.first.wait(gmutex());
		}
		gmutex().unlock();
	}
	pthread_attr_destroy(&attr);
	idbg("");
	return OK;
}
//-------------------------------------------------------------------------
void Thread::signalWaiter(){
	pcndpair->second = 0;
	pcndpair->first.signal();
	pcndpair = NULL;
}
//-------------------------------------------------------------------------
int Thread::waited(){
	return pcndpair != NULL;
}
//-------------------------------------------------------------------------
void Thread::waitAll(){
    gmut.lock();
    while(thcnt != 0) gcon.wait(gmut);
    gmut.unlock();
}
//-------------------------------------------------------------------------
void* Thread::th_run(void *pv){
	idbg("thrun enter"<<pv);
	Thread	*pth(reinterpret_cast<Thread*>(pv));
	Thread::enter();
	Thread::current(pth);
	if(pth->waited()){
		pth->signalWaiter();
		Thread::yield();
	}
	pth->prepare();
	pth->run();
	pth->unprepare();
	Thread::exit();
	if(pth->detached()) delete pth;
	idbg("thrun exit");
	return NULL;
}

//-------------------------------------------------------------------------
