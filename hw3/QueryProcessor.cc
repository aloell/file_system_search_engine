/*
 * Copyright 2011 Steven Gribble
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

#include <iostream>
#include <algorithm>

#include <unordered_set>
#include <list>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

struct MyHash{
	std::size_t operator()(const QueryProcessor::QueryResult& q) const{
        	return std::hash<string>{}(q.document_name);
	}
};

struct MyEqual{
	bool operator()(const QueryProcessor::QueryResult& q1, const QueryProcessor::QueryResult& q2) const{
        	return q1.document_name==q2.document_name;
	}
};

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;

  // MISSING:
  //assume doc name has file path prefix
  list<QueryProcessor::QueryResult> result;
  auto itr=query.begin();
  for(uint32_t i=0;i<arraylen_;i++){
	DocIDTableReader* docid_reader=(itr_array_[i])->LookupWord(*itr);
        if(docid_reader==nullptr) continue;
        list<docid_element_header> docid_header_list=docid_reader->GetDocIDList();
	for(auto& deh:docid_header_list){
		string doc_name;
		(dtr_array_[i])->LookupDocID(deh.docid, &doc_name);
                QueryProcessor::QueryResult cur_rst={doc_name,deh.num_positions};
		result.push_back(cur_rst);
	}
  }
  if(result.size()==0)
	return finalresult;
  itr++;
  for(;itr!=query.end();itr++){
        std::unordered_set<QueryProcessor::QueryResult,MyHash,MyEqual> dict;
	for(uint32_t i=0;i<arraylen_;i++){
		DocIDTableReader* docid_reader=(itr_array_[i])->LookupWord(*itr);
                if(docid_reader==nullptr) continue;
                list<docid_element_header> docid_header_list=docid_reader->GetDocIDList();
		for(auto& deh:docid_header_list){
			string doc_name;
			(dtr_array_[i])->LookupDocID(deh.docid, &doc_name);
                	QueryProcessor::QueryResult cur_rst={doc_name,deh.num_positions};
			dict.insert(cur_rst);
		}
        }
	for(auto qitr=result.begin();qitr!=result.end();){
        	auto search=dict.find(*qitr);
		if(search==dict.end()){
			list<QueryProcessor::QueryResult>::iterator origin=qitr;
			qitr++;			
			result.erase(origin);	
		}else{
			qitr->rank+=search->rank;
			qitr++;
		}
	}

  }
  for(auto qitr=result.begin();qitr!=result.end();qitr++){
	finalresult.push_back(*qitr);  
  }

  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
