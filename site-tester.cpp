#include "hqueue.h"
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
#include <pthread.h>
#include <queue>
#include <signal.h>

using namespace std;


hqueue<string> sites;
queue<string> webpages;
volatile sig_atomic_t flag = false;

void * producer_function(void * arg);
void change_flag(int sig);

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
  signal(SIGALRM, change_flag);

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
      //cout << words[0] << words[1] << endl;
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

  // Populate sites
  file.open(SITE_FILE.c_str());
  while(getline(file, line)) {
    sites.add(line);
  } 
  file.close();

  vector<string> searchterms;
  file.open(SEARCH_FILE.c_str());
  while(getline(file, line)) {
    searchterms.push_back(line);
  }
  file.close();

  /* 
     for(vector<string>::iterator i = searchterms.begin(); i != searchterms.end(); ++i) {
     cout << *i << endl;
     } 
     */


  pthread_t *ProducerThreads = (pthread_t*)malloc(NUM_FETCH * sizeof(pthread_t));
  pthread_t *ConsumerThreads = (pthread_t*)malloc(NUM_PARSE * sizeof(pthread_t));

  for(int i = 0; i < NUM_FETCH; i++) { 
    int errnum = pthread_create(&ProducerThreads[i], NULL, producer_function, NULL); 
    if(errnum != 0) {
      printf("Error creating thread: %s\n", strerror(errnum));
      //maybe some other behavior?
      exit(1);
    } 
  } 

  /* 
     while(!webpages.empty()){
     string w = webpages.front();
     cout << w << endl;
     webpages.pop();
     }

     while(!sites.empty()){
     string w = sites.front();
     cout << w << endl;
     sites.pop();
     }
     */

  while(!webpages.empty()) {
    string site = sites.pop_off();
    string test = webpages.front();
    webpages.pop();
    cout << site << endl;
    for(vector<string>::iterator i = searchterms.begin(); i != searchterms.end(); ++i)
    {
      int count = 0;
      size_t nPos = test.find(*i, 0);
      while(nPos != string::npos) {
        count++;
        nPos = test.find(*i, nPos+1);
      }
      cout << *i << ": " << count << endl;
    }
  }
}

void change_flag(int sig) {
  flag = true;
}

void * producer_function(void *arg) {
  //easy_curl

  while(1) {
    pthread_mutex_lock(&sites.m_mutex);
    while(sites.size() == 0) {
      pthread_cond_wait(&sites.notEmpty, &sites.m_mutex);
      //if(!keepRunning) {
      //pthread_exit(0);
      //}
      //basically check for crtl-C or SigHUP or w/e    
    }
    //cout << SITE_FILE << endl;
    // Instead of opening site file, pop off of sites 

    string running = sites.pop_off();

    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = (char*) malloc(1);
    chunk.size = 0;
    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, running.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
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
  pthread_exit(0);

}

