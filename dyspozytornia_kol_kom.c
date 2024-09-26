#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/msg.h>

struct Message {
    long type;
    int data_a;
    int data_b;
    int data_c;
};

int main(int argc, char* argv[]){ 

int a, b, c; //zmienne do ktorych przypsiujemy wylosowane wartosci produktow w zamowieniu
struct Message message;

//Inicjalizacja generatora liczb pseudolosowych seedem bazującym na czasie
//Używając time(NULL) jako seeda, uzyskamy zazwyczaj różne liczby przy każdym uruchomieniu programu.
srand(time(NULL));

//sprawdzamy, czy podano prawidlowa ilosc argumentow
if(argc<6){
  fprintf(stderr, "Za mało argumentów. Użyj:\n");
  fprintf(stderr, "%s <klucz> <liczba_zamówień> <max_A_per_zam> <max_B_per_zam> <max_C_per_zam>\n", argv[0]);
  exit(1);
}
printf("klucz: %s\n", argv[1]);

//zamiana charow na inty
int klucz = atoi(argv[1]);
int liczba_zamowien = atoi(argv[2]);
int max_a = atoi(argv[3])+1;
int max_b = atoi(argv[4])+1;
int max_c = atoi(argv[5])+1;

//tworzenie kolejki komunikatow
int ID_kolejki = msgget(klucz, IPC_CREAT | 0666);
if(ID_kolejki == -1) {
    perror("Błąd tworzenia kolejki komunikatów");
    exit(1);
}

//tworzymy zamowienia - co 1 sekunde wysylamy zamowienie
for(int i=0; i<liczba_zamowien; i++){
  a = rand() % max_a;
  b = rand() % max_b;
  c = rand() % max_c;
  
  //przesylamy zamowienie do kurierow - wrzucamy do kolejki komunikatow
  message.data_a = a;
  message.data_b = b;
  message.data_c = c;
  
  if (msgsnd(ID_kolejki, &message, sizeof(message)-sizeof(long) ,0) == -1) {
        perror("Błąd wpisywania komunikatu 1 do kolejki");
        exit(1);
    } 

  printf("Zlecono zlecenie %d na %dxA, %dxB, %dxC\n", i+1, a, b, c);
  sleep(1);
}
  
//DOPISAC!!!
//Dyspozytornia się wyłącza jak wszystkie procesy magazyny się już wyłączyły. Wówczas wypisywana jest wartość GLD jakie dyspozytornia zapłaciła za wszystkie wykonane zlecenia.  
exit(0);
}




