#include "system/socketaddress.hpp"
#include "system/socketdevice.hpp"
#include <iostream>

using namespace std;
using namespace solid;

int main(){
#ifdef ON_WINDOWS
	WSADATA	wsaData;
    int		err;
	WORD	wVersionRequested;
/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }
#endif
	ResolveData rd =  synchronous_resolve("0.0.0.0", "0", 0, SocketInfo::Inet4, SocketInfo::Datagram, 0);
	ResolveIterator it(rd.begin());
	
	SocketDevice sd;
	sd.create(it);
	sd.bind(it);
	
	if(sd.ok()){
		string				hoststr;
		string				servstr;
		SocketAddress		addr;
		
		sd.localAddress(addr);
		synchronous_resolve(
			hoststr,
			servstr,
			addr,
			ReverseResolveInfo::Numeric
		);
		cout<<hoststr<<':'<<servstr<<endl;
	}
	cout<<sd.ok()<<endl;
	return 0;
}
