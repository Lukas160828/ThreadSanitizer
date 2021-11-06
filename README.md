# ThreadSanitizer zur Deadlockanalyse

## Inhaltsverzeichnis
1. [Aktuell existierender Wissensstand](#aktuell-existierender-wissensstand)
    * [Dokumentation](#dokumentation)
    * [Beispiele](#beispiele)
    * [Anwendungen von ThreadSanitizer](#anwendung-von-threadsanitizer)
2. [Evaluation der Verfügbaren Informationen](#evaluation-der-verfügbaren-informationen)
***
# Aktuell existierender Wissensstand
## Dokumentation
Quelle 1: [ThreadSanitizerCppManual Github](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

Durch eine normale Suche per Google findet man als erstes Ergebnis dieses Github Repository. In dieser Quelle kann man sehr schnell eine Übersicht über die Hauptgesichtspunkte von ThreadSanitizer finden. 


Hierzu zählen:
1.  Eine Erklärung wie Data Races Entstehen und was sie Bewirken können. Das Gewählte Beispiel für ein Data Race finde ich jedoch etwas Unübersichtlich.
```cpp
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <map>

typedef std::map<std::string, std::string> map_t;

void *threadfunc(void *p) {
  map_t& m = *(map_t*)p;
  m["foo"] = "bar";
  return 0;
}

int main() {
  map_t m;
  pthread_t t;
  pthread_create(&t, 0, threadfunc, &m);
  printf("foo=%s\n", m["foo"].c_str());
  pthread_join(t, 0);
}
```
Dieses Beispiel finde ich nicht so gelungen da es nicht erklärt wird wodurch hier das Data Race entsteht. Hierbei wären ein paar Kommentare für die Übersicht hilfreich. Auch könnte man den Variablen Aussagekräftigere Namen geben als nur Buchstaben.

2. Eine Übersicht wo und wie ThreadSanitizer zur Verfügung gestellt wird. Es existiert eine Liste über die unterstützten Platformen. Die Liste wurde jedoch zuletzt im Dezember 2018 aktualisiert. Es existiert ein Link der auf eine aktuelle Liste zeigen soll, er führt jedoch lediglich auf ein Github Repository in dem das [Header File von ThreadSanitizer](https://github.com/llvm-mirror/compiler-rt/blob/master/lib/tsan/rtl/tsan_platform.h) liegt. Hierbei werden checks über die Platform im Code ausgeführt, welche man sich anschauen kann. Heißt hier ist die vollständige Liste theoretisch vorhanden, es ist jedoch nicht direkt ersichtlich da es nicht gesammelt verfügbar ist.

3. Eine Übersicht wie man ThreadSanitizer verwenden kann. Das Beispiel hierfür ist gut gewählt, da es sehr intuitiv verständlich ist. 

```cpp
$ cat simple_race.cc
#include <pthread.h>
#include <stdio.h>

int Global;

void *Thread1(void *x) {
  Global++;
  return NULL;
}

void *Thread2(void *x) {
  Global--;
  return NULL;
}

int main() {
  pthread_t t[2];
  pthread_create(&t[0], NULL, Thread1, NULL);
  pthread_create(&t[1], NULL, Thread2, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
}
```
Auch wird die Konsole gezeigt, welche Eingaben getätigt wurden und welches Ergebnis erzielt wurde. Dies führt dazu dass man es gut nachstellen und einen Beispiel Anwendungsfall hat. 

```
$ clang++ simple_race.cc -fsanitize=thread -fPIE -pie -g
$ ./a.out 
==================
WARNING: ThreadSanitizer: data race (pid=26327)
  Write of size 4 at 0x7f89554701d0 by thread T1:
    #0 Thread1(void*) simple_race.cc:8 (exe+0x000000006e66)

  Previous write of size 4 at 0x7f89554701d0 by thread T2:
    #0 Thread2(void*) simple_race.cc:13 (exe+0x000000006ed6)

  Thread T1 (tid=26328, running) created at:
    #0 pthread_create tsan_interceptors.cc:683 (exe+0x00000001108b)
    #1 main simple_race.cc:19 (exe+0x000000006f39)

  Thread T2 (tid=26329, running) created at:
    #0 pthread_create tsan_interceptors.cc:683 (exe+0x00000001108b)
    #1 main simple_race.cc:20 (exe+0x000000006f63)
==================
ThreadSanitizer: reported 1 warnings
```
4. Technische Hinweise zur Verwendung von ThreadSanitizer. Hierzu zählt dass ThreadSanitizer hauptsächlich nur mit instrumented- code funktioniert. Der Grund dafür ist, dass ThreadSanitizer auf die Informationen aus dem Stack Trace angewiesen ist. Bei non- instrumented Code kann dies nicht gewährleistet werden und somit ist auch der Erfolg von ThreadSanitizer ungewiss. 
***
## Beispiele
## Anwendungen von ThreadSanitizer
***
# Evaluation der Verfügbaren Informationen

