#ifndef EXAMPLE_CONSENSUS_SERVEROBJECT_HPP
#define EXAMPLE_CONSENSUS_SERVEROBJECT_HPP

#include <string>

#include "consensus/server/consensusobject.hpp"

struct StoreRequest;
struct FetchRequest;
struct EraseRequest;


struct ServerConfiguration: solid::consensus::server::Configuration{
	typedef std::vector<std::string>	StringVectorT;
	
	bool init(int _ipc_port);
	const std::string& errorString()const{
		return err;
	}
	std::ostream& print(std::ostream &_ros)const;
	StringVectorT	addrstrvec;
private:
	std::string		err;
};

std::ostream& operator<<(std::ostream &_ros, const ServerConfiguration &_rsp);


class ServerObject: public solid::Dynamic<ServerObject, solid::consensus::server::Object>{
	typedef solid::DynamicExecuter<void, ServerObject, solid::DynamicDefaultPointerStore, int>	DynamicExecuterExT;
public:
	static void dynamicRegister();
	static void registerMessages(solid::frame::ipc::Service &_ripcsvc);
	ServerObject(solid::frame::ipc::Service &_ripcsvc, solid::DynamicPointer<solid::consensus::server::Configuration> &_rcfgptr);
	~ServerObject();
	void dynamicExecute(solid::DynamicPointer<> &_dp, int);
	
	void dynamicExecute(solid::DynamicPointer<StoreRequest> &_rsig, int);
	void dynamicExecute(solid::DynamicPointer<FetchRequest> &_rsig, int);
	void dynamicExecute(solid::DynamicPointer<EraseRequest> &_rsig, int);
protected:
	/*virtual*/ void doSendMessage(solid::DynamicPointer<solid::frame::Message> &_rmsgptr, const solid::SocketAddressInet4 &_raddr);
private:
	/*virtual*/ void accept(solid::DynamicPointer<solid::consensus::WriteRequestMessage> &_rmsgptr);
	/*virtual*/ int recovery();
private:
	DynamicExecuterExT				exeex;
	solid::frame::ipc::Service		&ripcsvc;
};

#endif

