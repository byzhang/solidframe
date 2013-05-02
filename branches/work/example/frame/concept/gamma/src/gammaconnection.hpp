#ifndef CONCEPT_GAMMA_CONNECTION_HPP
#define CONCEPT_GAMMA_CONNECTION_HPP


#include "utility/dynamictype.hpp"
#include "utility/streampointer.hpp"
#include "utility/queue.hpp"
#include "utility/stack.hpp"

#include "core/tstring.hpp"
#include "core/common.hpp"

#include "gammareader.hpp"
#include "gammawriter.hpp"

#include "frame/aio/aiomultiobject.hpp"

#include <deque>

namespace solid{
class SocketAddress;
class InputStream;
class OutputStream;
class InputOutputStream;
}


using solid::ulong;
using solid::uint32;

namespace concept{

class Manager;

//signals
struct InputStreamMessage;
struct OutputStreamMessage;
struct InputOutputStreamMessage;
struct StreamErrorMessage;


namespace gamma{

class Service;
class Command;

//signals:
struct SocketMoveMessage;

class Logger: public solid::protocol::text::Logger{
protected:
	virtual void doInFlush(const char*, unsigned);
	virtual void doOutFlush(const char*, unsigned);
};

struct SocketData{
	SocketData(uint _sid):w(_sid), r(_sid, w), /*evs(0),*/ pcmd(NULL){
	}
	Writer		w;
	Reader		r;
	//ulong		evs;
	Command		*pcmd;
};

//! Alpha connection implementing the alpha protocol resembling somehow the IMAP protocol
/*!
	It uses a reader and a writer to implement a state machine for the 
	protocol communication. 
*/
class Connection: public solid::Dynamic<Connection, solid::frame::aio::MultiObject>{
	typedef solid::DynamicExecuter<void, Connection>	DynamicExecuterT;
public:
	enum{
		SocketRegister,
		SocketInit,
		SocketBanner,
		SocketParsePrepare,
		SocketParse,
		SocketIdleParse,
		SocketIdleWait,
		SocketIdleDone,
		SocketExecutePrepare,
		SocketExecute,
		SocketParseWait,
		SocketUnregister,
		SocketLeave
	};
	
	static void initStatic(Manager &_rm);
	static void dynamicRegister();
	
	static Connection &the();
	
	Connection(const solid::SocketDevice &_rsd);
	
	~Connection();
	
	/*virtual*/ bool notify(solid::DynamicPointer<solid::frame::Message> &_rmsgptr);
	
	//! The implementation of the protocol's state machine
	/*!
		The method will be called within a foundation::SelectPool by an
		foundation::aio::Selector.
	*/
	int execute(ulong _sig, solid::TimeSpec &_tout);
	
	//! creator method for new commands
	Command* create(const String& _name, Reader &_rr);
	
	//! Generate a new request id.
	/*!
		Every request has an associated id.
		We want to prevent receiving unexpected responses.
		
		The commands that are not responses - like those received in idle state, come with
		the request id equal to zero so this must be skiped.
	*/
	uint32 newRequestId(int _pos = -1);
	bool   isRequestIdExpected(uint32 _v, int &_rpos)const;
	void   deleteRequestId(uint32 _v);
	
	SocketData &socketData(const uint _sid);
	
	void dynamicExecute(solid::DynamicPointer<> &_dp);
	void dynamicExecute(solid::DynamicPointer<InputStreamMessage> &_rmsgptr);
	void dynamicExecute(solid::DynamicPointer<OutputStreamMessage> &_rmsgptr);
	void dynamicExecute(solid::DynamicPointer<InputOutputStreamMessage> &_rmsgptr);
	void dynamicExecute(solid::DynamicPointer<StreamErrorMessage> &_rmsgptr);
	void dynamicExecute(solid::DynamicPointer<SocketMoveMessage> &_rmsgptr);
	
	void appendContextString(std::string &_str);
private:
	void state(int _st){
		st = _st;
	}
	int state()const{
		return st;
	}
	
	int executeSocket(const uint _sid, const solid::TimeSpec &_tout);
	
	void prepareReader(const uint _sid);
	
	Command* doCreateSlave(const String& _name, Reader &_rr);
	Command* doCreateMaster(const String& _name, Reader &_rr);
	
	static void doInitStaticSlave(Manager &_rm);
	static void doInitStaticMaster(Manager &_rm);
	
	void pushExecQueue(uint _sid, uint32 _evs);
	uint popExecQueue();
	
	int doSocketPrepareBanner(const uint _sid, SocketData &_rsd);
	void doSocketPrepareParse(const uint _sid, SocketData &_rsd);
	int doSocketParse(const uint _sid, SocketData &_rsd, const bool _isidle = false);
	int doSocketExecute(const uint _sid, SocketData &_rsd, const int _state = 0);
	void doSendSocketMessage(const uint _sid);

private:
	typedef std::vector<SocketData*>				SocketDataVectorT;
	typedef std::vector<std::pair<uint32, int> >	RequestIdVectorT;
	struct StreamData{
		solid::StreamPointer<solid::InputStream>		pis;
		solid::StreamPointer<solid::OutputStream>		pos;
		solid::StreamPointer<solid::InputOutputStream>	pios;
	};
	typedef std::deque<StreamData>					StreamDataVectorT;
	typedef solid::Stack<uint32>					UIntStackT;

private:
	bool				isslave;
	int					st;
	uint32				crtreqid;
	DynamicExecuterT	dr;
	SocketDataVectorT	sdv;
	RequestIdVectorT	ridv;
	StreamDataVectorT	streamv;
	UIntStackT			freestk;
};



inline SocketData& Connection::socketData(uint _sid){
	return *sdv[_sid];
}

}//namespace gamma
}//namespace concept

#endif
