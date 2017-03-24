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
#include <ctime>
#include <unistd.h>
using namespace std;
vector<string> searchterms;
hqueue<string> sites;
hqueue<string> webpages;
queue<string> output_queue;
int periods = 0;
volatile sig_atomic_t flag = false;

void * producer_function(void * arg);
void change_flag(int sig);
void int_handler(int sig);
void * consumer_function(void *);
string output_to_file(string, string, int);

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
  signal(SIGINT, int_handler);
  signal(SIGHUP, int_handler);
  
  //create a set of signals that will be blocked
  sigset_t signal_set;
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set,SIGHUP);
  pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
  // Get Configuration File
  if(argv[1] != NULL) {
    config_file = argv[1];
  }
  else {
    cout << "Incorrect Usage" << endl;
    cout << "Usage: site-tester config-file" << endl;
    exit(0);
  }

  // Read from the config file
  ifstream file;
  
  file.open(config_file);
  if(!file.is_open()){
    cout << "Error: Unable to open " << config_file << endl;
    exit(0);
  }
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
      if(NUM_FETCH >8 || NUM_FETCH < 1){
        cout << "Invalid number of fetching threads. Valid values: 1-8" << endl;
        exit(0);
      }
    }
    else if(words[0] == "NUM_PARSE") {
      stringstream ss(words[1]);
      int tmp;
      ss >> tmp;
      NUM_PARSE = tmp;
      if(NUM_PARSE > 8 || NUM_PARSE<1){
        cout << "Invalid number of parsing threads. Valid values: 1-8" << endl;
        exit(0);
      }

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
  }
  file.close();

  //save sites into a vector to populate later
  vector<string> sites_vector;
  file.open(SITE_FILE.c_str());
  if(!file.is_open()){
    cout << "Error: Unable to open " << SITE_FILE << endl;
    exit(0);
  }
  while(getline(file, line)) {
    sites_vector.push_back(line);
  } 
  file.close();
  //save search terms to use later
  file.open(SEARCH_FILE.c_str());
  if(!file.is_open()){
    cout << "Error: Unable to open " << SEARCH_FILE << endl;
    exit(0);
  }
  while(getline(file, line)) {
    searchterms.push_back(line);
  }
  file.close();

 

  pthread_t *ProducerThreads = (pthread_t*)malloc(NUM_FETCH * sizeof(pthread_t));
  pthread_t *ConsumerThreads = (pthread_t*)malloc(NUM_PARSE * sizeof(pthread_t));
  
  //create producers
  for(int i = 0; i < NUM_FETCH; i++) { 
    int errnum = pthread_create(&ProducerThreads[i], NULL, producer_function, NULL); 
    if(errnum != 0) {
      printf("Error creating thread: %s\n", strerror(errnum));
      //maybe some other behavior?
      exit(1);
    } 
  }
  
  //create consumers
  for(int i =0; i < NUM_PARSE; i++){
    int errnum;
    errnum = pthread_create(&ConsumerThreads[i], NULL, consumer_function, NULL);
    if(errnum!=0){
      printf("Error creating thread: %s\n", strerror(errnum));
      exit(1);
    }
  }
  alarm(0);
  alarm(1);
  while(1){
    //unblock signals so main thread can receive
    sigset_t emptyset;
    sigemptyset(&emptyset);
    sigsuspend(&emptyset);
    if(flag == true){
      //when flag is true, repopulate the sites queue, then wake up producers
      for(int i=0; i<(int)sites_vector.size(); i++){
        sites.add(sites_vector[i]);
      }
      pthread_cond_signal(&sites.notEmpty);
      flag = false;
    }
    //every fetch seconds, make an alarm
    alarm(PERIOD_FETCH);
  }
}

void change_flag(int sig) {
  ofstream file;
  stringstream p;
  p << periods;
  
  string period_str = p.str();
  string output_file = period_str+".csv";

  if(periods > 0){
    file.open(output_file.c_str());
    
  }
  while(!output_queue.empty()){
    file << output_queue.front() << endl;
    output_queue.pop();
  }
  flag = true;
  periods ++;
}
void int_handler(int sig){
  //if main gets sigint or sighup, exit program
  cout << "Terminating" << endl;
  exit(0);
  /*
  for(int i = 0; i< NUM_FETCH; i++){
    pthread_join(ProdcuerThreads[i], NULL);
  }
  for(int i=0; i<NUM_PARSE; i++){
    pthread_join(ConsumerThreads[i], NULL);
  }*/
}


void * producer_function(void *arg) {
  //easy_curl

  while(1) {
    pthread_mutex_lock(&sites.m_mutex);
    while(sites.size() == 0) {
      pthread_cond_wait(&sites.notEmpty, &sites.m_mutex);
    }
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
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
    res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK){
      cout << "error" << endl;
    }
    else{
      string a = chunk.memory;
      //add both site url and site contents to webpages to access both in consumer
      webpages.add(running);
      webpages.add(a);
      pthread_cond_signal(&webpages.notEmpty);
    }
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    curl_global_cleanup();
    pthread_mutex_unlock(&sites.m_mutex);
  }
  pthread_exit(0);

}
void * consumer_function(void* args){
  while(1){
    pthread_mutex_lock(&webpages.m_mutex);
    while(webpages.size() == 0) {
      pthread_cond_wait(&webpages.notEmpty, &webpages.m_mutex);
    }
    //pop off twice to get both url and content  
    string site = webpages.pop_off();
    string test = webpages.pop_off();
    for(vector<string>::iterator i = searchterms.begin(); i != searchterms.end(); ++i){
      int count = 0;
      size_t nPos = test.find(*i, 0);
      while(nPos != string::npos) {
        count++;
        nPos = test.find(*i, nPos+1);
      }
      string output = output_to_file(*i, site, count);
      output_queue.push(output);
    }   
    pthread_mutex_unlock(&webpages.m_mutex); 
  }
  pthread_exit(0);
}

string output_to_file(string phrase, string site, int count){
  time_t timev;
  time(&timev);
  char *cdate = asctime(std::localtime(&timev));
  string date = cdate;
  stringstream a;
  a << count;
  string countstr =a.str() ;
  string output =date.erase(date.length()-1) + "," + phrase + "," + site + "," + countstr;
  
  return output;
}
