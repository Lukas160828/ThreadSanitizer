# ThreadSanitizer 

## Inhaltsverzeichnis
1. Grundlagen zu Data Races
  * [Wie entsteht ein Data Race](#wie_entsteht_ein_data_race)
  * [Algorithmen zur Erkennung](#algorithmen_zur_erkennung)
2. Aktueller Wissensstand zu ThreadSanitizer
  * [Dokumentation](#dokumentation)
  * [Beispiele](#beispiele)
  * [Anwendungen von ThreadSanitizer](#anwendung-von-threadsanitizer)
3. Benutzung von ThreadSanitizer
  * [Report Format](#report-format)
4. Verwendeter Algorithmus
  * [Instrumentation](#instrumentation)
  * [Die Hybrid State Machine](#die-hybrid-state-machine)
  * [Umgang mit Lese- und Schreibzugriffen](#umgang-mit-lese--und-schreibzugriffen)
  * [Die Überprüfung auf Data Races](#die-überprüfung-auf-data-races)
  * [Beispiel für ein false negative](#beispiel-für-ein-false-positive)
  * [Beispiel für ein false positive](#beispiel-für-ein-false-negative)
5. Fazit

***
# 1. Grundlagen zu Data Races
## Wie entsteht ein Data Race
Ein Data Race ist eine Konstellation, in der mindestens Zwei Threads auf einen geteilten Speicherbereich zugreifen und mindestens einer der Zugriffe die Ressource verändert, ohne einen exklusiven Zugriff darauf zu haben. Dies kann zu unvorhersehbarem Verhalten und nur schwer reproduzierbaren Fehlern führen. Ein Beispiel für ein Data Race findet man [hier](#beispiel1).
## Algorithmen zur Erkennung
Um Data Races zu erkennen, gibt es zwei grundlegende Ansätze- den der Dynamischen und den der Statischen Data Race Vorhersage. Im Statischen Ansatz wird der Source Code analysiert, ohne dass er ausgeführt werden muss. Beim Dynamischen Ansatz werden Events wie Zugriffe auf den Speicher per Trace aufgezeichnet. Dieser Trace ist dann jedoch nur für diesen speziellen Programmablauf gültig. Dies liegt daran, dass Nebenläufigkeit nichtdeterministisch ist und beim erneuten Ausführen eine andere, ebenfalls valide Trace- Reihenfolge möglich ist. Der Ansatz ist nun zu untersuchen, ob eine gültige Restrukturierung des Traces möglich ist, sodass ein Data Race auftritt.

Bei der Dynamischen Data Race Erkennung kommen zwei Hauptsächliche Algorithmen zum Einsatz. Zum einen die Happens-Before Relation und zum anderen die Analyse über locksets.

#### Happens- Before:
Bei der Happens-Before Methode betrachtet man alle Zugriffe auf den Speicher und versucht zwischen ihnen eine Relation bezüglich der Ausführungsreihenfolge herzustellen. Das Ziel hierbei ist herauszufinden, ob es möglich ist, zwei Zugriffe auf dieselbe Resource so umzuordnen dass ein Data Race entsteht. Dies ist allgemein möglich, wenn zwischen zwei Zugriffen keine klare Reihenfolge definiert werden kann. In diesem Fall geht man davon aus, dass man sie potentiell so umordnen kann, dass ein Data Race entsteht.

#### Locksets: 
Bei der Lockset Methode überprüft man, welche Locks von dem jeweiligen Thread zum Zeitpunkt des Zugriffs gehalten werden. Greifen mehrere Threads auf eine geteilte Ressource zu und teilen sich jedoch zusätzlich ein Lock, so schließen die Zugriffe der Threads sich gegenseitig aus. Andere Zugriffe auf dieselbe Adresse können erst stattfinden, wenn der Thread mit dem Lock diesen wieder frei gibt. Dadurch wird ein Data Race verhindert.




***
# 2. Aktueller Wissensstand zu ThreadSanitizer
## Dokumentation
Quelle 1: [ThreadSanitizerCppManual Github](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

Durch eine normale Suche per Google findet man als erstes Ergebnis dieses Github Repository. In dieser Quelle kann man sehr schnell eine Übersicht über die Hauptgesichtspunkte von ThreadSanitizer finden. 


Hierzu zählen:

1. Eine Einführung in Data Races anhand eines [Beispiels](#beispiel1) allgemein

2. Eine Übersicht wo und wie ThreadSanitizer zur Verfügung gestellt wird. Es existiert eine Liste über die unterstützten Plattformen. Die Liste wurde jedoch zuletzt im Dezember 2018 aktualisiert. Es existiert ein Link der auf eine aktuelle Liste zeigen soll, er führt jedoch lediglich auf ein Github Repository in dem das [Header File von ThreadSanitizer](https://github.com/llvm-mirror/compiler-rt/blob/master/lib/tsan/rtl/tsan_platform.h) liegt. Hierbei werden checks über die Plattform im Code ausgeführt, welche man sich anschauen kann. Das bedeuted, hier ist die vollständige Liste theoretisch vorhanden, es ist jedoch nicht direkt ersichtlich da es nicht gesammelt verfügbar ist.
3. Ein [Beispiel]() zur Benutzung von ThreadSanitizer, sowie eine Auflistung der vorhandenen Flags und Blacklist 

4. Technische Hinweise zur Verwendung von ThreadSanitizer. Hierzu zählt, dass ThreadSanitizer nur mit Instrumented-Code funktioniert. Der Grund dafür ist, dass ThreadSanitizer auf die Informationen aus dem Stack Trace angewiesen ist. Bei Non-Instrumented-Code kann dies nicht gewährleistet werden und somit ist auch der Erfolg von ThreadSanitizer ungewiss. 
5. Häufig gestellte Fragen

Quelle 2: [Clang 13 ThreadSanitizer Dokumentation](https://clang.llvm.org/docs/ThreadSanitizer.html)

Man findet auch von Clang eine Dokumentation zu ThreadSanitizer. 
Sie besteht hauptsächlich aus: 
1. Eine Einführung wofür das Tool benutzt wird 
2. Eine Auflistung der über die Plattformen auf denen ThreadSanitizer unterstützt wird
3. Ein Beispiel zur Benutzung von ThreadSanitizer mit der erzeugten Ausgabe. Was diese Dokumentation von der Quelle 1 maßgeblich abhebt, ist die Beschreibung über den Umgang mit ThreadSanitizer im Makefile. Es wird erklärt, wie man Teile des Codes von ThreadSanizier ausschließt, damit diese nicht überprüft werden, sie jedoch weiterhin instrumented werden um false positives durch Non-Instrumented-Code auszuschließen. Auch wird gezeigt, wie man die Instrumentation hier komplett ausschalten kann. Dies kann zu verfälschten Ergebnissen durch fehlende Stack Frames führen 
4. Einschränkungen der Benutzung von ThreadSanitizer sowie erwarteter Speicheraufwand 

Wissen zu Data Races wird hierbei vorrausgesetzt, da diese nicht erklärt werden.

***
## Beispiele
Quelle 1: [ThreadSanitizerCppManual Github](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

Eine Erklärung, wie Data Races entstehen und was sie bewirken können. Das gewählte Beispiel für ein Data Race ist jedoch etwas unübersichtlich.
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
Dieses Beispiel finde ich nicht so gut gewählt, da es nicht erklärt wird, wodurch hier das Data Race entsteht. Hierbei wären ein paar Kommentare für die Übersicht hilfreich. Auch könnte man den Variablen aussagekräftigere Namen geben als nur Buchstaben.

Eine Übersicht wie man ThreadSanitizer verwenden kann. Das Beispiel hierfür ist gut gewählt, da es sehr intuitiv verständlich ist. 

### <a name="beispiel1"></a>
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
Auch wird die Konsole gezeigt, also welche Eingaben getätigt wurden und welches Ergebnis erzielt wurde. Dies führt dazu, dass man es gut nachstellen kann und einen beispielhaften Anwendungsfall hat. 

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
# 3. Benutzung von ThreadSanitizer
## Report Format
Eine ausführliche Beschreibung wie Reports aufgebaut sind findet man [hier](https://github.com/google/sanitizers/wiki/ThreadSanitizerReportFormat).

Allgemein beschrieben wird der Report als einzelne Blöcke ausgegeben. Diese Blöcke beinhalten Informationen zu Aktionen, die zu dem Fehler geführt haben, oder Threads, aus denen diese Aktionen ausgeführt wurden. Hier ein Beispiel für einen Report:
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
Die ersten Beiden Blöcke beinhalten die Zugriffe, die aus unterschiedlichen Threads heraus, auf dieselbe Adresse ausgeführt werden. Hierdurch wird der Fehler hervorgerufen.

Der zweite Block beinhaltet Informationen zu den Threads, die am Problem beteiligt sind. In diesem Fall ist es neben dem Main Thread, welcher nicht aufgezählt wird, nur der Thread T1.

Jeder Block beinhaltet einen Stack Frame an Instruktionen, die ausgeführt wurden, um den Fehler hervorzurufen und wo genau diese Instruktionen stattgefunden haben.

Zusätzlich gibt es auch Blöcke, die Informationen über eine Synchronisation von Threads per sleep Funktion beinhalten. Auch gibt es Blöcke, die über die Verwendung von Mutexes informieren und in welchem Zustand sie sich bei den jeweiligen Speicherzugriffen befanden:
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
***
# 4. Verwendeter Algorithmus
Im Folgenden werde ich die Funktionsweise des Algorithmus von ThreadSanitizer zur Erkennuung von Data Races textuell beschreiben. Eine Formale Definition findet man [hier](http://www.cs.columbia.edu/~junfeng/11fa-e6121/papers/thread-sanitizer.pdf) (Seite 63 ff).
## Instrumentation
ThreadSanitizer betrachtet den Programmfluss als Abfolge von Events. Die wichtigsten Events zur Erkennung von Data Races sind zum einen Zugriffe auf den Speicher wie Schreiben oder Lesen. Hier wird grundlegend jeder Zugriff instrumentiert, außer wenn man weiß, dass dadurch kein Data Race entstehen kann oder er redundant ist.

Ein Beispiel für Zugriffe ohne Data Race ist:
* Lesezugriffe auf globale Konstanten

Ein Beispiel für einen Redundanten Zugriff ist:
* Lesezugriffe die sequentiell vor Schreibzugriffen stattfinden

Weitere wichtige Ereignisse sind Synchronisierende Events, wie die Aneignung oder Freigabe eines Locks. Dies ist wichtig, um die Happens-Before Relation festzustellen.
## Die Hybrid State Machine
Der Zustand von ThreadSanitizer besteht aus einem globalem Zustand und einzelnen Zuständen für jede Speicheradresse, auf die zugegriffen wird. Der globale Zustand beschreibt die bisher beobachteten Synchronisationsevents. 

Für jede Speicheradresse gibt es einen sogenannten per-ID Zustand, in welchem alle Lese- und Schreibzugriffe aus allen Threads auf die jeweilige Adresse gespeichert werden. Diese werden in einem Schreibzugriff Segment-Set (SS<sub><sup>wr</sup></sub>) und einem Lesezugriff Segment-Set (SS<sub><sup>rd</sup></sub>) gespeichert. Man speichert in dem SS<sub><sup>rd</sup></sub> nur Segmente ab, bei denen keine Happens-Before Relation zu einem Lesezugriffs-Segment aus dem SS<sub><sup>wr</sup></sub> definiert werden kann. Der Grund dafür ist, dass nur wenn keine partielle Ordnung hergestellt werden kann, durch die Happens- Before Relation ein Data Race erkannt werden kann.

Der globale, und per-ID Zustand wird nach jedem Speicherzugriff aktualisiert und überprüft, ob ein Data Race vorliegt. Mehr dazu im nächsten Abschnitt.
## Umgang mit Lese- und Schreibzugriffen
Wird erkannt, dass ein Zugriff auf den Speicher stattfindet, wird ein Handler aufgerufen mit der ID des Threads, der auf die Ressource zugreift, die ID des Speicherorts auf den zugegriffen wird, und einem Flag ob es ein Lese- oder Schreibzugriff ist.

Im Handler wird zunächst über die Thread ID das aktuelle Segment und über die ID des Speicherorts der dazugehörige per-ID State geladen. Dort stehen alle bisher erfolgten Lese- und Schreibzugriffe (SS<sub><sup>rd</sup></sub>, SS<sub><sup>wr</sup></sub>), zwischen denen man keine Happens-Before Relation definieren kann. 

Nun werden SS<sub><sup>rd</sup></sub> und SS<sub><sup>wr</sup></sub> um den Aufruf erweitert, so dass sie immer noch ihren Definitionen entsprechen. Das bedeutet, dass man für kein Segment aus SS<sub><sup>rd</sup></sub> eine Happens-Before Relation auf ein Segment aus SS<sub><sup>wr</sup></sub> definieren kann. Anhand der aktuellen Segment-Sets wird nun der per-ID Zustand aktualisiert und überprüft, ob ein Data Race erkannt werden kann.

## Die Überprüfung auf Data Races
Die Überprüfung, ob ein Data Race erkannt werden kann, findet nach jedem Zugriff auf den Speicher statt, nachdem Segment-Sets SS<sub><sup>rd</sup></sub> und SS<sub><sup>wr</sup></sub> aktualisiert wurden. 

Zuerst iteriert man über SS<sub><sup>wr</sup></sub> und vergleicht das Schreib-Segment mit jedem anderen Schreib-Segment. Hierzu wird zuerst überprüft, ob man zwischen den beiden Segmenten in der aktuellen Iterationsstufe eine Happens-Before Relation definieren kann. Falls ja, dann geht man davon aus, dass kein Data Race vorliegt.

Nachdem man nun das Segment der aktuellen Iterationsstufe mit allen Schreib-Segmenten in SS<sub><sup>wr</sup></sub> verglichen hat, vergleicht man es nun noch mit allen Lese-Segmenten in SS<sub><sup>rd</sup></sub>. Da man durch die Definition schon weiß, dass keines der Segmente aus SS<sub><sup>rd</sup></sub> eine Relation auf Segmente aus SS<sub><sup>wr</sup></sub> aufweisen, muss nun lediglich die andere Richtung überprüft werden. Weist nun also das Segment der aktuellen Iterationsstufe keine Happens-Before Relation auf das aktuelle Lese-Segment auf, wissen wir, dass sie ungeordnet sind. Ist dies der Fall, wird ein Data Race berichtet.
## Beispiel bei dem der Algorithmus funktioniert:
<picture>
  <img src= bsp1(1).png>
</picture>

Beim ersten Beispiel handelt es sich lediglich um zwei Lesezugriffe auf dieselbe Resource, aus verschiedenen Threads heraus. Dieser Fall tritt Bei unserem vorherigen [Beispiel](#beispiel1) auf. Die Zugriffe werden ohne Absicherungen wie zum Beispiel Locks durchgeführt, und weisen somit das Potential für ein Data Race auf. 

Schritt 1:

<picture>
  <img src= bsp1(2).png>
</picture>

Der erste Schreibzugriff " Global++" von T1 wurde erkannt und ThreadSanitizer aufgerufen.


<picture>
  <img src= bsp1state1.png>
</picture>

 Nun wird der per-ID State(bestehend aus SS<sub><sup>rd</sup></sub> und SS<sub><sup>wr</sup></sub>) aktualisiert. Der aktuelle Zugriff wird als Schreibzugriff zu SS<sub><sup>wr</sup></sub> hinzugefügt.

<picture>
  <img src= bsp1(3).png>
</picture>

Nun wird der zweite Schreibzugriff "Global--" von T2 erkannt, bei welchem jetzt ein Data Race auftreten kann.

<picture>
  <img src= bsp1state2.png>
</picture>

Zuerst wird überprüft, ob der aktuelle Zugriff in einer Happens-Before Relation zu einem existierenden Eintrag steht. Da keine übergreifenden Locks zwischen den Threads verwendet werden, kann keine Abhängigkeit zwischen den Zugriffen festgestellt werden. Dadurch wird kein bestehender Eintrag herausgelöscht und SS<sub><sup>wr</sup></sub> um den neuen Zugriff ergänzt.

Nun wird überprüft, ob im aktuellen State ein Data Race erkannt werden kann. Dies wird getan, indem als erstes der neue Zugriff mit allen anderen Schreibzugriffen auf dieselbe Ressource(Also allen Einträgen aus SS<sub><sup>wr</sup></sub>) verglichen wird. Als erstes wird der Zugriff von T2 mit dem Zugriff von T1 verglichen. Dabei kann keine Happens-Before Relation zwischen den Zugriffen definiert werden.

Dies Bedeutet, dass es zwei Zugriffe auf dieselbe Ressource gibt, die nicht per Happens-Before Relation geordnet sind. Dadurch geht man davon aus, dass an dieser Stelle ein Data Race auftreten kann und ThreadSanitizer berichtet dies.

Einen Source Code der diesen Ausführungstrace erzeugen kann, finden sie in dem File Data_Race_Beispiel_2.cpp.

## Beispiel für ein false negative:
Der folgende Code zeigt ein Beispiel für ein Programm was ein potentiellen Data Race aufweißt, es jedoch auch eine Ausführungsreihenfolge existiert bei welcher ThreadSanitizer diesen nicht erkennt.

```cpp
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
```
Der aufgezeichnete Trace sei wie folgt:

<picture>
  <img src= bsp2(1).png>
</picture>

 Auch in diesem Beispiel finden zwei Schreibzugriffe aus verschiedenen Threads heraus statt. Zusätzlich werden Locks verwendet, die jedoch einen der Zugriffe nicht absichert. 

<picture>
  <img src= bsp2(2).png>
</picture>

Der erst Schreibzugriff von T1 findet statt, bevor der Thread einen Lock hält.

<picture>
  <img src= bsp2state1.png>
</picture>

Der Zugriff wird im per-ID State notiert.

<picture>
  <img src= bsp2(3).png>
</picture>

Nun findet der Zugriff von T2 statt, welcher durch ein Lock abgesichert wird. 

<picture>
  <img src= bsp2state2.png>
</picture>


Wenn in diesem Fall der per-ID State aktualisiert wird, wird der Zugriff von T2 mit allen anderen Zugriffen verglichen. Beim Vergleich mit dem Zugriff von T1 erkennt ThreadSanitizer hier eine Happens-Before Relation, zwischen den Zugriffen, welche jedoch nicht existiert. Der Grund für dieses Phänomen ist dass die Happens-Before Relation eine Ordnung von w(a) < rel(y) in T1 und acq(y) < w(a) in T2 erkennt. Durch die Transitivität der Happens- Before Relation wird nun fälschlicherweise w(a)T1 < w(a)T2 festgestellt.  Dies führt dazu, dass der Zugriff von T1 aus SS<sub><sup>wr</sup></sub> herausgenommen wird. Dies liegt daran, da alle Zugriffe die geordnet vor einem anderen passieren, für die Vergleiche auf die Happens-Before Relation irrelevant sind, anhand der Transitivität.

Wenn nun der aktuelle State auf ein Data Race untersucht wird, werden alle Schreibzugriffspaare auf ein Data Race untersucht. Jedoch ist der erste Zugriff von T1 in SS<sub><sup>wr</sup></sub> nicht mehr vorhanden und somit wird das paar nicht untersucht. Dadurch jedoch dass der Schreibzugriff aus T1 nicht abgesichert ist, existiert eine valide Umordnung des Traces, in der die Schreibzugriffe in direkter nachfolge ausgeführt werden. Dies bedeutet, dass hier ein potentieller Data Race nicht erkannt wird.f

Einen Source Code der diesen Ausführungstrace erzeugen kann finden sie in dem File false_negative.cpp.

Wie dieses Beispiel zeigt, ist der Algorithmus nicht vollständig, da es potentielle Data Races gibt die nicht erkannt werden.

## Beispiel für ein false positive
Bei folgendem Beispiel wird ThreadSanitizer einen Data Race melden obwohl keiner existiert.

```cpp
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
```

Der aufgezeichnete Trace sei wie folgt:

<picture>
  <img src= bsp3(1).png>
</picture>

Die in diesem Beispiel von ThreadSanitizer aufgezeichneten Events sind die zwei Schreibzugriffe auf "Condition" und "Global" von T1. Sowie den Lesezugriff auf "Condition" und Schreibzugriff auf "Global" von T2.

<picture>
  <img src= bsp3(2).png>
</picture>

Der erste Schreibzugriff "Condition = 1" von T1

<picture>
  <img src= bsp3state1.png>
</picture>

Der per-ID State wird um aktuellen Aufruf ergänzt.

<picture>
  <img src= bsp3(3).png>
</picture>

Zweiter Schreibzugriff "Global = 1" von T1

<picture>
  <img src= bsp3state2.png>
</picture>

Der per-ID State wird um aktuellen Aufruf ergänzt.

<picture>
  <img src= bsp3(4).png>
</picture>

T2 liest "Condition" 

<picture>
  <img src= bsp3state3.png>
</picture>

Da kein Lock oder ähnliche Synchronisationsmechanismen verwendet wurden, kann zwischen dem neuen Lesezugriff und den bereits vermerkten Schreibzugriffen keine Relation hergestellt werden. Heißt er wird in SSrd aufgenommen.

Beim Check ob ein Data Race stattfindet, der nach jedem erkannten Event durchgeführt wird, wird nun erkannt dass es einen Schreib- und einen Lesezugriff auf die selbe Variable gibt. Weiter sind die beiden Events ungeordnet, heißt hier wird ein Data Race erkannt. Dieser Data Race ist findet tatsächlich statt, der false positive tritt im nächsten Schritt auf.

<picture>
  <img src= bsp3(5).png>
</picture>

T2 schreibt "Global = 2"

<picture>
  <img src= bsp3state4.png>
</picture>

Wieder kann keine Happens-Before Relation zwischen dem aktuellen Schreibzugriff und den bereits notierten Zugriffen festgestellt werden. Dies bedeutet dass der Schreibzugriff zu SSwr hinzugefügt wird. 

Nun wird wieder auf ein Data Race überprüft. Hierbei wird erkannt dass die Zugriffe T1_w(b) und T2_w(b) die auf die selbe Variable Zugreifen aus verschiedenen Threads heraus. Des weiteren sind die Events ungeordnet nach der Happens-Before Relation. Dies bedeutet dass hier ein Data Race erkannt wird weil ThreadSanitizer davon ausgeht dass es eine valide Umformung des Traces gibt, sodass T1_w(b) und T2_w(b) direkt nebeneinander stehen. Zum Beispiel den folgenden Trace:

<picture>
  <img src= bsp3(6).png>
</picture>

Dieser ist jedoch gar nicht möglich, da t2_w(b) nur stattfindet wenn T1_w(a) vorher stattgefunden hat und dadurch die Bedingungsvariable auf 1 gesetzt ist. Heißt hier gibt es eine Abhängigkeit der Ausführungsreihenfolge die durch die Happens-Before Relation nicht erfasst wird. 

ThreadSanitizer meldet ein Data Race das so nicht entstehen kann und foglich nicht existiert.

***

# 5. Fazit
Data Races sind ein extrem schwer zu erkennendes Problem. ThreadSanitizer geht dies mit einem Algorithmus basierend auf der Lamport's Happens-Before Relation an. Der Algorithmus ist nicht Vollständig, da false negatives existieren, und nicht komplett genau, da false positives existieren.  Im allgemeinen können jedoch sehr gute Ergebnisse erzielt werden, was die Zuverlässigkeit der Erkennung von Data Races angeht. 

Die Grundlegende Funktion des Tools ist einfach zu verwenden und man findet im Internet leicht Informationen, wie die Ergebnisse zu Interpretieren sind. 

Diese Eigenschaften zeichnen das Tool als hilfreich und produktivitätssteigernd aus. Für die Entwicklung eines Mehrläufigen Programms empfiehlt sich die Benutzung um besagte Data Races frühzeitig zu erkennen und zu beheben.
 




