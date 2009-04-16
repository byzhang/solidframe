/* Implementation file aioobject.cpp
	
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

#include "foundation/aio/aioobject.hpp"
#include "foundation/aio/aiosingleobject.hpp"
#include "foundation/aio/aiomultiobject.hpp"
#include "foundation/aio/src/aiosocket.hpp"
#include "system/cassert.hpp"

#include <memory>
#include <cstring>

namespace foundation{

namespace aio{
//======================== aio::Object ==================================

Object::SocketStub::~SocketStub(){
	delete psock;
}

/*virtual*/ Object::~Object(){
}

/*virtual*/ int Object::accept(foundation::Visitor &_roi){
	return BAD;
}

void Object::pushRequest(uint _pos, uint _req){
	if(pstubs[_pos].request > SocketStub::Response) return;
	pstubs[_pos].request = _req;
	*reqpos = _pos; ++reqpos;
}

inline void Object::doPushResponse(uint32 _pos){
	if(pstubs[_pos].request != SocketStub::Response){
		*respos = _pos; ++respos;
		cassert(respos - resbeg == 1);
		pstubs[_pos].request = SocketStub::Response;
	}
}
inline void Object::doPopTimeout(uint32 _pos){
	cassert(toutpos != toutbeg);
	--toutpos;
	toutbeg[pstubs[_pos].toutpos] = *toutpos;
#ifdef DEBUG
	*toutpos = -1;
#endif
	pstubs[_pos].toutpos = -1;
}

void Object::doAddSignaledSocketFirst(uint _pos, uint _evs){
	cassert(_pos < stubcp || pstubs[_pos].psock);
	pstubs[_pos].chnevents = _evs;
	if(pstubs[_pos].toutpos >= 0){
		doPopTimeout(_pos);
	}
	doPushResponse(_pos);
}
void Object::doAddSignaledSocketNext(uint _pos, uint _evs){
	cassert(_pos < stubcp || pstubs[_pos].psock);
	pstubs[_pos].chnevents |= _evs;
	if(pstubs[_pos].toutpos >= 0){
		doPopTimeout(_pos);
	}
	doPushResponse(_pos);
}
void Object::doAddTimeoutSockets(const TimeSpec &_timepos){
	//add all timeouted stubs to responses
	const int32 *pend(toutpos);
	for(int32 *pit(toutbeg); pit != pend;){
		cassert(pstubs[*pit].psock);
		if(pstubs[*pit].timepos <= _timepos){
			pstubs[*pit].chnevents |= TIMEOUT;
			doPushResponse(*pit);
			--pend;
			*pit = *pend;
			//TODO: add some checking
			//pstubs[*pit].toutpos = pit - toutbeg;
		}else ++pit;
	}
}

void Object::doPrepare(TimeSpec *_ptimepos){
	ptimepos = _ptimepos;
}
void Object::doUnprepare(){
	ptimepos = NULL;
}

void Object::doClearRequests(){
	reqpos = reqbeg;
}
void Object::doClearResponses(){
	respos = resbeg;
}

//========================== aio::SingleObject =============================

SingleObject::SingleObject(Socket *_psock):
	Object(&stub, 1, &req, &res, &tout), req(-1), res(-1), tout(-1)
{
	stub.psock = _psock;
}
SingleObject::SingleObject(const SocketDevice &_rsd):
	Object(&stub, 1, &req, &res, &tout), req(-1), res(-1), tout(-1)
{
	if(_rsd.ok()){
		socketSet(_rsd);
	}
}
SingleObject::~SingleObject(){
}
bool SingleObject::socketOk()const{
	cassert(stub.psock);
	return stub.psock->ok();
}

int SingleObject::socketAccept(SocketDevice &_rsd){
	int rv = stub.psock->accept(_rsd);
	if(rv == NOK){
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}

int SingleObject::socketConnect(const AddrInfoIterator& _rai){
	cassert(stub.psock);
	int rv = stub.psock->connect(_rai);
	if(rv == NOK){
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}


int SingleObject::socketSend(const char* _pb, uint32 _bl, uint32 _flags){
	//ensure that we dont have double request
	//cassert(stub.request <= SocketStub::Response);
	cassert(stub.psock);
	int rv = stub.psock->send(_pb, _bl);
	if(rv == NOK){
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}

int SingleObject::socketSendTo(const char* _pb, uint32 _bl, const SockAddrPair &_sap, uint32 _flags){
	//ensure that we dont have double request
	//cassert(stub.request <= SocketStub::Response);
	cassert(stub.psock);
	int rv = stub.psock->sendTo(_pb, _bl, _sap);
	if(rv == NOK){
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}

int SingleObject::socketRecv(char *_pb, uint32 _bl, uint32 _flags){
	//ensure that we dont have double request
	//cassert(stub.request <= SocketStub::Response);
	cassert(stub.psock);
	int rv = stub.psock->recv(_pb, _bl);
	if(rv == NOK){
		//stub.timepos.set(0xffffffff, 0xffffffff);
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}

int SingleObject::socketRecvFrom(char *_pb, uint32 _bl, uint32 _flags){
	//ensure that we dont have double request
	//cassert(stub.request <= SocketStub::Response);
	cassert(stub.psock);
	int rv = stub.psock->recvFrom(_pb, _bl);
	if(rv == NOK){
		//stub.timepos.set(0xffffffff, 0xffffffff);
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}
uint32 SingleObject::socketRecvSize()const{
	return stub.psock->recvSize();
}
const SockAddrPair &SingleObject::socketRecvAddr() const{
	return stub.psock->recvAddr();
}
const uint64& SingleObject::socketSendCount()const{
	return stub.psock->sendCount();
}
const uint64& SingleObject::socketRecvCount()const{
	return stub.psock->recvCount();
}
bool SingleObject::socketHasPendingSend()const{
	return stub.psock->isSendPending();
}
bool SingleObject::socketHasPendingRecv()const{
	return stub.psock->isRecvPending();
}
int SingleObject::socketLocalAddress(SocketAddress &_rsa)const{
	return stub.psock->localAddress(_rsa);
}
int SingleObject::socketRemoteAddress(SocketAddress &_rsa)const{
	return stub.psock->remoteAddress(_rsa);
}
void SingleObject::socketTimeout(const TimeSpec &_crttime, ulong _addsec, ulong _addnsec){
	stub.timepos = _crttime;
	stub.timepos.add(_addsec, _addnsec);
	if(stub.toutpos < 0){
		stub.toutpos = 0;
		*toutpos = 0;
		++toutpos;
	}
	if(*ptimepos > stub.timepos){
		*ptimepos = stub.timepos;
	}
}
uint32 SingleObject::socketEvents()const{
	return stub.chnevents;
}
void SingleObject::socketErase(){
	delete stub.psock;
	stub.psock = NULL;
}
int SingleObject::socketSet(Socket *_psock){
	cassert(!stub.psock);
	stub.psock = _psock;
	return 0;
}
int SingleObject::socketSet(const SocketDevice &_rsd){
	cassert(!stub.psock);
	if(_rsd.ok()){
		Socket::Type tp;
		if(_rsd.isListening()){
			tp = Socket::ACCEPTOR;
		}else if(_rsd.type() == AddrInfo::Stream){
			tp = Socket::CHANNEL;
		}else if(_rsd.type() == AddrInfo::Datagram){
			tp = Socket::STATION;
		}else return -1;
		stub.psock = new Socket(tp, _rsd);
		return 0;
	}
	return -1;
}

void SingleObject::socketRequestRegister(){
	//ensure that we dont have double request
	cassert(stub.request <= SocketStub::Response);
	stub.request = SocketStub::RegisterRequest;
	*reqpos = 0;
	++reqpos;
}
void SingleObject::socketRequestUnregister(){
	//ensure that we dont have double request
	cassert(stub.request <= SocketStub::Response);
	stub.request = SocketStub::UnregisterRequest;
	*reqpos = 0;
	++reqpos;
}

int SingleObject::socketState()const{
	return stub.state;
}
void SingleObject::socketState(int _st){
	stub.state = _st;
}

bool SingleObject::socketIsSecure()const{
	return stub.psock->isSecure();
}

SecureSocket* SingleObject::socketSecureSocket(){
	return stub.psock->secureSocket();
}
void SingleObject::socketSecureSocket(SecureSocket *_pss){
	stub.psock->secureSocket(_pss);
}

int SingleObject::socketSecureAccept(){
	int rv = stub.psock->secureAccept();
	if(rv == NOK){
		//stub.timepos.set(0xffffffff, 0xffffffff);
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}
int SingleObject::socketSecureConnect(){
	int rv = stub.psock->secureConnect();
	if(rv == NOK){
		//stub.timepos.set(0xffffffff, 0xffffffff);
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}

//=========================== aio::MultiObject =============================

MultiObject::MultiObject(Socket *_psock):Object(NULL, 0, NULL, NULL, NULL){
	if(_psock)
		socketInsert(_psock);
}
MultiObject::MultiObject(const SocketDevice &_rsd):Object(NULL, 0, NULL, NULL, NULL){
	socketInsert(_rsd);
}
MultiObject::~MultiObject(){
	for(uint i = 0; i < stubcp; ++i){
		delete pstubs[i].psock;
		pstubs[i].psock = NULL;
	}
	char *oldbuf = reinterpret_cast<char*>(pstubs);
	delete []oldbuf;
}
bool MultiObject::socketOk(uint _pos)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->ok();
}

int MultiObject::socketAccept(uint _pos, SocketDevice &_rsd){
	cassert(_pos < stubcp);
	int rv = pstubs[_pos].psock->accept(_rsd);
	if(rv == NOK){
		pushRequest(0, SocketStub::IORequest);
	}
	return rv;
}

int MultiObject::socketConnect(uint _pos, const AddrInfoIterator& _rai){
	cassert(_pos < stubcp);
	int rv = pstubs[_pos].psock->connect(_rai);
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}

const SockAddrPair &MultiObject::socketRecvAddr(uint _pos) const{
	return pstubs[_pos].psock->recvAddr();
}

int MultiObject::socketSend(
	uint _pos,
	const char* _pb,
	uint32 _bl,
	uint32 _flags
){
	cassert(_pos < stubcp);
	int rv = pstubs[_pos].psock->send(_pb, _bl, _flags);
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}

int MultiObject::socketSendTo(
	uint _pos,
	const char* _pb,
	uint32 _bl,
	const SockAddrPair &_sap,
	uint32 _flags
){
	cassert(_pos < stubcp);
	int rv = pstubs[_pos].psock->sendTo(_pb, _bl, _sap, _flags);
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}

int MultiObject::socketRecv(
	uint _pos,
	char *_pb,
	uint32 _bl,
	uint32 _flags
){
	cassert(_pos < stubcp);
	int rv = pstubs[_pos].psock->recv(_pb, _bl, _flags);
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}
int MultiObject::socketRecvFrom(
	uint _pos,
	char *_pb,
	uint32 _bl,
	uint32 _flags
){
	cassert(_pos < stubcp);
	int rv = pstubs[_pos].psock->recvFrom(_pb, _bl, _flags);
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}
uint32 MultiObject::socketRecvSize(uint _pos)const{	
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->recvSize();
}
const uint64& MultiObject::socketSendCount(uint _pos)const{	
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->sendCount();
}
const uint64& MultiObject::socketRecvCount(uint _pos)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->recvCount();
}
bool MultiObject::socketHasPendingSend(uint _pos)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->isSendPending();
}
bool MultiObject::socketHasPendingRecv(uint _pos)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->isRecvPending();
}
int MultiObject::socketLocalAddress(uint _pos, SocketAddress &_rsa)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->localAddress(_rsa);
}
int MultiObject::socketRemoteAddress(uint _pos, SocketAddress &_rsa)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].psock->remoteAddress(_rsa);
}
void MultiObject::socketTimeout(
	uint _pos,
	const TimeSpec &_crttime,
	ulong _addsec,
	ulong _addnsec
){
	pstubs[_pos].timepos = _crttime;
	pstubs[_pos].timepos.add(_addsec, _addnsec);
	if(pstubs[_pos].toutpos < 0){
		pstubs[_pos].toutpos = toutpos - toutbeg;
		*toutpos = _pos;
		++toutpos;
	}
	if(*ptimepos > pstubs[_pos].timepos){
		*ptimepos = pstubs[_pos].timepos;
	}
}
uint32 MultiObject::socketEvents(uint _pos)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].chnevents;
}
void MultiObject::socketErase(uint _pos){
	cassert(_pos < stubcp);
	delete pstubs[_pos].psock;
	pstubs[_pos].reset();
}
int MultiObject::socketInsert(Socket *_psock){
	cassert(_psock);
	uint pos = newStub();
	pstubs[pos].psock = _psock;
	return pos;
}
int MultiObject::socketInsert(const SocketDevice &_rsd){
	if(_rsd.ok()){
		uint pos = newStub();
		Socket::Type tp;
		if(_rsd.isListening()){
			tp = Socket::ACCEPTOR;
		}else if(_rsd.type() == AddrInfo::Stream){
			tp = Socket::CHANNEL;
		}else if(_rsd.type() == AddrInfo::Datagram){
			tp = Socket::STATION;
		}else return -1;
		
		pstubs[pos].psock = new Socket(tp, _rsd);
		return pos;
	}
	return -1;
}
void MultiObject::socketRequestRegister(uint _pos){
	cassert(_pos < stubcp);
	cassert(pstubs[_pos].request <= SocketStub::Response);
	pstubs[_pos].request = SocketStub::RegisterRequest;
	*reqpos = _pos;
	++reqpos;
}
void MultiObject::socketRequestUnregister(uint _pos){
	cassert(_pos < stubcp);
	cassert(pstubs[_pos].request <= SocketStub::Response);
	pstubs[_pos].request = SocketStub::UnregisterRequest;
	*reqpos = _pos;
	++reqpos;
}
int MultiObject::socketState(unsigned _pos)const{
	cassert(_pos < stubcp);
	return pstubs[_pos].state;
}
void MultiObject::socketState(unsigned _pos, int _st){
	cassert(_pos < stubcp);
	pstubs[_pos].state = _st;
}
inline uint MultiObject::dataSize(uint _cp){
	return _cp * sizeof(SocketStub)//the stubs
			+ _cp * sizeof(int32)//the requests
			+ _cp * sizeof(int32)//the reqponses
			+ _cp * sizeof(int32);//the timeouts
}
void MultiObject::reserve(uint _cp){
	cassert(_cp > stubcp);
	//first we allocate the space
	char* buf = new char[dataSize(_cp)];
	SocketStub	*pnewstubs(reinterpret_cast<SocketStub*>(buf));
	int32		*pnewreqbeg(reinterpret_cast<int32*>(buf + sizeof(SocketStub) * _cp));
	int32		*pnewresbeg(reinterpret_cast<int32*>(buf + sizeof(SocketStub) * _cp + _cp * sizeof(int32)));
	int32		*pnewtoutbeg(reinterpret_cast<int32*>(buf + sizeof(SocketStub) * _cp + 2 * _cp * sizeof(int32)));
	if(pstubs){
		//then copy all the existing stubs:
		memcpy(pnewstubs, (void*)pstubs, sizeof(SocketStub) * stubcp);
		//then the requests
		memcpy(pnewreqbeg, reqbeg, sizeof(int32) * stubcp);
		//then the responses
		memcpy(pnewresbeg, resbeg, sizeof(int32) * stubcp);
		//then the timeouts
		memcpy(pnewtoutbeg, resbeg, sizeof(int32) * stubcp);
		char *oldbuf = reinterpret_cast<char*>(pstubs);
		delete []oldbuf;
		pstubs = pnewstubs;
	}else{
		reqpos = reqbeg = NULL;
		respos = resbeg = NULL;
		toutpos = toutbeg = NULL;
	}
	for(int i = _cp - 1; i >= (int)stubcp; --i){
		pnewstubs[i].reset();
		posstk.push(i);
	}
	pstubs = pnewstubs;
	stubcp = _cp;
	reqpos = pnewreqbeg + (reqpos - reqbeg);
	reqbeg = pnewreqbeg;
	respos = pnewresbeg + (respos - resbeg);
	resbeg = pnewresbeg;
	toutpos = pnewtoutbeg + (toutpos - toutbeg);
	toutbeg = pnewtoutbeg;
}
uint MultiObject::newStub(){
	if(posstk.size()){
		uint pos = posstk.top();
		posstk.pop();
		return pos;
	}
	reserve(stubcp + 4);
	uint pos = posstk.top();
	posstk.pop();
	return pos;
}

bool MultiObject::socketIsSecure(uint _pos)const{
	return pstubs[_pos].psock->isSecure();
}

SecureSocket* MultiObject::socketSecureSocket(unsigned _pos){
	return pstubs[_pos].psock->secureSocket();
}
void MultiObject::socketSecureSocket(unsigned _pos, SecureSocket *_pss){
	pstubs[_pos].psock->secureSocket(_pss);
}

int MultiObject::socketSecureAccept(unsigned _pos){
	int rv = pstubs[_pos].psock->secureAccept();
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}
int MultiObject::socketSecureConnect(unsigned _pos){
	int rv = pstubs[_pos].psock->secureConnect();
	if(rv == NOK){
		pushRequest(_pos, SocketStub::IORequest);
	}
	return rv;
}

}//namespace aio;
}//namespace foundation