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

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"


static void Usage(void);

int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  char* rootdir=argv[1];
 
  int res;
  DocTable dt;
  MemIndex idx;
  LinkedList llres;
  LLIter lit;
  unsigned int i;

  // Crawl part of the file tree.
  res = CrawlFileTree(rootdir,&dt, &idx);
  Verify333(res!=0);


  char* token;
  const char* delimiter=" \n";
  while(true){
        char* tokens[100];
        int numTokens=0;
        char* query=(char*)malloc(100);
	fgets(query,100,stdin);
	//printf("your entered:%s,%d\n",query,strlen(query));
        char* saveptr;
        for(;;query=NULL){
	    	token=strtok_r(query,delimiter,&saveptr);
            	if(token==NULL) break;
		tokens[numTokens++]=token;
	    	printf("parsed token:%s, token length:%d\n",token,strlen(token));
  	}
  	llres = MIProcessQuery(idx, tokens, numTokens);
	if(llres==NULL||NumElementsInLinkedList(llres)==0){
		printf("no results found!\n");
		return EXIT_SUCCESS;
	}
  	lit = LLMakeIterator(llres, 0);
        printf("results for query\n");
  	for (i = 0; i < NumElementsInLinkedList(llres); i++) {
    		SearchResult *res;
    		LLIteratorGetPayload(lit, (LLPayload_t*)(&res));
      		char *docname = DTLookupDocID(dt, res->docid);
		printf("\t\t\tnumber:%d, docname:%s, rank:%d\n",i,docname,res->rank);
    		LLIteratorNext(lit);
  	}
        printf("\n");
  	LLIteratorFree(lit);
  	FreeLinkedList(llres, free);
	free(query);
  }       

  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(-1);	
}

