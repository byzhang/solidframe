#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/concept_check.hpp>
#include <queue>
#include <sstream>
#include "system/timespec.hpp"


using namespace boost::asio;
using boost::asio::ip::tcp;
using namespace std;
using namespace solid;

typedef boost::asio::ip::tcp::endpoint  TcpEndPointT;

struct Manager{
	struct DataStub{
		std::string		data;
	};
	
	typedef std::vector<DataStub>	DataVectorT;
	
	Manager(
		uint32_t _repeatcnt,
		uint32_t _datafromlen,
		uint32_t _datatolen,
		uint32_t _datacnt
	);
	
	const DataVectorT& dataVector()const{
		return datavec;
	}
	uint32_t repeatCount()const{return repeat_count;}
	
	void endPoint(const TcpEndPointT &_rendpoint){
		endpoint = _rendpoint;
	}
	const TcpEndPointT& endPoint()const{
		return endpoint;
	}
	
	void report(uint32_t _mintime, uint32_t _maxtime, const uint64_t &_readcnt){
		if(mintime > _mintime) mintime = _mintime;
		if(maxtime < _maxtime) maxtime = _maxtime;
		readcnt += _readcnt;
	}
	void print(){
		std::cerr<<"mintime = "<<mintime<<" maxtime = "<<maxtime<<" readcnt = "<<readcnt<<std::endl;
	}
private:
	DataVectorT		datavec;
	uint32_t		repeat_count;
	TcpEndPointT	endpoint;
	
	uint32_t		mintime;
	uint32_t		maxtime;
	uint64_t		readcnt;
};


class session
{
public:
	session(boost::asio::io_service& io_service, Manager &_rm, uint32_t _idx)
		: rm(_rm), socket_(io_service), mintime(0xffffffff), maxtime(0), crtread(0), readcnt(0)
	{
		writeidx = _idx % rm.dataVector().size();
		readidx = writeidx;
	}
	~session(){
		rm.report(mintime, maxtime, readcnt);
	}
	tcp::socket& socket()
	{
		return socket_;
	}

	void start(const TcpEndPointT &_rendpoint)
	{
		socket().async_connect(
			_rendpoint,
			boost::bind(
				&session::handle_connect,
				this,
				boost::asio::placeholders::error
			)
		);
	}

private:
	void handle_connect(
		const boost::system::error_code &_rerr
	){
		if(_rerr){
			delete this;
			return;
		}
		
		socket().async_read_some(
			boost::asio::buffer(data_, max_length),
			boost::bind(&session::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);
		const std::string &data = rm.dataVector()[writeidx % rm.dataVector().size()].data;
		//cout<<"write "<<(writeidx % rm.dataVector().size())<<" size = "<<data.size()<<endl;
		timeq.push(TimeSpec::createRealTime());
		boost::asio::async_write(
			socket_,
			boost::asio::buffer(data.data(), data.size()),
			boost::bind(&session::handle_write, this,
			boost::asio::placeholders::error)
		);
	}
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred)
	{
		if (!error)
		{	
			if(consume_data(data_, bytes_transferred)){
				delete this;
				return;
			}
			socket().async_read_some(
				boost::asio::buffer(data_, max_length),
				boost::bind(&session::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
			);
		}
		else
		{
			delete this;
		}
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			++writeidx;
			if(writeidx == rm.repeatCount()){
				return;
			}
			const std::string &data = rm.dataVector()[writeidx%rm.dataVector().size()].data;
			//cout<<"write2 "<<(writeidx % rm.dataVector().size())<<" size = "<<data.size()<<endl;
			timeq.push(TimeSpec::createRealTime());
			boost::asio::async_write(
				socket_,
				boost::asio::buffer(data.data(), data.size()),
				boost::bind(&session::handle_write, this,
				boost::asio::placeholders::error)
			);
		}
		else
		{
			//delete this;
		}
	}
	bool consume_data(const char *_p, size_t _l);
	
	typedef std::queue<TimeSpec>	TimeSpecQueueT;
	enum { max_length = 1024 };
	Manager			&rm;
	tcp::socket		socket_;
	char			data_[max_length];
	uint32_t		writeidx;
	uint32_t		readidx;
	TimeSpecQueueT	timeq;
	uint32_t		mintime;
	uint32_t		maxtime;
	uint32_t		crtread;
	uint64_t		readcnt;
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 5)
		{
			std::cerr << "Usage: boost_echoclient <address> <port> <connection_count> <repeat_count>\n";
			return 1;
		}

		boost::asio::io_service io_service;

		using namespace std; // For atoi.
		
		Manager m(atoi(argv[4]), 4096, 24 * 1024, 10);
		
		m.endPoint(ip::tcp::endpoint(ip::address::from_string(argv[1]), atoi(argv[2])));
		
		int concnt = atoi(argv[3]);
		int idx = 0;
		while(concnt--){
			session *ps = new session(io_service, m, idx);
			ps->start(m.endPoint());
			++idx;
		}
		io_service.run();
		m.print();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
Manager::Manager(
	uint32_t _repeatcnt,
	uint32_t _datafromlen,
	uint32_t _datatolen,
	uint32_t _datacnt
):repeat_count(_repeatcnt){
	mintime = 0xffffffff;
	maxtime = 0;
	readcnt = 0;
	string line;
	for(char c = '0'; c <= '9'; ++c) line += c;
	for(char c = 'a'; c <= 'z'; ++c) line += c;
	for(char c = 'A'; c <= 'Z'; ++c) line += c;
	size_t	idx = 0;
	for(uint32_t i = 1; i <= _datacnt; ++i){
		uint32_t datalen = (_datafromlen * _datacnt + _datatolen * i - _datafromlen * i - _datatolen)/(_datacnt - 1);
		//cout<<"Data len for "<<i<<" = "<<datalen<<endl;
		datavec.push_back(DataStub());
		for(uint32_t j = 0;  datavec.back().data.size() < datalen; ++j){
			if(!(j % line.size())){
				ostringstream oss;
				oss<<' '<<idx<<' ';
				++idx;
				datavec.back().data += oss.str();
			}else{
				datavec.back().data.push_back(line[j%line.size()]);
			}
		}
	}
}

bool session::consume_data(const char *_p, size_t _l){
	//cout<<"consume [";
	//cout.write(_p, _l)<<']'<<endl;
	readcnt += _l;
	
	while(_l){
		const uint32_t	crtdatalen = rm.dataVector()[readidx % rm.dataVector().size()].data.size();
		uint32_t 		toread = crtdatalen - crtread;
		if(toread > _l) toread = _l;
		
		crtread += toread;
		_l -= toread;
		
		//cout<<"consume "<<(readidx % rm.dataVector().size())<<" size = "<<crtdatalen<<" crtread = "<<crtread<<endl;
		
		if(crtread == crtdatalen){
			crtread = 0;
			TimeSpec ts(TimeSpec::createRealTime());
			ts -= timeq.front();
			timeq.pop();
			const uint32_t msec = ts.seconds() * 1000 + ts.nanoSeconds()/1000000;
			if(mintime > msec) mintime = msec;
			if(maxtime < msec) maxtime = msec;
			++readidx;
			if(readidx == rm.repeatCount()){
				return true;
			}
		}
	}
	return false;
}