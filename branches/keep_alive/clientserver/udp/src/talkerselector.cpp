/* Implementation file talkerselector.cpp
	
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "system/debug.hpp"
#include "system/timespec.hpp"

#include <new>
#include "utility/stack.hpp"
#include "utility/queue.hpp"

#include "core/object.hpp"
#include "core/common.hpp"
#include "udp/talkerselector.hpp"
#include "udp/talker.hpp"
#include "udp/station.hpp"

// #include <iostream>
// 
// using namespace std;

namespace clientserver{
namespace udp{

struct TalkerSelector::SelTalker{
	enum State{
		OutExecQueue = 0,
		InExecQueue = 1,
	};
	SelTalker():timepos(0),evmsk(0),state(OutExecQueue){}
	void reset();
	TalkerPtrTp		tkrptr;//4
	TimeSpec		timepos;//4
	ulong			evmsk;//4
	//state: 1 means the object is in queue
	//or if 0 in execq check loop means the channel was already checked
	int				state;//4
};

struct TalkerSelector::Data{
	enum{
		UNROLLSZ = 4, 
		UNROLLMSK = 3,
		EPOLLMASK = EPOLLERR | EPOLLHUP | EPOLLET | EPOLLIN | EPOLLOUT,
		EXIT_LOOP = 1,
		FULL_SCAN = 2,
		READ_PIPE = 4,
		MAXTIMEPOS = 0xffffffff,
		MAXPOLLWAIT = 0x7FFFFFFF,
	};
	typedef Stack<SelTalker*> 	FreeStackTp;
	typedef Queue<SelTalker*>	ChQueueTp;
	
	int computePollWait();
	
	Data();
	~Data();
	int  empty()const{ return sz == 1;}
		
	void initFreeStack();
	void push(SelTalker *_pc){
		execq.push(_pc);
	}
	
	uint			cp;
	uint			sz;
	int				selcnt;
	int				epfd;
	epoll_event 	*pevs;
	SelTalker		*pchs;
	ChQueueTp		execq;
	FreeStackTp		fstk;
	int				pipefds[2];
	TimeSpec        ntimepos;//next timepos == next timeout
	TimeSpec        ctimepos;//current time pos
};

//---------------------------------------------------------------------------

void TalkerSelector::SelTalker::reset(){
	state = OutExecQueue;
	tkrptr.release();
	timepos.set(0);
	evmsk = 0;
}

//---------------------------------------------------------------------------

TalkerSelector::Data::Data():
	cp(0), sz(0), selcnt(0),
	epfd(-1), pevs(NULL), pchs(NULL),
	ntimepos(0), ctimepos(0)
{
	pipefds[0] = pipefds[1] = -1;
}

TalkerSelector::Data::~Data(){
	close(pipefds[0]);
	close(pipefds[1]);
	close(epfd);
	delete []pevs;
	delete []pchs;
}

void TalkerSelector::Data::initFreeStack(){
	for(int i = cp - 1; i > 0; --i){
		fstk.push(&(pchs[i]));
	}
}

TalkerSelector::TalkerSelector():d(*(new Data)){
}

int TalkerSelector::reserve(ulong _cp){
	cassert(_cp);
	cassert(!d.sz);
	if(_cp < d.cp) return OK;
	if(d.cp){
		cassert(false);
		delete []d.pevs;delete []d.pchs;//delete []ph;
	}
	d.cp = _cp;
	
	//The epoll_event table
	
	d.pevs = new epoll_event[d.cp];
	for(uint i = 0; i < d.cp; ++i){
		d.pevs[i].events = 0;
		d.pevs[i].data.ptr = NULL;
	}
	
	//the SelTalker table:
	
	d.pchs = new SelTalker[d.cp];
	//no need for initialization
		
	//Init the free station stack:
	d.initFreeStack();
	
	//init the station queue
	//execq.reserve(cp);
	//zero is reserved for pipe
	if(d.epfd < 0 && (d.epfd = epoll_create(d.cp)) < 0){
		cassert(false);
		return -1;
	}
	//init the pipes:
	if(d.pipefds[0] < 0){
		if(pipe(d.pipefds)){
			cassert(false);
			return -1;
		}
		fcntl(d.pipefds[0], F_SETFL, O_NONBLOCK);
		//fcntl(pipefds[1],F_SETFL, O_NONBLOCK);
		
		epoll_event ev;
		ev.data.ptr = &d.pchs[0];
		ev.events = EPOLLIN | EPOLLPRI;//must be LevelTriggered
		if(epoll_ctl(d.epfd, EPOLL_CTL_ADD, d.pipefds[0], &ev)){
			idbgx(Dbg::udp, "error epollctl");
			cassert(false);
			return -1;
		}
	}
	idbgx(Dbg::udp, "Pipe fds "<<d.pipefds[0]<<" "<<d.pipefds[1]);
	d.ctimepos.set(0);
	d.ntimepos.set(Data::MAXTIMEPOS);
	d.selcnt = 0;
	d.sz = 1;
	return OK;
}

TalkerSelector::~TalkerSelector(){
	delete &d;
	idbgx(Dbg::udp, "done");
}

uint TalkerSelector::capacity()const{
	return d.cp - 1;
}
uint TalkerSelector::size()const{
	return d.sz;
}
int  TalkerSelector::empty()const{
	return d.sz == 1;
}
int  TalkerSelector::full()const{
	return d.sz == d.cp;
}
void TalkerSelector::prepare(){
}
void TalkerSelector::unprepare(){
}

void TalkerSelector::push(const TalkerPtrTp &_tkrptr, uint _thid){
	cassert(d.fstk.size());
	SelTalker *pc(d.fstk.top());
	d.fstk.pop();
	_tkrptr->setThread(_thid, pc - d.pchs);
	
	pc->timepos.set(Data::MAXTIMEPOS);
	pc->evmsk = 0;
	
	epoll_event ev;
	ev.events = 0;
	ev.data.ptr = pc;
	
	if(_tkrptr->station().descriptor() >= 0 && 
		epoll_ctl(d.epfd, EPOLL_CTL_ADD, _tkrptr->station().descriptor(), &ev)){
		idbgx(Dbg::udp, "error adding filedesc "<<_tkrptr->station().descriptor());
		pc->tkrptr.clear();
		pc->reset(); d.fstk.push(pc);
		cassert(false);
	}else{
		++d.sz;
		pc->tkrptr = _tkrptr;
		idbgx(Dbg::udp, "pushing "<<&(*(pc->tkrptr))<<" on pos "<<(pc - d.pchs));
		pc->state = SelTalker::InExecQueue;
		d.push(pc);
	}
}

void TalkerSelector::signal(uint _objid){
	idbgx(Dbg::udp, "signal connection: "<<_objid);
	write(d.pipefds[1], &_objid, sizeof(uint));
}

int TalkerSelector::doIo(Station &_rch, ulong _evs){
	int rv = 0;
	if(_evs & EPOLLIN){
		rv = _rch.doRecv();
	}
	if((_evs & EPOLLOUT) && !(rv & ERRDONE)){
		rv |= _rch.doSend();
	}
	return rv;
}
inline int TalkerSelector::Data::computePollWait(){
	if(ntimepos.seconds() == MAXTIMEPOS)
		return MAXPOLLWAIT;
	int rv = (ntimepos.seconds() - ctimepos.seconds()) * 1000;
	rv += (ntimepos.nanoSeconds() - ctimepos.nanoSeconds())/1000000 ; 
	if(rv < 0) return MAXPOLLWAIT;
	return rv;
}

void TalkerSelector::run(){
	TimeSpec	crttout;
	epoll_event	ev;
	int 		state;
	int			evs;
	const int	maxnbcnt = 256;
	int			nbcnt = -1;
	int 		pollwait = 0;
	do{
		state = 0;
		if(nbcnt < 0){
			clock_gettime(CLOCK_MONOTONIC, &d.ctimepos);
			nbcnt = maxnbcnt;
		}
		for(int i = 0; i < d.selcnt; ++i){
			SelTalker *pc = reinterpret_cast<SelTalker*>(d.pevs[i].data.ptr);
			if(pc != d.pchs){
				if(pc->state == SelTalker::OutExecQueue && (evs = doIo(pc->tkrptr->station(), d.pevs[i].events))){
					crttout = d.ctimepos;
					state |= doExecute(*pc, evs, crttout, ev);
				}
			}else{
				state |= Data::READ_PIPE;
			}
		}
		if(state & Data::READ_PIPE){
			state |= doReadPipe();
		}
		if((state & Data::FULL_SCAN) || d.ctimepos >= d.ntimepos){
			idbgx(Dbg::udp, "fullscan");
			d.ntimepos.set(Data::MAXTIMEPOS);
			for(uint i = 1; i < d.cp; ++i){
				SelTalker &pc = d.pchs[i];
				if(!pc.tkrptr) continue;
				evs = 0;
				if(d.ctimepos >= pc.timepos) evs |= TIMEOUT;
				else if(d.ntimepos > pc.timepos) d.ntimepos = pc.timepos;
				if(pc.tkrptr->signaled(S_RAISE)) evs |= SIGNALED;//should not be checked by objs
				if(evs){
					crttout = d.ctimepos;
					doExecute(pc, evs, crttout, ev);
				}
			}
		}
		{	
			int qsz = d.execq.size();
			while(qsz){//we only do a single scan:
				idbgx(Dbg::udp, "qsz = "<<qsz<<" queuesz "<<d.execq.size());
				SelTalker &rc = *d.execq.front();d.execq.pop(); --qsz;
				if(rc.state == SelTalker::InExecQueue){
					crttout = d.ctimepos;
					//rc.state = 0;//moved to doExecute
					doExecute(rc, 0, crttout, ev);
				}
			}
		}
		idbgx(Dbg::udp, "sz = "<<d.sz);
		if(d.empty()) state |= Data::EXIT_LOOP;
		if(state || d.execq.size()){
			pollwait = 0;
			--nbcnt;
		}else{
			pollwait = d.computePollWait();
			idbgx(Dbg::udp, "pollwait "<<pollwait<<" ntimepos.s = "<<d.ntimepos.seconds()<<" ntimepos.ns = "<<d.ntimepos.nanoSeconds());
			idbgx(Dbg::udp, "ctimepos.s = "<<d.ctimepos.seconds()<<" ctimepos.ns = "<<d.ctimepos.nanoSeconds());
			nbcnt = -1;
        }
		d.selcnt = epoll_wait(d.epfd, d.pevs, d.sz, pollwait);
		idbgx(Dbg::udp, "selcnt = "<<d.selcnt);
	}while(!(state & Data::EXIT_LOOP));
	idbgx(Dbg::udp, "exiting loop");
}

int TalkerSelector::doExecute(
	SelTalker &_rch,
	ulong _evs,
	TimeSpec &_rcrttout,
	epoll_event &_rev
){
	int rv = 0;
	Talker &rcon = *_rch.tkrptr;
	_rch.state = SelTalker::OutExecQueue;
	switch(rcon.execute(_evs, _rcrttout)){
		case BAD://close
			idbgx(Dbg::udp, "BAD: removing the connection");
			d.fstk.push(&_rch);
			epoll_ctl(d.epfd, EPOLL_CTL_DEL, rcon.station().descriptor(), NULL);
			_rch.tkrptr.clear();
			--d.sz;
			if(d.empty()) rv = Data::EXIT_LOOP;
			break;
		case OK://
			idbgx(Dbg::udp, "OK: reentering connection");
			d.execq.push(&_rch);
			_rch.state = SelTalker::InExecQueue;
			_rch.timepos.set(Data::MAXTIMEPOS);
			break;
		case NOK:
			idbgx(Dbg::udp, "TOUT: connection waits for signals");
			{	
				ulong t = (EPOLLERR | EPOLLHUP | EPOLLET) | rcon.station().ioRequest();
				if((_rch.evmsk & Data::EPOLLMASK) != t){
					idbgx(Dbg::udp, "RTOUT: epollctl");
					_rch.evmsk = _rev.events = t;
					_rev.data.ptr = &_rch;
					int rv = epoll_ctl(d.epfd, EPOLL_CTL_MOD, rcon.station().descriptor(), &_rev);
					cassert(!rv);
				}
				if(_rcrttout != d.ctimepos){
					_rch.timepos = _rcrttout;
					if(d.ntimepos > _rcrttout){
						d.ntimepos = _rcrttout;
					}
				}else{
					_rch.timepos.set(Data::MAXTIMEPOS);
				}
			}
			break;
		case LEAVE:
			idbgx(Dbg::udp, "LEAVE: connection leave");
			d.fstk.push(&_rch);
			//TODO: remove fd from epoll
			if(d.sz == d.cp) rv = Data::EXIT_LOOP;
			_rch.tkrptr.release();
			--d.sz;
			break;
		case REGISTER:{//
			idbgx(Dbg::udp, "REGISTER: register connection with new descriptor");
			_rev.data.ptr = &_rch;
			uint ioreq = rcon.station().ioRequest();
			_rch.evmsk = _rev.events = (EPOLLERR | EPOLLHUP | EPOLLET) | ioreq;
			int rv = epoll_ctl(d.epfd, EPOLL_CTL_ADD, rcon.station().descriptor(), &_rev);
			cassert(!rv);
			if(!ioreq){
				d.execq.push(&_rch);
				_rch.state = SelTalker::InExecQueue;
			}
			}break;
		case UNREGISTER:{
			idbgx(Dbg::udp, "UNREGISTER: unregister connection's descriptor");
			int rv = epoll_ctl(d.epfd, EPOLL_CTL_DEL, rcon.station().descriptor(), NULL);
			cassert(!rv);
			d.execq.push(&_rch);
			_rch.state = SelTalker::InExecQueue;
			}break;
		default:
			cassert(false);
	}
	idbgx(Dbg::udp, "doExecute return "<<rv);
	return rv;
}

int TalkerSelector::doReadPipe(){
	enum {BUFSZ = 128, BUFLEN = BUFSZ * sizeof(uint)};
	uint	buf[BUFSZ];
	int rv = 0;//no
	int rsz = 0;int j = 0;int maxcnt = (d.cp / BUFSZ) + 1;
	idbgx(Dbg::udp, "maxcnt = "<<maxcnt);
	SelTalker	*pch = NULL;
	while((++j <= maxcnt) && ((rsz = read(d.pipefds[0], buf, BUFLEN)) == BUFLEN)){
		for(int i = 0; i < BUFSZ; ++i){
			idbgx(Dbg::udp, "buf["<<i<<"]="<<buf[i]);
			uint pos = buf[i];
			if(pos){
				if(pos < d.cp && (pch = d.pchs + pos)->tkrptr && pch->state == SelTalker::OutExecQueue && pch->tkrptr->signaled(S_RAISE)){
					d.execq.push(pch);
					pch->state = SelTalker::InExecQueue;
					idbgx(Dbg::udp, "pushig "<<pos<<" connection into queue");
				}
			}else rv = Data::EXIT_LOOP;
		}
	}
	if(rsz){
		rsz >>= 2;
		for(int i = 0; i < rsz; ++i){	
			idbgx(Dbg::udp, "buf["<<i<<"]="<<buf[i]);
			uint pos = buf[i];
			if(pos){
				if(pos < d.cp && (pch = d.pchs + pos)->tkrptr && pch->state == SelTalker::OutExecQueue && pch->tkrptr->signaled(S_RAISE)){
					d.execq.push(pch);
					pch->state = SelTalker::InExecQueue;
					idbgx(Dbg::udp, "pushig "<<pos<<" connection into queue");
				}
			}else rv = Data::EXIT_LOOP;
		}
	}
	if(j > maxcnt){
		//dummy read:
		rv = Data::EXIT_LOOP | Data::FULL_SCAN;//scan all filedescriptors for events
		idbgx(Dbg::udp, "reading dummy");
		while((rsz = read(d.pipefds[0], buf, BUFSZ)) > 0);
	}
	idbgx(Dbg::udp, "readpiperv = "<<rv);
	return rv;
}

}//namespace tcp
}//namespace clientserver

