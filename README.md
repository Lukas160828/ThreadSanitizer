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

3. Eine Übersicht wie man ThreadSanitizer verwenden kann. 

Unter anderem eine Erklärung was Data Races sind, wie man ThreadSanitizer benutzen kann und welche Platformen unterstützt werden. 
***
## Beispiele
## Anwendungen von ThreadSanitizer
***
# Evaluation der Verfügbaren Informationen

