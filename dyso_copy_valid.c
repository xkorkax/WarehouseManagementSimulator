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

int a, b, c;                                                //zmienne do ktorych przypsiujemy wylosowane wartosci produktow w zamowieniu
int ID_kolejki;                                             //ID kolejki, dzięki której komunikuje się dyspozytornia z kurierami
int pracujacy_kurierzy = 9;                                 //ile kurierów jeszcze działa
int suma_do_zaplaty = 0;                                    
struct Message order;                                       //struktura słuząca do przesyłania zamowien, typ1
order.type = 1;
struct Message feedback;

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


ID_kolejki = msgget(klucz, IPC_CREAT | 0666);               //tworzymy kolejke komunikatow/otwieramy juz istniejaca
msgctl(ID_kolejki, IPC_RMID, 0);                            //usuwamy kolejke, zeby nie korzystac przypadkiem z jakiejs starej

ID_kolejki = msgget(klucz, IPC_CREAT | 0666);               //tworzenie nowej kolejki komunikatow
if(ID_kolejki == -1) 
{
    perror("Błąd tworzenia kolejki komunikatów");
    exit(1);
}


//tworzymy zamowienia - co 0.25 sekundeywysylamy zamowienie
for(int i=0; i<liczba_zamowien; i++)
{
    a = rand() % (max_a-1) +1;
    b = rand() % (max_b-1) +1;
    c = rand() % (max_c-1) +1;
  
    //przesylamy zamowienie do kurierow - wrzucamy do kolejki komunikatow - wiadomosc typu 1
    order.data_a = a;
    order.data_b = b;
    order.data_c = c;
  
    if(msgsnd(ID_kolejki, &order, sizeof(order)-sizeof(long) ,0) == -1) 
    {
        perror("Błąd wpisywania komunikatu przez do kolejki");
        exit(1);
    } 
 
    printf("Zlecono zlecenie %d na %dxA, %dxB, %dxC\n", i+1, a, b, c);
    usleep(250000);
}
  

//odbieranie wiadomosci zwrotnej od kuriera, o tym ile trzeba zapłacic
while(pracujacy_kurierzy > 0)
{
    if(msgrcv(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long), 2, 0)== -1)
    {
        perror("Błąd odbierania wiadomości zwrotnej przez magazyn");
    } 
    else
    {
        if(feedback.data_a > 0) 
        {
            suma_do_zaplaty += feedback.data_a;
        } 
        else 
        {
            pracujacy_kurierzy--;
            printf("kurier out:(\n");
        }
    }
}

//gdy wszystkie procesy magazyny się wyłączyły to dyspozytornia wypisuje sumę GLD jaką zapłaciła i też się wyłącza
printf("ostateczna suma do zaplaty: %d\n", suma_do_zaplaty); 
exit(0);
}