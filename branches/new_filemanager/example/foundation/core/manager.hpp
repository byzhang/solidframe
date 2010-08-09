/* Declarations file manager.hpp
	
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

#ifndef TESTMANAGER_HPP
#define TESTMANAGER_HPP

#include "foundation/manager.hpp"
#include "common.hpp"


namespace foundation{
namespace ipc{
class Service;
}
namespace file{
class Manager;
}
}


namespace concept{

class Service;
class Visitor;
class SignalExecuter;
struct FileManagerController;

//! A proof of concept server
/*!
	This is a proof of concept server and should be taken accordingly.
	
	It tries to give an ideea of how the foundation::Manager should be used.
	
	To complicate things a little, it allso keeps a map service name to service,
	so that services can be more naturally accessed by their names.
*/
class Manager: public foundation::Manager{
public:
	Manager();
	
	~Manager();
	
	//! Overwrite the foundation::Manager::the to give access to the extended interface
	static Manager& the(){return static_cast<Manager&>(foundation::Manager::the());}
	
	//! Starts a specific service or all services
	/*!
		\param _which If not null it represents the name of the service
	*/
	int start(const char *_which = NULL);
	
	//! Stops a specific service or all services
	/*!
		\param _which If not null it represents the name of the service
	*/
	int stop(const char *_which = NULL);
	
	//! Registers a service given its name and a pointer to a service.
	int insertService(const char* _nm, Service* _psrvc);
	
	int signalService(const char *_nm, DynamicPointer<foundation::Signal> &_rsig);
	
	//! Get the id of the signal executer specialized for reading
	void readSignalExecuterUid(ObjectUidT &_ruid);
	
	//! Get the id of the signal executer specialized for writing
	void writeSignalExecuterUid(ObjectUidT &_ruid);
	
	//! Removes a service
	void removeService(Service *_psrvc);
	void removeService(foundation::Service *_psrvc);
	
	//! Visit all services
	int visitService(const char* _nm, Visitor &_roi);
	
	int insertIpcTalker(
		const AddrInfoIterator &_rai,
		const char *_node = NULL,
		const char *_svc = NULL
	);
	
	//! Pushes an object into a specific pool.
	template <class J>
	void pushJob(J *_pj, int _pos = 0);
	
	foundation::ipc::Service 	&ipc();
	foundation::file::Manager	&fileManager();
private:
	friend struct FileManagerController;
	void removeFileManager();
private:
	friend class SignalExecuter;
	struct Data;
	Data	&d;
};

}//namespace concept
#endif