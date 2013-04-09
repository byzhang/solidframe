/* Implementation file alphaconnection.cpp
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidFrame framework.

	SolidFrame is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SolidFrame is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidFrame.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "system/debug.hpp"
#include "system/timespec.hpp"
#include "system/mutex.hpp"
#include "system/socketaddress.hpp"

#include "utility/ostream.hpp"
#include "utility/istream.hpp"
#include "utility/iostream.hpp"

#include "frame/ipc/ipcservice.hpp"
#include "frame/requestuid.hpp"


#include "core/manager.hpp"
#include "core/messages.hpp"

#include "alpha/alphaservice.hpp"

#include "alphaconnection.hpp"
#include "alphacommand.hpp"
#include "alphamessages.hpp"
#include "alphaprotocolfilters.hpp"
#include "audit/log.hpp"


using namespace solid;

namespace concept{
namespace alpha{

void Logger::doInFlush(const char *_pb, unsigned _bl){
	if(Log::the().isSet(Log::any, Log::Input)){
		Log::the().record(Log::Input, Log::any, 0, __FILE__, __FUNCTION__, __LINE__).write(_pb, _bl);
		Log::the().done();
	}
}

void Logger::doOutFlush(const char *_pb, unsigned _bl){
	if(Log::the().isSet(Log::any, Log::Output)){
		Log::the().record(Log::Output, Log::any, 0, __FILE__, __FUNCTION__, __LINE__).write(_pb, _bl);
		Log::the().done();
	}
}

/*static*/ void Connection::initStatic(Manager &_rm){
	Command::initStatic(_rm);
}
namespace{
static const DynamicRegisterer<Connection>	dre;
}
/*static*/ void Connection::dynamicRegister(){
	DynamicExecuterT::registerDynamic<InputStreamMessage, Connection>();
	DynamicExecuterT::registerDynamic<OutputStreamMessage, Connection>();
	DynamicExecuterT::registerDynamic<InputOutputStreamMessage, Connection>();
	DynamicExecuterT::registerDynamic<StreamErrorMessage, Connection>();
	DynamicExecuterT::registerDynamic<RemoteListMessage, Connection>();
	DynamicExecuterT::registerDynamic<FetchSlaveMessage, Connection>();
	DynamicExecuterT::registerDynamic<SendStringMessage, Connection>();
	DynamicExecuterT::registerDynamic<SendStreamMessage, Connection>();
}

#ifdef UDEBUG
/*static*/ Connection::ConnectionsVectorT& Connection::connections(){
	static ConnectionsVectorT cv;
	return cv;
}
#endif


Connection::Connection(
	ResolveData &_rai
):	wtr(&logger), rdr(wtr, &logger),
	pcmd(NULL), ai(_rai), reqid(1){
	aiit = ai.begin();
	state(Connect);
#ifdef UDEBUG
	connections().push_back(this);
#endif
}

Connection::Connection(
	const SocketDevice &_rsd
):	BaseT(_rsd), wtr(&logger),rdr(wtr, &logger),
	pcmd(NULL), reqid(1){

	state(Init);
#ifdef UDEBUG
	connections().push_back(this);
#endif

}


/*
NOTE: 
* Releasing the connection here and not in a base class destructor because
here we know the exact type of the object - so the service can do other things 
based on the type.
* Also it ensures a proper/safe visiting. Suppose the unregister would have taken 
place in a base destructor. If the visited object is a leaf, one may visit
destroyed data.
NOTE: 
* Visitable data must be destroyed after releasing the connection!!!
*/

Connection::~Connection(){
	idbg("destroy connection id "<<this->id()<<" pcmd "<<pcmd);
	delete pcmd; pcmd = NULL;
#ifdef UDEBUG
	for(
		ConnectionsVectorT::iterator it(connections().begin());
		it != connections().end();
		++it
	){
		if(*it == this){
			*it = connections().back();
			connections().pop_back();
			break;
		}
	}
#endif
}


/*virtual*/ bool Connection::notify(DynamicPointer<frame::Message> &_rmsgptr){
	if(this->state() < 0){
		_rmsgptr.clear();
		return false;//no reason to raise the pool thread!!
	}
	dr.push(this, DynamicPointer<>(_rmsgptr));
	return Object::notify(frame::S_SIG | frame::S_RAISE);
}


/*
	The main loop with the implementation of the alpha protocol's
	state machine. We dont need a loop, we use the ConnectionSelector's
	loop - returning OK.
	The state machine is a simple switch
*/

int Connection::execute(ulong _sig, TimeSpec &_tout){
	//concept::Manager &rm = concept::Manager::the();
	frame::requestuidptr->set(this->id(), Manager::the().id(*this).second);
	//_tout.add(2400);
	if(_sig & (frame::TIMEOUT | frame::ERRDONE)){
		if(state() == ConnectWait){
			state(Connect);
		}else
			if(_sig & frame::TIMEOUT){
				edbg("timeout occured - destroy connection "<<state());
			}else{
				edbg("error occured - destroy connection "<<state());
			}
			return BAD;
	}
	
	if(notified()){//we've received a signal
		ulong sm(0);
		{
			Locker<Mutex>	lock(this->mutex());
			sm = grabSignalMask(0);//grab all bits of the signal mask
			if(sm & frame::S_KILL) return BAD;
			if(sm & frame::S_SIG){//we have signals
				dr.prepareExecute(this);
			}
		}
		if(sm & frame::S_SIG){//we've grabed signals, execute them
			while(dr.hasCurrent(this)){
				dr.executeCurrent(this);
				dr.next(this);
			}
		}
		//now we determine if we return with NOK or we continue
		if(!_sig) return NOK;
	}
	const uint32 sevs = socketEventsGrab();
	if(sevs & frame::ERRDONE){
		return BAD;
	}
// 	if(socketEvents() & frame::OUTDONE){
// 		switch(state()){
// 			case IdleExecute:
// 			case ExecuteTout:
// 				state(Execute);
// 				break;
// 			case Execute:
// 				break;
// 			case Connect:
// 				state(Init);
// 				break;
// 			default:
// 				state(Parse);
// 		}
// 	}
	int rc;
	switch(state()){
		case Init:
			wtr.buffer(protocol::SpecificBuffer(2*1024));
			rdr.buffer(protocol::SpecificBuffer(2*1024));
			if(this->socketIsSecure()){
				int rv = this->socketSecureAccept();
				state(Banner);
				return rv;
			}else{
				state(Banner);
			}
		case Banner:{
			concept::Manager	&rm = concept::Manager::the();
			uint32				myport(rm.ipc().basePort());
			IndexT				objid(this->id());
			uint32				objuid(Manager::the().id(*this).second);
			char				host[SocketInfo::HostStringCapacity];
			char				port[SocketInfo::ServiceStringCapacity];
			SocketAddress		addr;
			
			
			
			writer()<<"* Hello from alpha server ("<<myport<<' '<<' '<< objid<<' '<<objuid<<") [";
			socketLocalAddress(addr);
			addr.toString(
				host,
				SocketInfo::HostStringCapacity,
				port,
				SocketInfo::ServiceStringCapacity,
				SocketInfo::NumericService | SocketInfo::NumericHost
			);
			writer()<<host<<':'<<port<<" -> ";
			socketRemoteAddress(addr);
			addr.toString(
				host,
				SocketInfo::HostStringCapacity,
				port,
				SocketInfo::ServiceStringCapacity,
				SocketInfo::NumericService | SocketInfo::NumericHost
			);
			writer()<<host<<':'<<port<<"]"<<'\r'<<'\n';
			writer().push(&Writer::flushAll);
			state(Execute);
			}break;
		case ParsePrepare:
			idbg("PrepareReader");
			prepareReader();
		case Parse:
			//idbg("Parse");
			state(Parse);
			switch((rc = reader().run())){
				case OK: break;
				case NOK:
					if(socketHasPendingSend()){
						socketTimeoutSend(3000);
					}else if(socketHasPendingRecv()){
						socketTimeoutRecv(3000);
					}else{
						_tout.add(2000);
					}
					state(ParseTout);
					return NOK;
				case BAD:
					edbg("");
					return BAD;
				case YIELD:
					return OK;
			}
			if(reader().isError()){
				delete pcmd; pcmd = NULL;
				logger.inFlush();
				state(Execute);
				writer().push(Writer::putStatus);
				break;
			}
		case ExecutePrepare:
			logger.inFlush();
			idbg("PrepareExecute");
			pcmd->execute(*this);
			state(Execute);
		case Execute:
			//idbg("Execute");
			switch((rc = writer().run())){
				case NOK:
					if(socketHasPendingSend()){
						socketTimeoutSend(3000);
						state(ExecuteIOTout);
					}/*else if(socketHasPendingRecv()){
						socketTimeoutRecv(3000);
						state(ExecuteIOTout);
					}*/else{
						_tout.add(2000);
						state(ExecuteTout);
					}
					return NOK;
				case OK:
					if(state() != IdleExecute){
						delete pcmd; pcmd = NULL;
						state(ParsePrepare); rc = OK;
					}else{
						state(Parse); rc = OK;
					}
				case YIELD:
					return OK;
				default:
					edbg("rc = "<<rc);
					return rc;
			}
			break;
		case IdleExecute:
			//idbg("IdleExecute");
			if(sevs & frame::OUTDONE){
				state(Execute);
				return OK;
			}return NOK;
		case Connect:
			switch(socketConnect(aiit)){
				case BAD:
					if(++aiit){
						state(Connect);
						return OK;
					}
					return BAD;
				case OK:
					idbg("");
					state(Init);
					//idbg("");
					return NOK;//for register
				case NOK:
					state(ConnectWait);
					idbg("");
					return NOK;
			}
			break;
		case ConnectWait:
			state(Init);
			//delete(paddr); paddr = NULL;
			break;
		case ParseTout:
			if(sevs & frame::INDONE){
				state(Parse);
				return OK;
			}
			return NOK;
		case ExecuteIOTout:
			idbg("State: ExecuteTout ");
			if(sevs & frame::OUTDONE){
				state(Execute);
				return OK;
			}
		case ExecuteTout:
			return NOK;
	}
	return OK;
}

//prepare the reader and the writer for a new command
void Connection::prepareReader(){
	writer().clear();
	reader().clear();
	//logger.outFlush();
	reader().push(&Reader::checkChar, protocol::Parameter('\n'));
	reader().push(&Reader::checkChar, protocol::Parameter('\r'));
	reader().push(&Reader::fetchKey<Reader, Connection, AtomFilter, Command>, protocol::Parameter(this, 64));
	reader().push(&Reader::checkChar, protocol::Parameter(' '));
	reader().push(&Reader::fetchFilteredString<TagFilter>, protocol::Parameter(&writer().tag(),64));
}

void Connection::dynamicExecute(DynamicPointer<> &_dp){
	wdbg("Received unknown signal on ipcservice");
}

void Connection::dynamicExecute(DynamicPointer<RemoteListMessage> &_rmsgptr){
	idbg("");
	if(_rmsgptr->requid && _rmsgptr->requid != reqid){
		idbg("RemoteListMessage signal rejected");
		return;
	}
	idbg("");
	newRequestId();//prevent multiple responses with the same id
	if(pcmd){
		int rv;
		if(!_rmsgptr->err){
			rv = pcmd->receiveData((void*)_rmsgptr->ppthlst, -1, 0, ObjectUidT(), &_rmsgptr->conid);
			_rmsgptr->ppthlst = NULL;
		}else{
			rv = pcmd->receiveError(_rmsgptr->err, ObjectUidT(), &_rmsgptr->conid);
		}
		switch(rv){
			case BAD:
				idbg("");
				break;
			case OK:
				idbg("");
				if(state() == ParseTout){
					state(Parse);
				}
				if(state() == ExecuteTout){
					state(Execute);
				}
				break;
			case NOK:
				idbg("");
				state(IdleExecute);
				break;
		}
	}
}
void Connection::dynamicExecute(DynamicPointer<FetchSlaveMessage> &_rmsgptr){
	idbg("");
	if(_rmsgptr->requid && _rmsgptr->requid != reqid) return;
	idbg("");
	newRequestId();//prevent multiple responses with the same id
	if(pcmd){
		int rv;
		if(_rmsgptr->filesz >= 0){
			idbg("");
			rv = pcmd->receiveNumber(_rmsgptr->filesz, 0, _rmsgptr->msguid, &_rmsgptr->conid);
		}else{
			idbg("");
			rv = pcmd->receiveError(-1, _rmsgptr->msguid, &_rmsgptr->conid);
		}
		switch(rv){
			case BAD:
				idbg("");
				break;
			case OK:
				idbg("");
				if(state() == ParseTout){
					state(Parse);
				}
				if(state() == ExecuteTout){
					state(Execute);
				}
				break;
			case NOK:
				idbg("");
				state(IdleExecute);
				break;
		}
	}
}
void Connection::dynamicExecute(DynamicPointer<SendStringMessage> &_rmsgptr){
}
void Connection::dynamicExecute(DynamicPointer<SendStreamMessage> &_rmsgptr){
}
void Connection::dynamicExecute(DynamicPointer<InputStreamMessage> &_rmsgptr){
	idbg("");
	if(_rmsgptr->requid.first && _rmsgptr->requid.first != reqid){
		return;
	}
	idbg("");
	newRequestId();//prevent multiple responses with the same id
	if(pcmd){
		switch(pcmd->receiveInputStream(_rmsgptr->sptr, _rmsgptr->fileuid, 0, ObjectUidT(), NULL)){
			case BAD:
				idbg("");
				break;
			case OK:
				idbg("");
				if(state() == ParseTout){
					state(Parse);
				}
				if(state() == ExecuteTout){
					idbg("");
					state(Execute);
				}
				break;
			case NOK:
				idbg("");
				state(IdleExecute);
				break;
		}
	}
}
void Connection::dynamicExecute(DynamicPointer<OutputStreamMessage> &_rmsgptr){
	idbg("");
	if(_rmsgptr->requid.first && _rmsgptr->requid.first != reqid) return;
	idbg("");
	newRequestId();//prevent multiple responses with the same id
	if(pcmd){
		switch(pcmd->receiveOutputStream(_rmsgptr->sptr, _rmsgptr->fileuid, 0, ObjectUidT(), NULL)){
			case BAD:
				idbg("");
				break;
			case OK:
				idbg("");
				if(state() == ParseTout){
					state(Parse);
				}
				if(state() == ExecuteTout){
					state(Execute);
				}
				break;
			case NOK:
				idbg("");
				state(IdleExecute);
				break;
		}
	}
}
void Connection::dynamicExecute(DynamicPointer<InputOutputStreamMessage> &_rmsgptr){
	idbg("");
	if(_rmsgptr->requid.first && _rmsgptr->requid.first != reqid) return;
	idbg("");
	newRequestId();//prevent multiple responses with the same id
	if(pcmd){
		switch(pcmd->receiveInputOutputStream(_rmsgptr->sptr, _rmsgptr->fileuid, 0, ObjectUidT(), NULL)){
			case BAD:
				idbg("");
				break;
			case OK:
				idbg("");
				if(state() == ParseTout){
					state(Parse);
				}
				if(state() == ExecuteTout){
					state(Execute);
				}
				break;
			case NOK:
				idbg("");
				state(IdleExecute);
				break;
		}
	}
}
void Connection::dynamicExecute(DynamicPointer<StreamErrorMessage> &_rmsgptr){
	idbg("");
	if(_rmsgptr->requid.first && _rmsgptr->requid.first != reqid) return;
	idbg("");
	newRequestId();//prevent multiple responses with the same id
	if(pcmd){
		switch(pcmd->receiveError(_rmsgptr->errid, ObjectUidT(), NULL)){
			case BAD:
				idbg("");
				break;
			case OK:
				idbg("");
				if(state() == ParseTout){
					state(Parse);
				}
				if(state() == ExecuteTout){
					state(Execute);
				}
				break;
			case NOK:
				idbg("");
				state(IdleExecute);
				break;
		}
	}
}

}//namespace alpha
}//namespace concept