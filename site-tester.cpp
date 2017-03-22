#include <iostream>
#include <string>
#include <sys/wait.h>
#include <errno.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <cstring>
#include <queue>
using namespace std;

// memory structure and callback function from https://curl.haxx.se/libcurl/c/getinmemory.html
struct MemoryStruct{
	char *memory;
	size_t size;
	
};
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void* userp){
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}


int main(int argc, char *argv[]) {
  string line;
  char* config_file;
  int PERIOD_FETCH = 180;
  int NUM_FETCH = 1;
  int NUM_PARSE = 1;
  string SEARCH_FILE = "Search.txt";
  string SITE_FILE = "Sites.txt";

  // Get Configuration File
  if(argv[1] != NULL) {
    config_file = argv[1];
  }
  else {
    cout << "Error with the config file" << endl;
    // I think we have to exit here or something - figure out later
  }

  // Read from the config file
  ifstream file;
  file.open(config_file);
  while(getline(file, line)) {
    //cout << "10" << endl;
    string buf;
    stringstream iss(line);
    vector<string> words;
    while(getline(iss, buf, '=')) {
      words.push_back(buf);
    }
    if(words[0] == "PERIOD_FETCH") {
      stringstream ss(words[1]);
      int tmp;
      ss >> tmp;
      PERIOD_FETCH = tmp;
    }
    else if(words[0] == "NUM_FETCH") {
      stringstream ss(words[1]);
      int tmp;
      ss >> tmp;
      NUM_FETCH = tmp;
    }
    else if(words[0] == "NUM_PARSE") {
      stringstream ss(words[1]);
      int tmp;
      ss >> tmp;
      NUM_PARSE = tmp;
    }
    else if(words[0] == "SEARCH_FILE") {
      SEARCH_FILE = words[1];
    }
    else if(words[0] == "SITE_FILE") {
      SITE_FILE = words[1];
    }
    else {
      cout << "Warning: " << words[0] << " is not a parameter" << endl;
    }
    /* 
    for(vector<string>::iterator i = words.begin(); i != words.end(); ++i) {
      cout << *i << endl;
    } 
    */
  }
  //cout << PERIOD_FETCH << endl;
  file.close();
  
  //easy_curl
  
  queue<string> webpages;
  
  file.open(SITE_FILE.c_str());
  while(getline(file, line)) {
	
    CURL *curl_handle;
	CURLcode res;
	struct MemoryStruct chunk;
	chunk.memory = (char*) malloc(1);
	chunk.size = 0;
	curl_global_init(CURL_GLOBAL_ALL);
	
	curl_handle = curl_easy_init();
	
	curl_easy_setopt(curl_handle, CURLOPT_URL, line.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl_handle);
	if(res != CURLE_OK){
		cout << "error" << endl;
	}
	else{
		string a = chunk.memory;
        webpages.push(a);
	}
	curl_easy_cleanup(curl_handle);
	free(chunk.memory);
	curl_global_cleanup();
  }
}
