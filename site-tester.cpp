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
}
