/* Implementation file proxymulticonnection.cpp
	
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

#include "clientserver/tcp/channel.hpp"

#include "core/server.hpp"
#include "proxy/proxyservice.hpp"
#include "proxymulticonnection.hpp"
#include "system/socketaddress.hpp"
#include "system/debug.hpp"
#include "system/timespec.hpp"

namespace cs=clientserver;
static char	*hellostr = "Welcome to proxy service!!!\r\n"; 

namespace test{

namespace proxy{

MultiConnection::MultiConnection(cs::tcp::Channel *_pch, const char *_node, const char *_srv): 
									BaseTp(_pch),
									//bend(bbeg + BUFSZ),brpos(bbeg),bwpos(bbeg),
									pai(NULL),b(false){
	if(_node){
		pai = new AddrInfo(_node, _srv);
		it = pai->begin();
		//state(CONNECT);
	}else{
		//state(INIT);
	}
	bp = be = NULL;
	state(READ_ADDR);
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

MultiConnection::~MultiConnection(){
	test::Server &rs = test::Server::the();
	rs.service(*this).removeConnection(*this);
	delete pai;
}

int MultiConnection::execute(ulong _sig, TimeSpec &_tout){
	idbg("time.sec "<<_tout.seconds()<<" time.nsec = "<<_tout.nanoSeconds());
	if(_sig & (cs::TIMEOUT | cs::ERRDONE)){
		idbg("connecton timeout or error");
		return BAD;
	}
	if(signaled()){
		test::Server &rs = test::Server::the();
		{
		Mutex::Locker	lock(rs.mutex(*this));
		ulong sm = grabSignalMask();
		if(sm & cs::S_KILL) return BAD;
		}
	}
	switch(state()){
		case READ_ADDR:
			idbgx(Dbg::tcp, "READ_ADDR");
		case READ_PORT:
			idbgx(Dbg::tcp, "READ_PORT");
			return doReadAddress();
		case REGISTER_CONNECTION:{
			idbgx(Dbg::tcp, "REGISTER_CONNECTION");
			pai = new AddrInfo(addr.c_str(), port.c_str());
			it = pai->begin();
			channelAdd(cs::tcp::Channel::create(it));
			channelRegisterRequest(1);
			state(CONNECT);
			}return NOK;
		case CONNECT://connect the other end:
			idbgx(Dbg::tcp, "CONNECT");
			switch(channelConnect(1, it)){
				case BAD:
					idbgx(Dbg::tcp, "BAD");
					return BAD;
				case OK:
					state(SEND_REMAINS);
					idbgx(Dbg::tcp, "OK");
					return OK;
				case NOK:
					state(CONNECT_TOUT);
					idbgx(Dbg::tcp, "NOK");
					return NOK;
			};
		case CONNECT_TOUT:{
			idbgx(Dbg::tcp, "CONNECT_TOUT");
			uint32 evs = channelEvents(1);
			if(!evs || !(evs & cs::OUTDONE)) return BAD;
			
			state(SEND_REMAINS);
		}
		case SEND_REMAINS:
			idbgx(Dbg::tcp, "SEND_REMAINS");
			if(bp != be){
				switch(channelSend(1, bp, be - bp)){
					case BAD:
						idbgx(Dbg::tcp, "BAD");
						return BAD;
					case OK:
						state(PROXY);
						idbgx(Dbg::tcp, "OK");
						break;
					case NOK:
						stubs[0].recvbuf.usecnt = 1;
						state(PROXY);
						idbgx(Dbg::tcp, "NOK");
						return NOK;
				}
			}
		case PROXY:
			idbgx(Dbg::tcp, "PROXY");
			return doProxy(_tout);
	}
	return NOK;
}
int MultiConnection::doReadAddress(){
	if(bp and !be) return doRefill();
	char *bb = bp;
	switch(state()){
		case READ_ADDR:
			idbgx(Dbg::tcp, "READ_ADDR");
			while(bp != be){
				if(*bp == ' ') break;
				++bp;
			}
			addr.append(bb, bp - bb);
			if(bp == be) return doRefill();
			if(addr.size() > 64) return BAD;
			//*bp == ' '
			++bp;
			state(READ_PORT);
		case READ_PORT:
			idbgx(Dbg::tcp, "READ_PORT");
			bb = bp;
			while(bp != be){
				if(*bp == '\n') break;
				++bp;
			}
			port.append(bb, bp - bb);
			if(bp == be) return doRefill();
			if(port.size() > 64) return BAD;
			if(port.size() && port[port.size() - 1] == '\r'){
				port.resize(port.size() - 1);
			}
			//*bp == '\n'
			++bp;
			state(REGISTER_CONNECTION);
			return OK;
	}
	cassert(false);
	return BAD;
}
int MultiConnection::doProxy(const TimeSpec &_tout){
	uint32 evs0 = channelEvents(0);
	uint32 evs1 = channelEvents(1);
	if(evs0 & cs::TIMEOUT)return BAD;
	if(evs1 & cs::TIMEOUT)return BAD;
	if(evs0 & cs::OUTDONE) stubs[1].recvbuf.usecnt = 0;
	if(evs1 & cs::OUTDONE) stubs[0].recvbuf.usecnt = 0;
	if(evs0 & cs::INDONE){
		//send on 1
		stubs[0].recvbuf.usecnt = 1;
		switch(channelSend(1, stubs[0].recvbuf.data, channelRecvSize(0))){
			case BAD: return BAD;
			case OK: stubs[0].recvbuf.usecnt = 0;break;
			case NOK:
				channelTimeout(1, _tout, 30, 0);
				break;
		}
	}
	if(evs1 & cs::INDONE){
		//send on 0
		stubs[1].recvbuf.usecnt = 1;
		switch(channelSend(0, stubs[1].recvbuf.data, channelRecvSize(1))){
			case BAD: return BAD;
			case OK: stubs[1].recvbuf.usecnt = 0;break;
			case NOK:
				channelTimeout(0, _tout, 30, 0);
				break;
		}
	}
	if(stubs[0].recvbuf.usecnt == 0){
		switch(channelRecv(0, stubs[0].recvbuf.data, Buffer::Capacity)){
			case BAD:	return BAD;
			case OK:
				stubs[0].recvbuf.usecnt = 1;
				switch(channelSend(1, stubs[0].recvbuf.data, channelRecvSize(0))){
					case BAD: return BAD;
					case OK: stubs[0].recvbuf.usecnt = 0;break;
					case NOK:
						channelTimeout(1, _tout, 30, 0);
						break;
				}
				break;
			case NOK:
				channelTimeout(0, _tout, 30, 0);
				break;	
		}
	}
	if(stubs[1].recvbuf.usecnt == 0){
		switch(channelRecv(1, stubs[1].recvbuf.data, Buffer::Capacity)){
			case BAD:	return BAD;
			case OK:
				stubs[1].recvbuf.usecnt = 1;
				switch(channelSend(0, stubs[1].recvbuf.data, channelRecvSize(1))){
					case BAD: return BAD;
					case OK: stubs[1].recvbuf.usecnt = 0;break;
					case NOK:
						channelTimeout(0, _tout, 30, 0);
						break;
				}
				break;
			case NOK:
				channelTimeout(1, _tout, 30, 0);
				break;	
		}
	}
	if(channelRequestCount()){
		idbgx(Dbg::tcp, "NOK");
		return NOK;
	}
	idbgx(Dbg::tcp, "OK");
	return OK;
}


int MultiConnection::doRefill(){
	idbgx(Dbg::tcp, "");
	if(bp == NULL){//we need to issue a read
		switch(channelRecv(0, stubs[0].recvbuf.data, Buffer::Capacity)){
			case BAD:	return BAD;
			case OK:
				bp = stubs[0].recvbuf.data;
				be = stubs[0].recvbuf.data + channelRecvSize(0);
				return OK;
			case NOK:
				be = NULL;
				bp = stubs[0].recvbuf.data;
				idbgx(Dbg::tcp, "NOK");
				return NOK;
		}
	}
	if(be == NULL){
		if(channelEvents(0) & cs::INDONE){
			be = stubs[0].recvbuf.data + channelRecvSize(0);
		}else{
			idbgx(Dbg::tcp, "NOK");
			return NOK;
		}
	}
	return OK;
}

int MultiConnection::execute(){
	return BAD;
}

int MultiConnection::accept(cs::Visitor &_rov){
	return BAD;
}
}//namespace proxy
}//namespace test
