#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>


int main(int argc, char* argv[]){ 

int pdesk;
int a, b, c; //zmienne do ktorych przypsiujemy wylosowane wartosci produktow w zamowieniu

//Inicjalizacja generatora liczb pseudolosowych seedem bazującym na czasie
//Używając time(NULL) jako seeda, uzyskamy zazwyczaj różne liczby przy każdym uruchomieniu programu.
srand(time(NULL));

//sprawdzamy, czy podano prawidlowa ilosc argumentow
if(argc<6){
  fprintf(stderr, "Za mało argumentów. Użyj:\n");
  fprintf(stderr, "%s <klucz> <liczba_zamówień> <max_A_per_zam> <max_B_per_zam> <max_C_per_zam>\n", argv[0]);
  exit(1);
}
//printf("klucz: %s\n", argv[1]);

//zamiana charow na inty
int liczba_zamowien = atoi(argv[2]);
int max_a = atoi(argv[3])+1;
int max_b = atoi(argv[4])+1;
int max_c = atoi(argv[5])+1;
printf("%d, %d\n", max_a, max_b)

//tworzenie kolejki FIFO
if (mkfifo(argv[1], 0600) == -1){ 
  perror("Błąd utworzenia kolejki FIFO"); 
  exit(1);
}

//otwieramy kolejke FIFO w trybie do zapisu - dystrybutornia zapisuje do kolejki zamowienia, ktore beda odczytywac kurierzy
pdesk = open(argv[1], O_WRONLY); 
if (pdesk == -1){
  perror("Otwarcie potoku do zapisu");
  exit(1); 
}

//tworzymy zamowienia - co 1 sekunde wysylamy zamowienie
for(int i=0; i<liczba_zamowien; i++){
  a = rand() % max_a;
  b = rand() % max_b;
  c = rand() % max_c;
  
  //przesylamy zamowienie do kurierow - wrzucamy do kolejki fifo
  if (write(pdesk, , 7) == -1){ 
    perror("blad zapisu zamowienia do potoku"); 
    exit(1);
  }
}
  
exit(0);
}




