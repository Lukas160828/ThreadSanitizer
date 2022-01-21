#include <pthread.h>
#include <stdio.h>
#include <mutex>

int Global;
int Condition;

void *Thread1(void *x) {
  Condition = 1;  
  Global = 1;
  return NULL;
}

void *Thread2(void *x) {
  if(Condition == 1){
      Global = 2;
  }
  return NULL;
}

int main() {
  Condition = 0;  
  pthread_t t[2];
  pthread_create(&t[0], NULL, Thread1, NULL);
  pthread_create(&t[1], NULL, Thread2, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
}