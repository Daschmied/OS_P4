#include <iostream>
#include <string>
#include <sys/wait.h>
#include <errno.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

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
    int write = 0
    string buf;
    stringstream iss(line);
    vector<string> words;
    while(getline(iss, buf, '=')) {
      words.push_back(buf);
    }
    for(vector<string>::iterator i = words.begin(); i != words.end(); ++i) {
      cout << *i << endl;
      if(*i == "PERIOD_FETCH") {
        //Set the value for the next one
      }
    } 
  }
  
  file.close();
}
