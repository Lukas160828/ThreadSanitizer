# ThreadSanitizer 

## Inhaltsverzeichnis
1. Aktueller Wissensstand
  * [Dokumentation](#dokumentation)
  * [Beispiele](#beispiele)
  * [Anwendungen von ThreadSanitizer](#anwendung-von-threadsanitizer)
2. Benutzung von ThreadSanitizer
  * [Report Format](#report-format)
  * [false positives](#false-positives)
  * [false negatives](#false-negatives)

***
# 1. Aktueller Wissensstand
## Dokumentation
Quelle 1: [ThreadSanitizerCppManual Github](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

Durch eine normale Suche per Google findet man als erstes Ergebnis dieses Github Repository. In dieser Quelle kann man sehr schnell eine Übersicht über die Hauptgesichtspunkte von ThreadSanitizer finden. 


Hierzu zählen:

1. Eine Einführung in Data Races anhand eines [Beispiels]() allgemein

2. Eine Übersicht wo und wie ThreadSanitizer zur Verfügung gestellt wird. Es existiert eine Liste über die unterstützten Platformen. Die Liste wurde jedoch zuletzt im Dezember 2018 aktualisiert. Es existiert ein Link der auf eine aktuelle Liste zeigen soll, er führt jedoch lediglich auf ein Github Repository in dem das [Header File von ThreadSanitizer](https://github.com/llvm-mirror/compiler-rt/blob/master/lib/tsan/rtl/tsan_platform.h) liegt. Hierbei werden checks über die Platform im Code ausgeführt, welche man sich anschauen kann. Heißt hier ist die vollständige Liste theoretisch vorhanden, es ist jedoch nicht direkt ersichtlich da es nicht gesammelt verfügbar ist.
3. Ein [Beispiel]() zur Benutzung von ThreadSanitizer, sowie eine Auflistung der vorhandenen Flags und Blacklist 

4. Technische Hinweise zur Verwendung von ThreadSanitizer. Hierzu zählt dass ThreadSanitizer hauptsächlich nur mit instrumented- code funktioniert. Der Grund dafür ist, dass ThreadSanitizer auf die Informationen aus dem Stack Trace angewiesen ist. Bei non- instrumented Code kann dies nicht gewährleistet werden und somit ist auch der Erfolg von ThreadSanitizer ungewiss. 
5. Häufig gestellte Fragen

Quelle 2: [Clang 13 ThreadSanitizer Dokumentation](https://clang.llvm.org/docs/ThreadSanitizer.html)

Man findet auch von Clang eine Dokumentation zu ThreadSanitizer. 
Sie besteht Hauptsächlich aus: 
1. Eine Einführung wofür das Tool benutzt wird 
2. Eine Auflistung der über die Platformen auf denen ThreadSanitizer unterstützt wird
3. Ein Beispiel zur Benutzung von ThreadSanitizer mit der erzeugten Ausgabe. Was diese Dokumentation von der Quelle 1 maßgeblich abhebt ist die Beschreibung über den Umgang mit ThreadSanitizer im Makefile. Es wird erklärt wie man Teile des Codes von ThreadSanizier ausschließt damit diese nicht überprüft werden, sie jedoch weiterhin instrumented werden um false positives durch uninstrumented code auszuschließen. Auch wird gezeigt wie man die Instrumentation hier komplett ausschalten kann. Dies kann zu verfälschten Ergebnissen führen durch fehlende Stack Frames
4. Einschränkungen der Benutzung von ThreadSanitizer sowie erwarter Speicheraufwand 

Wissen zu Data Races wird hierbei vorrausgesetzt da diese nicht erklärt werden.

***
## Beispiele
Quelle 1: [ThreadSanitizerCppManual Github](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

Eine Erklärung wie Data Races Entstehen und was sie Bewirken können. Das Gewählte Beispiel für ein Data Race ist jedoch etwas unübersichtlich.
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

Eine Übersicht wie man ThreadSanitizer verwenden kann. Das Beispiel hierfür ist gut gewählt, da es sehr intuitiv verständlich ist. 

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

## Anwendungen von ThreadSanitizer
***
# 2. Benutzung von ThreadSanitizer
## Report Format
Eine ausführliche Beschreibung wie Reports aufgebaut sind findet man [hier](https://github.com/google/sanitizers/wiki/ThreadSanitizerReportFormat).

Allgemein beschrieben wird der Report als einzelne Blöcke ausgegeben. Diese Blöcke beinhalten dann Informationen zu Aktionen die zu dem Fehler geführt haben oder Threads aus denen diese Aktionen ausgeführt wurden. Hier ein Beispiel für einen Report:
```
WARNING: ThreadSanitizer: data race (pid=9337)
  Write of size 4 at 0x7fe3c3075190 by thread T1:
    #0 foo1() simple_stack2.cc:9 (exe+0x000000003c9a)
    #1 bar1() simple_stack2.cc:16 (exe+0x000000003ce4)
    #2 Thread1(void*) simple_stack2.cc:34 (exe+0x000000003d99)

  Previous read of size 4 at 0x7fe3c3075190 by main thread:
    #0 foo2() simple_stack2.cc:20 (exe+0x000000003d0c)
    #1 bar2() simple_stack2.cc:29 (exe+0x000000003d74)
    #2 main simple_stack2.cc:41 (exe+0x000000003ddb)

  Thread T1 (tid=9338, running) created at:
    #0 pthread_create tsan_interceptors.cc:683 (exe+0x00000000de83)
    #1 main simple_stack2.cc:40 (exe+0x000000003dd6)
```
Die ersten Beiden Blöcke beinhalten die Zugriffe, die auf die selbe Adresse ausgeführt werden aus unterschiedlichen Threads heraus. Hierdurch wird der Fehler hervorgerufen.

Der zweite Block beinhaltet Informationen zu den Threads die am Problem beteiligt sind. In diesem Fall ist es neben dem main thread, welcher nicht aufgezählt wird, nur der Thread T1.

Jeder Block beinhaltet einen Stack Frame an Instruktionen die ausgeführt wurden um den Fehler hervorzurufen und wo genau diese stattgefunden haben.

Zusätzlich gibt es auch Blöcke die Informationen über eine Synchronisation von Threads per sleep Funktion. Auch gibt es Blöcke die über die Verwendung von Mutexes informieren und in welchem Zustand sie sich bei den jeweiligen Speicherzugriffen befanden:
```
  Write of size 4 at 0x7f2f35fca190 by thread T1 (mutexes: write M1):
...
  Previous write of size 4 at 0x7f2f35fca190 by thread T2 (mutexes: write M2, read M3):
...
  Mutex M1 created at:
    #0 pthread_mutex_init tsan_interceptors.cc:737 (exe+0x00000000d9b1)
    #1 main mutexset6.cc:31 (exe+0x000000003e67)

  Mutex M2 created at:
    #0 pthread_spin_init tsan_interceptors.cc:796 (exe+0x00000000d091)
    #1 main mutexset6.cc:32 (exe+0x000000003e78)

  Mutex M3 created at:
    #0 pthread_rwlock_init tsan_interceptors.cc:839 (exe+0x00000000c8f1)
    #1 main mutexset6.cc:33 (exe+0x000000003e89)
```

