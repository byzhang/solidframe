/* Implementation file speed.cpp
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidGround framework.

	SolidGround is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidGround.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <queue>
#include <list>
#include "utility/queue.h"
using namespace std;


struct D{
	D(int i):data(i){
		cout<<"D()"<<endl;
	}
	D():data(0){
		cout<<"D()"<<endl;
	}
	D(const D &_d):data(_d.data){
		cout<<"D(D)"<<endl;
	}
	D& operator=(const D &_d){
		data = _d.data;
		cout<<"operator ="<<endl;
	}
	~D(){
		cout<<"~D()"<<endl;
	}
	int data;
};

//typedef queue<int, list<int> > QueueTp;
//typedef queue<int> QueueTp;
typedef Queue<int>	QueueTp;
int main(){
	ulong sum = 0;
	cout<<"sizeof queue<int> = "<<sizeof(QueueTp)<<endl;
	QueueTp q;
	for(int j = 0; j < 20; ++j){
		for(int i = 0; i < 100000; ++i){
			q.push(i);
			sum += q.front();
			q.pop();
		}
		
		while(q.size()){
			sum += q.front();
			q.pop();
		}//*/
	}
	cout<<"sum = "<<sum<<endl;
}
