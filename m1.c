#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/msg.h>
#include <signal.h>

struct Message {
    long type;
    int data_a;
    int data_b;
    int data_c;
};


int main(int argc, char* argv[]){ 
  
  int desc_pliku_konf;
  int klucz_kolejki = atoi(argv[2]);
  int ID_kolejki;
  int ID_procesu_pierwotnego;
  int zrealizowane_zamowienia = 0;
  int pid;
  int a = 0;
  int b = 0;
  int c = 0;
  int pracujacy_kurierzy = 3;
  int stanA, stanB, stanC;
  int cenaA, cenaB, cenaC;
  int pipe1_desc[2], pipe2_desc[2], pipe3_desc[2], pipe4_desc[2], pipe5_desc[2], pipe6_desc[2];
  int do_zaplaty1 = -1; 
  int do_zaplaty11 = -1;
  int do_zaplaty2 = -1;
  int do_zaplaty21 = -1;
  int do_zaplaty3 = -1;
  int do_zaplaty31 = -1;
  int suma_do_zaplaty = 0;
  struct Message message;
  struct Message message2;
  
  ID_procesu_pierwotnego = getpid();
  //printf("ID: %d\n", ID_procesu_pierwotnego);
  
  //sprawdzamy, czy podano prawidlowa ilosc argumentow
  if(argc<2){
    fprintf(stderr, "Za mało argumentów. Użyj:\n");
    fprintf(stderr, "%s <ścieżka do pliku konfiguracyjnego> <klucz    dostepu do kolejki>\n", argv[0]);
    exit(1);
  }
  
  //otwieramy plik konfiguracyjny
  desc_pliku_konf = open(argv[1], O_RDONLY);
  if(desc_pliku_konf == -1){
    perror("Blad otwarcia pliku konfiguracyjnego");
    exit(1);
  }
  
  int stan_magazynu[6];
  
  //wczytujemy dane z pliku
  char buffer[1];  // Wczytujemy po jednym znaku
  int bytesRead, index = 0;
  int value = 0;
  
  while((bytesRead = read(desc_pliku_konf, buffer, sizeof(buffer))) > 0) {
    if(buffer[0] == ' ' || buffer[0] == '\n' || buffer[0] == '\t') {
        // Jeśli napotkaliśmy spację, nową linię lub tabulator, zakończ parsowanie liczby
        stan_magazynu[index++] = value;
        value = 0;                          // Resetujemy wartość dla kolejnej liczby
        } else {
          value = value * 10 + (buffer[0] - '0');       // Parsowanie cyfry
        }
    }

    // Sprawdzamy, czy ostatnia liczba została wczytana
    if (value != 0) {
        stan_magazynu[index++] = value;
    }

  stanA = stan_magazynu[0];
  cenaA = stan_magazynu[1];
  stanB = stan_magazynu[2];
  cenaB = stan_magazynu[3];
  stanC = stan_magazynu[4];
  cenaC = stan_magazynu[5];

  close(desc_pliku_konf);                                                     //zamykamy deskryptor pliku
  
  //twprzymy potoki, ktorymi beda komunikowali sie kurierzy z magazynem
  if(pipe(pipe1_desc) == -1 || pipe(pipe2_desc) == -1 || pipe(pipe3_desc) == -1 || pipe(pipe4_desc) == -1 || pipe(pipe5_desc) == -1 || pipe(pipe6_desc) == -1){ 
    perror("Tworzenie potoku"); 
    exit(1);
  }
  

  
  //po załadowaniu pliku konfiguracyjnego tworzeni są kurierzy  (podprocesy procesu magazynu i jest ich dla każdego magazynu 3) nasłuchujący na kanale globalnym dyspozytorni, oraz tworzą kanał komunikacji z magazynem
  
  int pom, pom2, pom3;
  int id_K1,id_K2, id_K3;
  
  //tworzymy trzech kurierow
  pom = fork();               
      
  if(pom > 0){
    pom2 = fork();
    
    if(pom2 == 0){
      id_K2 = getpid();
    } 
    else{
      pom3 = fork(); 
      if(pom3 == 0){
        id_K3 = getpid();
      }  
    }
  } else {
       id_K1 = getpid();
  }
  
  printf("ID: %d, parent: %d\n", getpid(), getppid());    //wyświetlamy kurierów
  
  //od tego miejsca wykonuja sie 4 procesy
  
  //kurierzy nasluchuja na kanale globalnym - odbieraja wiadomosc z kolejki komunikatow  
  
  //otwieranie kolejki komunikatow przez kurierow
  if(getpid() != ID_procesu_pierwotnego){
     ID_kolejki = msgget(klucz_kolejki, 0666);            
     
     if(ID_kolejki == -1) {
        perror("Błąd otwarcia kolejki komunikatów");
        exit(1);
    }
  }
    
  //zamykanie niepotrzebnych deksryptorów
  if(getpid() == id_K1){  
    close(pipe1_desc[0]);
    close(pipe4_desc[1]);
  } else if(getpid() == id_K2){  
    close(pipe2_desc[0]);
    close(pipe5_desc[1]);
  } else if(getpid() == id_K3){  
    close(pipe3_desc[0]);
    close(pipe6_desc[1]);
  } else {
    close(pipe1_desc[1]);
    close(pipe4_desc[0]);
    close(pipe2_desc[1]);
    close(pipe5_desc[0]);
    close(pipe3_desc[1]);
    close(pipe6_desc[0]);
  }
  
  while(pracujacy_kurierzy>0){
    if(pid == id_K1){
     sleep(1);
      //kurier odbiera zamowienia z glownego kanalu
      if(msgrcv(ID_kolejki, &message, sizeof(message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
        printf("Bląd odbierania zamowienia z dyspozytorni");
      }
      else {
        printf("zamowienie odebral kurier 1 magazunu 1: A: %d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
          //realizowanie zamowienia
          //wysylamy do magazynu stan zamowienia
          if (write(pipe1_desc[1], &message.data_a, sizeof(int)) == -1 ||
              write(pipe1_desc[1], &message.data_b, sizeof(int)) == -1 ||
              write(pipe1_desc[1], &message.data_c, sizeof(int)) == -1) {
                perror("Błąd podczas zapisywania do potoku");
                return 1;
          } else{
              printf("kurier 1 wyslał zapytanie do magazynu 1: A:%d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
          }
        }  
    } else if(pid == id_K2){
    sleep(1);
      //kurier odbiera zamowienia z glownego kanalu
      if(msgrcv(ID_kolejki, &message, sizeof(message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
        printf("Bląd odbierania zamowienia z dyspozytorni");
      }
      else {
        printf("zamowienie odebral kurier 2 magazunu 1: A: %d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
          //realizowanie zamowienia
          //wysylamy do magazynu stan zamowienia
          if (write(pipe2_desc[1], &message.data_a, sizeof(int)) == -1 ||
              write(pipe2_desc[1], &message.data_b, sizeof(int)) == -1 ||
              write(pipe2_desc[1], &message.data_c, sizeof(int)) == -1) {
                perror("Błąd podczas zapisywania do potoku");
                return 1;
          } else{
              printf("kurier 2 wyslał zapytanie do magazynu 1: A:%d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
          }
        }  
    
    } else if(pid == id_K3){
     sleep(1);
      //kurier odbiera zamowienia z glownego kanalu
      if(msgrcv(ID_kolejki, &message, sizeof(message)-sizeof(long), 0, IPC_NOWAIT)==-1) {
        printf("Bląd odbierania zamowienia z dyspozytorni");
      }
      else {
        printf("zamowienie odebral kurier 3 magazunu 1: A: %d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
          //realizowanie zamowienia
          //wysylamy do magazynu stan zamowienia
          if (write(pipe3_desc[1], &message.data_a, sizeof(int)) == -1 ||
              write(pipe3_desc[1], &message.data_b, sizeof(int)) == -1 ||
              write(pipe3_desc[1], &message.data_c, sizeof(int)) == -1) {
                perror("Błąd podczas zapisywania do potoku");
                return 1;
          } else{
              printf("kurier 3 wyslał zapytanie do magazynu 1: A:%d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
          }
        }  
    } 
    
    else {
    //kod dla procesu macierzystego
    //sprawdzamy, czy magazyn moze zrealizowac zamowienie
    //odbieramy z pipe

  // Odczytaj wartości z potoku1
  if (read(pipe1_desc[0], &message2.data_a, sizeof(int)) == -1 ||
      read(pipe1_desc[0], &message2.data_b, sizeof(int)) == -1 ||
      read(pipe1_desc[0], &message2.data_c, sizeof(int)) == -1) {
      perror("Błąd odczytu z potoku");
      // Dodaj odpowiednie działania w przypadku błędu
  } else {
      // Wartości zostały pomyślnie odczytane
      printf("magazyn mowi: Odczytane wartości: %d, %d, %d\n", message2.data_a, message2.data_b, message2.data_c);
      
      if(message2.data_a <= stanA && message2.data_b <= stanB && message2.data_c <= stanC) {
        printf("magazyn mowi: mozna zrealizowac zamowienie\n");
        stanA -= message2.data_a;
        stanB -= message2.data_b;
        stanC -= message2.data_c;
        do_zaplaty1 = (message2.data_a*cenaA) + (message2.data_b*cenaB) + (message2.data_c*cenaC);
        write(pipe4_desc[1], &do_zaplaty1, sizeof(int));
        printf("magazyn mowi: do zaplaty: %d\n", do_zaplaty1);
        
      } else {
        printf("%d, %d, %d\n", cenaA, cenaB, cenaC);
        do_zaplaty1 = 0;
        printf("magazyn mowi: NIE mozna zrealizowac zamowienia\n");
        write(pipe4_desc[1], &do_zaplaty1, sizeof(int));
      }
      a = 1;
}

 // Odczytaj wartości z potoku2
  if (read(pipe2_desc[0], &message2.data_a, sizeof(int)) == -1 ||
      read(pipe2_desc[0], &message2.data_b, sizeof(int)) == -1 ||
      read(pipe2_desc[0], &message2.data_c, sizeof(int)) == -1) {
      perror("Błąd odczytu z potoku");
      // Dodaj odpowiednie działania w przypadku błędu
  } else {
      // Wartości zostały pomyślnie odczytane
      printf("Odczytane wartości: %d, %d, %d\n", message2.data_a, message2.data_b, message2.data_c);
      
      if(message2.data_a <= stanA && message2.data_b <= stanB && message2.data_c <= stanC) {
        printf("mozna zrealizowac zamowienie\n");
        stanA -= message2.data_a;
        stanB -= message2.data_b;
        stanC -= message2.data_c;
        do_zaplaty2 = (message2.data_a*cenaA) + (message2.data_b*cenaB) + (message2.data_c*cenaC);
        write(pipe5_desc[1], &do_zaplaty2, sizeof(int));
        printf("do zaplaty: %d\n", do_zaplaty2);
        
      } else {
        do_zaplaty2 = 0;
        printf("NIE mozna zrealizowac zamowienia\n");
        write(pipe5_desc[1], &do_zaplaty2, sizeof(int));
      }
      b = 1;
}

 // Odczytaj wartości z potoku3
  if (read(pipe3_desc[0], &message2.data_a, sizeof(int)) == -1 ||
      read(pipe3_desc[0], &message2.data_b, sizeof(int)) == -1 ||
      read(pipe3_desc[0], &message2.data_c, sizeof(int)) == -1) {
      perror("Błąd odczytu z potoku");
      // Dodaj odpowiednie działania w przypadku błędu
  } else {
      // Wartości zostały pomyślnie odczytane
      printf("Magazyn: Odczytane wartości: %d, %d, %d\n", message2.data_a, message2.data_b, message2.data_c);
      
      if(message2.data_a <= stanA && message2.data_b <= stanB && message2.data_c <= stanC) {
        printf("maazyn: mozna zrealizowac zamowienie\n");
        stanA -= message2.data_a;
        stanB -= message2.data_b;
        stanC -= message2.data_c;
        do_zaplaty3 = (message2.data_a*cenaA) + (message2.data_b*cenaB) + (message2.data_c*cenaC);
        write(pipe6_desc[1], &do_zaplaty3, sizeof(int));
        printf("magazyn: do zaplaty: %d\n", do_zaplaty3);
        
      } else {
        do_zaplaty3 = 0;
        printf("magazyn: NIE mozna zrealizowac zamowienia\n");
        write(pipe6_desc[1], &do_zaplaty3, sizeof(int));
      }
  }
  c = 1;
}

//odbieamy wiadomosci zwrotne od magazynu
  if(getpid() == id_K1){
    read(pipe4_desc[0], &do_zaplaty11, sizeof(int));
    //printf("czy mozna zrealizowac zamowienie: %d\n", do_zaplaty11);
    if(do_zaplaty11 > 0){
      suma_do_zaplaty += do_zaplaty11;
    } else {
      //zabij proces
      printf("kurier1 out\n");
      close(pipe1_desc[1]);
      pracujacy_kurierzy--;
      kill(id_K1, SIGKILL);
    }
 }
 
  if(getpid() == id_K2){
    read(pipe5_desc[0], &do_zaplaty21, sizeof(int));
    //printf("czy mozna zrealizowac zamowienie: %d\n", do_zaplaty21);
    if(do_zaplaty21 > 0){
      suma_do_zaplaty += do_zaplaty21;
    } else {
      //zabij proces
      printf("kurier2 out\n");
      close(pipe2_desc[1]);
      pracujacy_kurierzy--;
      kill(id_K2, SIGKILL);
    }
 }
 
 if(getpid() == id_K3){
    read(pipe6_desc[0], &do_zaplaty31, sizeof(int));
    //printf("czy mozna zrealizowac zamowienie: %d\n", do_zaplaty31);
    if(do_zaplaty31 > 0){
      suma_do_zaplaty += do_zaplaty31;
    } else {
      //zabij proces
      printf("kurier3 out\n");
      close(pipe3_desc[1]);
      pracujacy_kurierzy--;
      kill(id_K3, SIGKILL);
    }
 }
} 
       
  printf("koniec\n");
  exit(0);
}
