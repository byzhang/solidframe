/* Declarations file socketaddress.hpp
	
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

#ifndef SYSTEM_SOCKETADDRESS_HPP
#define SYSTEM_SOCKETADDRESS_HPP

#include <sys/un.h>
#include <arpa/inet.h>

#include "common.hpp"
#include "socketinfo.hpp"

#ifdef HAS_CPP11
#include <memory>
#else
#include "boost/shared_ptr.hpp"
#endif

struct SocketAddressInfo;
struct SocketDevice;
//struct sockaddr_in;
//struct sockaddr_in6;
//==================================================================
//! An interator for POSIX addrinfo (see man getaddrinfo)
/*!
	Usually it will hold all data needed for creating and connecting 
	a socket. Use ResolverData::begin() to get started.
*/
struct ResolveIterator{
	ResolveIterator();
	ResolveIterator& next();
	int family()const;
	int type()const;
	int protocol()const;
	size_t size()const;
	sockaddr* sockAddr()const;
	operator bool()const;
	ResolveIterator &operator++();
	bool operator==(const ResolveIterator &_rrit)const;
private:
	friend struct ResolveData;
	ResolveIterator(addrinfo *_pa);
	const addrinfo	*paddr;
};
//==================================================================
//! A shared pointer for POSIX addrinfo (see man getaddrinfo)
/*!
	Use synchronous_resolve to create ResolveData objects.
*/
struct ResolveData{
	enum Flags{
		CannonName = AI_CANONNAME,
		NumericHost = AI_NUMERICHOST,
		All	= AI_ALL,
		AddrConfig = AI_ADDRCONFIG,
		V4Mapped  = AI_V4MAPPED,
		NumericService = AI_NUMERICSERV
	};
	typedef ResolveIterator const_iterator;
	
	ResolveData();
	ResolveData(addrinfo *_pai);
	ResolveData(const ResolveData &_rai);
	
	
	~ResolveData();
	//! Get an iterator to he first resolved ip address
	const_iterator begin()const;
	const_iterator end()const;
	//! Check if the returned list of ip addresses is empty
	bool empty()const;
	void clear();
	ResolveData& operator=(const ResolveData &_rrd);
private:
	static void delete_addrinfo(void *_pv);
#ifdef HAS_CPP11
	typedef std::shared_ptr<addrinfo>	AddrInfoSharedPtrT;
#else
	typedef boost::shared_ptr<addrinfo>	AddrInfoSharedPtrT;
#endif
	AddrInfoSharedPtrT		aiptr;
};

ResolveData synchronous_resolve(const char *_node, const char *_service);
ResolveData synchronous_resolve(
	const char *_node, 
	const char *_service, 
	int _flags,
	int _family = -1,
	int _type = -1,
	int _proto = -1
);	

ResolveData synchronous_resolve(const char *_node, int _port);
ResolveData synchronous_resolve(
	const char *_node, 
	int _port,
	int _flags,
	int _family = -1,
	int _type = -1,
	int _proto = -1
);
//==================================================================
struct SocketAddress;
struct SocketAddressInet;
struct SocketAddressInet4;
struct SocketAddressInet6;
struct SocketAddressLocal;
#ifdef ON_WINDOWS
typedef int socklen_t;
#endif
//! A pair of a sockaddr pointer and a size
/*!
	It is a commodity structure, it will not allocate data
	for sockaddr pointer nor it will delete it. Use this 
	structure with SocketAddress and ResolveIterator
*/
struct SocketAddressStub{
	SocketAddressStub(sockaddr *_pa = NULL, size_t _sz = 0);
	
	SocketAddressStub(const ResolveIterator &_it);
	SocketAddressStub(const SocketAddress &_rsa);
	SocketAddressStub(const SocketAddressInet &_rsa);
	SocketAddressStub(const SocketAddressInet4 &_rsa);
	SocketAddressStub(const SocketAddressInet6 &_rsa);
	SocketAddressStub(const SocketAddressLocal &_rsa);
	
	SocketAddressStub& operator=(const ResolveIterator &_it);
	SocketAddressStub& operator=(const SocketAddress &_rsa);
	SocketAddressStub& operator=(const SocketAddressInet &_rsa);
	SocketAddressStub& operator=(const SocketAddressInet4 &_rsa);
	SocketAddressStub& operator=(const SocketAddressInet6 &_rsa);
	SocketAddressStub& operator=(const SocketAddressLocal &_rsa);
	
	operator const sockaddr*()const;
	
	void clear();
	
	SocketInfo::Family family()const;
	
	bool isInet4()const;
	bool isInet6()const;
	bool isLocal()const;
	
	socklen_t size()const;
	int port()const;
	
	const sockaddr	*sockAddr()const;
private:
	const sockaddr	*addr;
	socklen_t		sz;
};
//==================================================================
//! Holds a generic socket address
/*!
	On unix it can be either: inet_v4, inet_v6 or unix/local address 
*/
struct SocketAddress{
private:
	union AddrUnion{
		sockaddr		addr;
		sockaddr_in 	inaddr4;
		sockaddr_in6 	inaddr6;
		sockaddr_un		localaddr;
	};
public:
	enum {Capacity = sizeof(AddrUnion)};
	
	SocketAddress();
	SocketAddress(const SocketAddressStub &);
	SocketAddress(const char* _addr, int _port);
	SocketAddress(const char* _path);
	
	SocketAddress& operator=(const ResolveIterator &);
	SocketAddress& operator=(const SocketAddressStub &);
	
	SocketInfo::Family family()const;
	
	bool isInet4()const;
	bool isInet6()const;
	bool isLocal()const;
	
	bool isLoopback()const;
	bool isInvalid()const;
	
	bool empty()const;
	
	const socklen_t&	size()const;

	const sockaddr* sockAddr()const;
	operator const sockaddr*()const;
	//! Get the name associated to the address
	/*!
		Generates the string name associated to a specific address
		filling the given buffers. It is a wrapper for POSIX,
		getnameinfo.
		Usage:<br>
		<CODE>
		char			host[SocketAddress::HostNameCapacity];<br>
		char			port[SocketAddress::ServerNameCapacity];<br>
		SocketAddress	addr;<br>
		channel().localAddress(addr);<br>
		addr.name(<br>
			host,<br>
			SocketAddress::HostNameCapacity,<br>
			port,<br>
			SocketAddress::ServiceNameCapacity,<br>
			SocketAddress::NumericService<br>
		);<br>
		</CODE>
		\retval true for success, false for error.
		\param _host An output buffer to keep the host name.
		\param _hostcp The capacity of the output host buffer.
		\param _serv An output buffer to keep the service name/port.
		\param _servcp The capacity of the output service buffer.
		\param _flags Some request flags
	*/
	bool toString(
		char* _host,
		unsigned _hostcp,
		char* _serv,
		unsigned _servcp,
		uint32	_flags = 0
	)const;
	
	bool operator<(const SocketAddress &_raddr)const;
	bool operator==(const SocketAddress &_raddr)const;
	
	void address(const char*_str);
	
	const in_addr& address4()const;
	const in6_addr& address6()const;
	int port()const;
	bool port(int _port);
	void clear();
	size_t hash()const;
	size_t addressHash()const;
	
	void path(const char*_pth);
	const char* path()const;
private:
	friend struct SocketDevice;
	operator sockaddr*();
	sockaddr* sockAddr();
	AddrUnion	d;
	socklen_t	sz;
};
//==================================================================
struct SocketAddressInet{
private:
	union AddrUnion{
		sockaddr		addr;
		sockaddr_in 	inaddr4;
		sockaddr_in6 	inaddr6;
	};
public:
	enum {Capacity = sizeof(AddrUnion)};
	
	SocketAddressInet();
	SocketAddressInet(const SocketAddressStub &);
	SocketAddressInet(const char* _addr, int _port = 0);
	
	SocketAddressInet& operator=(const SocketAddressStub &);
	
	SocketInfo::Family family()const;
	
	bool isInet4()const;
	bool isInet6()const;
	
	bool isLoopback()const;
	bool isInvalid()const;
	
	bool empty()const;
	
	const socklen_t&	size()const;

	const sockaddr* sockAddr()const;
	operator const sockaddr*()const;
	//! Get the name associated to the address
	//! \see SocketAddress::toString
	bool toString(
		char* _host,
		unsigned _hostcp,
		char* _serv,
		unsigned _servcp,
		uint32	_flags = 0
	)const;
	
	bool operator<(const SocketAddressInet &_raddr)const;
	bool operator==(const SocketAddressInet &_raddr)const;
	
	void address(const char*_str);
	
	const in_addr& address4()const;
	const in6_addr& address6()const;
	
	int port()const;
	bool port(int _port);
	void clear();
	size_t hash()const;
	size_t addressHash()const;
	
private:
	friend struct SocketDevice;
	operator sockaddr*();
	sockaddr* sockAddr();
	AddrUnion	d;
	socklen_t	sz;
};
//==================================================================
struct SocketAddressInet4{
private:
	union AddrUnion{
		sockaddr		addr;
		sockaddr_in 	inaddr4;
	};
public:
	enum {Capacity = sizeof(AddrUnion)};
	
	SocketAddressInet4();
	SocketAddressInet4(const SocketAddressStub &);
	SocketAddressInet4(const char* _addr, int _port = 0);
	
	SocketAddressInet4& operator=(const ResolveIterator &);
	SocketAddressInet4& operator=(const SocketAddressStub &);
			
	SocketInfo::Family family()const;
	bool isInet4()const;
		
	bool isLoopback()const;
	bool isInvalid()const;
	
	socklen_t	size()const;

	const sockaddr* sockAddr()const;
	//operator sockaddr*(){return sockAddr();}
	operator const sockaddr*()const;
	//! Get the name associated to the address
	//! \see SocketAddress::toString
	bool toString(
		char* _host,
		unsigned _hostcp,
		char* _serv,
		unsigned _servcp,
		uint32	_flags = 0
	)const;
	
	bool operator<(const SocketAddressInet4 &_raddr)const;
	bool operator==(const SocketAddressInet4 &_raddr)const;
	
	void address(const char*_str);
	
	const in_addr& address()const;
	
	int port()const;
	void port(int _port);
	void clear();
	size_t hash()const;
	size_t addressHash()const;
	
private:
	friend struct SocketDevice;
	operator sockaddr*();
	sockaddr* sockAddr();
	AddrUnion	d;
};
//==================================================================
struct SocketAddressInet6{
private:
	union AddrUnion{
		sockaddr		addr;
		sockaddr_in6 	inaddr6;
	};
public:
	enum {Capacity = sizeof(AddrUnion)};
	
	SocketAddressInet6();
	SocketAddressInet6(const SocketAddressStub &);
	SocketAddressInet6(const char* _addr, int _port = 0);
	
	SocketAddressInet6& operator=(const SocketAddressStub &);
		
	SocketInfo::Family family()const;
	bool isInet6()const;
	
	bool isLoopback()const;
	bool isInvalid()const;
	
	bool empty()const;
	
	socklen_t size()const;

	const sockaddr* sockAddr()const;
	operator const sockaddr*()const;
	
	//! Get the name associated to the address
	//! \see SocketAddress::toString
	bool toString(
		char* _host,
		unsigned _hostcp,
		char* _serv,
		unsigned _servcp,
		uint32	_flags = 0
	)const;
	
	bool operator<(const SocketAddressInet6 &_raddr)const;
	bool operator==(const SocketAddressInet6 &_raddr)const;
	
	void address(const char*_str);
	
	const in6_addr& address()const;
	
	int port()const;
	void port(int _port);
	void clear();
	size_t hash()const;
	size_t addressHash()const;
	
private:
	friend struct SocketDevice;
	operator sockaddr*();
	sockaddr* sockAddr();
	AddrUnion	d;
};
//==================================================================
bool operator<(const in_addr &_inaddr1, const in_addr &_inaddr2);

bool operator==(const in_addr &_inaddr1, const in_addr &_inaddr2);

bool operator<(const in6_addr &_inaddr1, const in6_addr &_inaddr2);

bool operator==(const in6_addr &_inaddr1, const in6_addr &_inaddr2);

size_t hash(const in_addr &_inaddr);

size_t hash(const in6_addr &_inaddr);
//==================================================================
struct SocketAddressLocal{
private:
	union AddrUnion{
		sockaddr		addr;
		sockaddr_un		localaddr;
	};
public:
	enum {Capacity = sizeof(AddrUnion)};
	
	SocketAddressLocal();
	SocketAddressLocal(const char* _path);
	
	SocketAddressLocal& operator=(const SocketAddressStub &);
	
	SocketInfo::Family family()const;
	
	bool empty()const;
	
	const socklen_t&	size()const;

	const sockaddr* sockAddr()const;
	operator const sockaddr*()const;
	
	bool operator<(const SocketAddressLocal &_raddr)const;
	bool operator==(const SocketAddressLocal &_raddr)const;
	
	void clear();
	size_t hash()const;
	size_t addressHash()const;
	
	void path(const char*_pth);
	const char* path()const;
private:
	friend struct SocketDevice;
	operator sockaddr*();
	sockaddr* sockAddr();
	AddrUnion	d;
	socklen_t	sz;
};
//==================================================================
#ifndef NINLINES
#include "system/cassert.hpp"
#include "system/debug.hpp"
#include "system/socketaddress.ipp"
#endif
//==================================================================

#endif
