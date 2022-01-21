#include <pthread.h>
#include <stdio.h>
#include <mutex>

int Global;
std::mutex m;

void *Thread1(void *x) {
  Global = 1;
  m.lock();
  m.unlock();
  return NULL;
}

void *Thread2(void *x) {
  m.lock();
  Global = 2;
  m.unlock();
  return NULL;
}

int main() {
  pthread_t t[2];
  pthread_create(&t[0], NULL, Thread1, NULL);
  pthread_create(&t[1], NULL, Thread2, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
}