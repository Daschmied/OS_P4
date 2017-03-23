#ifndef HQUEUE_H
#define HQUEUE_H

#include <pthread.h>
#include <list>
#include <string>
using namespace std;

template<typename T> class hqueue {

  list<T> inqueue;

  public:
    pthread_mutex_t m_mutex;
    pthread_cond_t notEmpty;

    hqueue() {
      pthread_mutex_init(&m_mutex, NULL);
      pthread_cond_init(&notEmpty, NULL);
    }

    ~hqueue() {
      pthread_mutex_destroy(&m_mutex);
      pthread_cond_destroy(&notEmpty);
    }

    void add(T object) {
      inqueue.push_back(object);
    }

    T pop_off() {
      T object = inqueue.front();
      inqueue.pop_front();
      return object;
    }

    int size() {
      int size = inqueue.size();
      return size;
    }
};

#endif
