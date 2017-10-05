/*
 * Copyright 2012 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include <sstream>
#include <cerrno>

#include <pthread.h>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;

namespace hw4 {

bool HttpConnection::GetNextRequest(HttpRequest *request) {
  // Use "WrappedRead" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use ParseRequest()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes GetNextRequest()!

  // MISSING:
  //assume that ParseRequest will remove the valid requst stored in the head part of buf_.
  char *buf=new char[1000];
  std::string::size_type pos=buffer_.find("\r\n\r\n");
  while(pos==std::string::npos){
        if(buffer_.size()>5000){
		std::cerr<<"buf size in reading socket too large"<<std::endl;
		return false;	
	}
 	int res=WrappedRead(fd_, (unsigned char*)buf, 1000);
  	if(res==-1){
		std::cerr<<"socket read error:"<<std::strerror(errno)<<std::endl;
		return false;
  	}
  	buffer_.append(buf,res);
  	pos=buffer_.find("\r\n\r\n");
  }
  *request=ParseRequest(pos+4);
  std::cout<<"request uri:"<<request->URI<<" pthread id:"<<pthread_self()<<std::endl;
  delete [] buf;
  return true;
}

bool HttpConnection::WriteResponse(const HttpResponse &response) {
  std::string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         (unsigned char *) str.c_str(),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(size_t end) {
  HttpRequest req;
  //req.URI = "/";  // by default, get "/".

  // Get the header.
  std::string str = buffer_.substr(0, end);
  if(str.size()<4){
	std::cout<<buffer_<<std::endl;
  	std::cerr<<"input of parsereqeust has errors."<<std::endl;
	exit(-1);
  }
  // Split the header into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers (i.e., req.headers[headername] = headervalue).
  // You should look at HttpResponse.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.

  // MISSING:
  string origin=str.substr(0,str.size()-4);
  typedef std::vector< string > split_vector_type; 
  split_vector_type SplitVec1; 
  split_vector_type SplitVec2;
  boost::split( SplitVec1, origin, boost::is_any_of("\r\n"), boost::token_compress_on );
  map<string,string> request_headers;
  for(uint32_t i=0;i<SplitVec1.size();i++){
	string s=SplitVec1[i];
        if(i==0){
		boost::split( SplitVec2, s, boost::is_any_of(" "), boost::token_compress_on );
        	//boost::to_lower(SplitVec2[1]);
		req.URI=SplitVec2[1];
		//std::cout<<"URI:"<<SplitVec2[1]<<std::endl;
		continue;        
	}
	boost::split( SplitVec2, s, boost::is_any_of(":"), boost::token_compress_on );
	string key=SplitVec2[0];
	string value=SplitVec2[1];
	boost::trim(key);
        boost::to_lower(key);
	boost::trim(value);
        boost::to_lower(value);
  	//std::cout<<"key:"<<key<<"  value:"<<value<<std::endl;
	request_headers[key]=value;
  }
  req.headers=request_headers;
  string nextBuf=buffer_.substr(str.size());
  buffer_=nextBuf;
  return req;
}

}  // namespace hw4
