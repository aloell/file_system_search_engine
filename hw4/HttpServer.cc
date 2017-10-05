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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include <pthread.h>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"
#include "./libhw3/QueryProcessor.h"

using std::cerr;
using std::cout;
using std::endl;

namespace hw4 {

// This is the function that threads are dispatched into
// in order to process new client connections.
void HttpServer_ThrFn(ThreadPool::Task *t);

// Given a request, produce a response.
HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices);

// Process a file request.
HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir);

// Process a query request.
HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices);

bool HttpServer::Run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!ss_.BindAndListen(AF_UNSPEC, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask *hst = new HttpServerTask(HttpServer_ThrFn);
    hst->basedir = staticfile_dirpath_;
    hst->indices = &indices_;
    if (!ss_.Accept(&hst->client_fd,
                    &hst->caddr,
                    &hst->cport,
                    &hst->cdns,
                    &hst->saddr,
                    &hst->sdns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.Dispatch(hst);
  }
  return true;
}

void HttpServer_ThrFn(ThreadPool::Task *t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  std::unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask *>(t));
  cout << "  client " << hst->cdns << ":" << hst->cport << " "
       << "(IP address " << hst->caddr << ")" << " connected." << endl;

  bool done = false;
  
  HttpConnection hc(hst->client_fd);
  while (!done) {
    // Use the HttpConnection class to read in the next request from
    // this client, process it by invoking ProcessRequest(), and then
    // use the HttpConnection class to write the response.  If the
    // client sent a "Connection: close\r\n" header, then shut down
    // the connection.

    // MISSING:
    
    HttpRequest request;
    if(hc.GetNextRequest(&request)){
        std::cout<<"thread id:"<<pthread_self()<<" request uri:"<<request.URI<<std::endl;
	HttpResponse response=ProcessRequest(request,hst->basedir,hst->indices);
    	bool succ=hc.WriteResponse(response);
	//cout<<"write successfully: "<<succ<<endl;
        string connection_close("close");
        auto itr=request.headers.find("connection");
        if(itr!=request.headers.end()){
		if(itr->second==connection_close) return;	
	}else{
	   return;
	}
    }else{
	return;
    }
  }
}

HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices) {
  // Is the user asking for a static file?
  if (req.URI.substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.URI, basedir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.URI, indices);
}


HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for.
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  std::string fname = "";
  
  // MISSING:
  std::map<string,string> content_types;
  content_types["html"]="text/html";
  content_types["htm"]="text/html";
  content_types["jpeg"]="image/jpeg";
  content_types["jpg"]="image/jpeg";
  content_types["png"]="image/png";
  string static_prefix="/static/";
  //remove the static prefix in the path
  fname=uri.substr(static_prefix.size());
  FileReader file_reader(basedir, fname);
  string* body_ptr=new string;
  if(file_reader.ReadFile(body_ptr)){ 	
	ret.protocol = "HTTP/1.1";
  	ret.response_code = 200;
  	ret.message = "OK";
	ret.body=*body_ptr;
        string file_type=fname.substr(fname.find(".")+1);
        ret.headers["Content-type"]=content_types[file_type];
	//std::cout<<ret<<std::endl;
  }else{
  	// If you couldn't find the file, return an HTTP 404 error.
  	ret.protocol = "HTTP/1.1";
  	ret.response_code = 404;
  	ret.message = "Not Found";
  	ret.body = "<html><body>Couldn't find file \"";
  	ret.body +=  EscapeHTML(fname);
  	ret.body += "\"</body></html>";
  }
  delete body_ptr;
  return ret;
}

HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/http333d server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 333gle logo and the
  //    search box/button
  //
  //  - if the user had previously typed in a search query, you also
  //    need to display the search results.
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - you'll want to create and use a hw3::QueryProcessor to process
  //    the query against the search indices
  //
  //  - in your generated search results, see if you can figure out
  //    how to hyperlink results to the file contents, like we did
  //    in our solution_binaries/http333d.

  // MISSING:
  
  URLParser url_parser;
  url_parser.Parse(uri);
  map<string, string> args=url_parser.get_args();
  ret.protocol = "HTTP/1.1";
  ret.response_code = 200;
  ret.message = "OK";
  ret.headers["Content-type"]="text/html";
  string body1("<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=windows-1252\"><title>333gle</title></head><body><center style=\"font-size:500%;\"><span style=\"position:relative;bottom:-0.33em;color:orange;\">3</span><span style=\"color:red;\">3</span><span style=\"color:gold;\">3</span><span style=\"color:blue;\">g</span><span style=\"color:green;\">l</span><span style=\"color:red;\">e</span></center><p></p><div style=\"height:20px;\"></div><center><form action=\"/query\" method=\"get\"><input size=\"30\" name=\"terms\" type=\"text\"><input value=\"Search\" type=\"submit\"></form></center>");
  if(args.size()==1&&args.begin()->first=="terms"){
  	string query_words=args.begin()->second;
  	std::stringstream ss(query_words);
  	std::vector<string> query;
  	string token;
        //std::cout<<"query words:"<<query_words<<std::endl;
  	for(;std::getline(ss,token,' ');){
		//std::cout<<"query token:"<<token<<std::endl;
  		query.push_back(token);
  	}
  	hw3::QueryProcessor qp(*indices);
  	vector<hw3::QueryProcessor::QueryResult> query_results=qp.ProcessQuery(query);
  	
  	string num_results=std::to_string(query_results.size());
  	string body2="<br>"+num_results+" results found for <b>"+EscapeHTML(query_words)+"</b><br><ul>";
  	string body3="";
  	for(hw3::QueryProcessor::QueryResult& qr:query_results){
  		token="<li><a href=\"/static/"+EscapeHTML(qr.document_name)+"\">"+EscapeHTML(qr.document_name)+"</a></li><br>";
        	body3.append(token);
  	}
  	string body4="</ul></body></html>";
  	string body=body1+body2+body3+body4;
  	ret.body=body;
  }else{
	string body4="</body></html>";
	string body=body1+body4;
	ret.body=body;
  }
  return ret;
}

}  // namespace hw4
