#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/msg.h>
#include <signal.h>

struct Message {
    long type;
    int data_a;
    int data_b;
    int data_c;
};


int main(int argc, char* argv[])
{ 
    int desc_pliku_konf; 
    int klucz_kolejki = atoi(argv[2]); 
    int ID_kolejki; 
    int ID_procesu_pierwotnego; 
    int stan_magazynu[6]; 
    int stanA, stanB, stanC; 
    int cenaA, cenaB, cenaC; 
    int pom, pom2, pom3; 
    int id_K1,id_K2, id_K3; 
    int pracujacy_kurier1 = 1; 
    int pracujacy_kurier2 = 1;  
    int pracujacy_kurier3 = 1;     
    int zarobione1 = 0;             
    int zarobione2 = 0;             
    int zarobione3 = 0;             
    int pipe1_desc[2], pipe2_desc[2], pipe3_desc[2], pipe4_desc[2], pipe5_desc[2], pipe6_desc[2];      
    int do_zaplaty_K1 = 0;          
    int feedback_do_zaplaty_K1 = -1; 
    int do_zaplaty_K2 = 0;  
    int feedback_do_zaplaty_K2 = -1; 
    int do_zaplaty_K3 = 0;  
    int feedback_do_zaplaty_K3 = -1; 
    int suma_zarobione_K1 = 0; 
    int suma_zarobione_K2 = 0; 
    int suma_zarobione_K3 = 0; 
    struct Message message;
    struct Message message2;
    struct Message feedback;
    feedback.type = 2;
    feedback.data_b = -1;
    feedback.data_c = -1;
    clock_t czas_start_K1, czas_stop_K1;
    clock_t czas_start_K2, czas_stop_K2;
    clock_t czas_start_K3, czas_stop_K3;
  
    ID_procesu_pierwotnego = getpid();
  
    //sprawdzamy, czy podano prawidlowa ilosc argumentow
    if(argc<3){
        fprintf(stderr, "Za mało argumentów. Użyj:\n");
        fprintf(stderr, "%s <ścieżka do pliku konfiguracyjnego> <klucz dostepu do kolejki>\n", argv[0]);
        exit(1);
    }
  
    //otwieramy plik konfiguracyjny
    desc_pliku_konf = open(argv[1], O_RDONLY);
    if(desc_pliku_konf == -1){
        perror("Blad otwarcia pliku konfiguracyjnego");
        exit(1);
    }

    //wczytujemy dane z pliku
    char buffer[1];                                                                                 // Wczytujemy po jednym znaku
    int bytesRead, index = 0;
    int value = 0;
  
    while((bytesRead = read(desc_pliku_konf, buffer, sizeof(buffer))) > 0) {
        if(buffer[0] == ' ' || buffer[0] == '\n' || buffer[0] == '\t') {                            // Jeśli napotkaliśmy spację, nową linię lub tabulator, zakończ parsowanie liczby
            stan_magazynu[index++] = value;
            value = 0;                                                                              // Resetujemy wartość dla kolejnej liczby
            } else {
            value = value * 10 + (buffer[0] - '0');                                                 // Parsowanie cyfry
            }
        }


    if (value != 0) { stan_magazynu[index++] = value; }                                             // Sprawdzamy, czy ostatnia liczba została wczytana

    stanA = stan_magazynu[0];                                                                       //przypisujemy odczytane z pliku wartosci do zmiennych
    cenaA = stan_magazynu[1];
    stanB = stan_magazynu[2];
    cenaB = stan_magazynu[3];
    stanC = stan_magazynu[4];
    cenaC = stan_magazynu[5];

    close(desc_pliku_konf);                                                                         //zamykamy deskryptor pliku
  

    //tworzymy potoki, ktorymi beda komunikowali sie kurierzy z magazynem
    if(pipe(pipe1_desc) == -1 || pipe(pipe2_desc) == -1 || 
        pipe(pipe3_desc) == -1 || pipe(pipe4_desc) == -1 || 
        pipe(pipe5_desc) == -1 || pipe(pipe6_desc) == -1)
    { 
        perror("Tworzenie potoku"); 
        exit(1);
    }
  
  
    //tworzymy trzech kurierow
    pom = fork();               
        
    if(pom > 0)
    {
        pom2 = fork();
        
        if(pom2 == 0){ id_K2 = getpid(); } 
        else
        {
            pom3 = fork(); 
            if(pom3 == 0) { id_K3 = getpid(); }  
        }
    } else { id_K1 = getpid(); }
  
    //printf("ID: %d, parent: %d\n", getpid(), getppid());                                          //od tego mies=jsca wykonuja sie 4 procesy
    
    //otwieranie kolejki komunikatow przez kurierow
    if(getpid() != ID_procesu_pierwotnego)
    {
        ID_kolejki = msgget(klucz_kolejki, 0666);            
        
        if(ID_kolejki == -1) 
        {
            perror("Błąd otwarcia kolejki komunikatów");
            exit(1);
        }
    }

    //zamykanie niepotrzebnych deksryptorów
    if(getpid() == id_K1){  
        close(pipe1_desc[0]);
        close(pipe4_desc[1]);
        close(pipe2_desc[0]);
        close(pipe2_desc[1]);
        close(pipe3_desc[0]);
        close(pipe3_desc[1]);
        close(pipe5_desc[0]);
        close(pipe5_desc[1]);
        close(pipe6_desc[0]);
        close(pipe6_desc[1]);
    } else if(getpid() == id_K2){  
        close(pipe2_desc[0]);
        close(pipe5_desc[1]);
        close(pipe1_desc[0]);
        close(pipe1_desc[1]);
        close(pipe3_desc[0]);
        close(pipe3_desc[1]);
        close(pipe4_desc[0]);
        close(pipe4_desc[1]);
        close(pipe6_desc[0]);
        close(pipe6_desc[1]);
    } else if(getpid() == id_K3){  
        close(pipe3_desc[0]);
        close(pipe6_desc[1]);
        close(pipe1_desc[0]);
        close(pipe1_desc[1]);
        close(pipe2_desc[0]);
        close(pipe2_desc[1]);
        close(pipe4_desc[0]);
        close(pipe4_desc[1]);
        close(pipe5_desc[0]);
        close(pipe5_desc[1]);
    } else {
        close(pipe1_desc[1]);
        close(pipe4_desc[0]);
        close(pipe2_desc[1]);
        close(pipe5_desc[0]);
        close(pipe3_desc[1]);
        close(pipe6_desc[0]);
    }

    czas_start_K1 = clock();
    czas_start_K2 = clock();
    czas_start_K3 = clock();

    
        //ODBIERANIE WIADOMOŚCI Z KOLEJKI KOMUNIKATÓW OD DYSPOZYTORA I PRZESYŁANIE ICH POTOKIEM DO MAGAZYNU
        if(getpid() != ID_procesu_pierwotnego)
        {
        while(1)                                                                                                    //pętla dla kurierów
        {   
            if(getpid() == id_K1)    //kurier1
            {
                //sprawdzanie, ile upłynelo od odebrania poprzedniego zamowienia
                czas_stop_K1 = clock();
                //printf("czas trwania: %.2f", ((double)(czas_stop_K1 - czas_start_K1)/CLOCKS_PER_SEC));
                if(((double)(czas_stop_K1 - czas_start_K1)/CLOCKS_PER_SEC)>150)
                {
                    printf("kurier1 out --> przekroczono limit czasu\n");
                    //printf("zarobione przez K1: %d\n", suma_zarobione_K1);
                    feedback.data_a = 0;
                    msgsnd(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long),IPC_NOWAIT); 
                    message.data_a = 999999;
                    write(pipe1_desc[1], &message.data_a, sizeof(int));                                             //napisz do magazynu, ze kurier zakonczyl prace
                    return 1;
                }

                if(msgrcv(ID_kolejki, &message, sizeof(message)-sizeof(long), 1, IPC_NOWAIT)==-1)                        //kurier odbiera zamowienia z glownego kanalu
                {
                    //perror("Bląd odbierania zamowienia z dyspozytorni");
                    continue;
                }
                else 
                {   //zegar dla K1 stop
                    czas_start_K1 = clock();
                    //zegar dla K1 start
                    printf("zamowienie odebral kurier 1 magazunu 3: A: %d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
                    //kurier wysyła do magazynu zapytanie o możliwość realizacji zamówienia
                    if (write(pipe1_desc[1], &message.data_a, sizeof(int)) == -1 ||
                        write(pipe1_desc[1], &message.data_b, sizeof(int)) == -1 ||
                        write(pipe1_desc[1], &message.data_c, sizeof(int)) == -1) 
                    {
                        perror("Błąd podczas zapisywania do potoku");
                        return 1;
                    } 
                    else
                    {
                        //kurier prawidłowo przekazal zamowienie do magazynu
                        //printf("kurier 1 wyslał zapytanie do magazynu 1: A:%d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
                    }
                }  
            } 
            else if(getpid() == id_K2)      //kurier2
            {
                //sprawdzanie, ile upłynelo od odebrania poprzedniego zamowienia
                czas_stop_K2 = clock();
                if(((double)(czas_stop_K2 - czas_start_K2)/CLOCKS_PER_SEC)>150)
                {
                    printf("kurier2 out --> przekroczono limit czasu\n");
                    //printf("zarobione przez K2: %d\n", suma_zarobione_K2);
                    //pracujacy_kurier2 = 0;
                    feedback.data_a = 0;
                    msgsnd(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long),IPC_NOWAIT); ///tu
                    message.data_a = 999999;
                    write(pipe2_desc[1], &message.data_a, sizeof(int));
                    return 1;

                }

                if(msgrcv(ID_kolejki, &message, sizeof(message)-sizeof(long), 1, IPC_NOWAIT)==-1)
                {
                    //printf("Bląd odbierania zamowienia z dyspozytorni");
                    //printf("brak zamowien\n");
                    continue;
                }
                else 
                {
                    czas_start_K2 = clock();
                    printf("zamowienie odebral kurier 2 magazunu 3: A: %d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
                    //kurier wysyła do magazynu zapytanie o możliwość realizacji zamówienia
                    if (write(pipe2_desc[1], &message.data_a, sizeof(int)) == -1 ||
                        write(pipe2_desc[1], &message.data_b, sizeof(int)) == -1 ||
                        write(pipe2_desc[1], &message.data_c, sizeof(int)) == -1) 
                    {
                        perror("Błąd podczas zapisywania do potoku");
                        return 1;
                    } 
                    else
                    {
                        //kurier prawidłowo przekazal zamowienie do magazynu
                        //printf("kurier 2 wyslał zapytanie do magazynu 1: A:%d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
                    }
                }  
    
            } 
            else if(getpid() == id_K3)      //kurier3
            {
                czas_stop_K3 = clock();
                if(((double)(czas_stop_K3 - czas_start_K3)/CLOCKS_PER_SEC)>150)
                {
                    printf("kurier3 out --> przekroczono limit czasu\n");
                    //printf("zarobione przez K3: %d\n", suma_zarobione_K3);
                    //pracujacy_kurier3 = 0;
                    feedback.data_a = 0;
                    msgsnd(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long),IPC_NOWAIT); ///tu
                    message.data_a = 999999;
                    write(pipe3_desc[1], &message.data_a, sizeof(int));
                    return 1;

                }

                if(msgrcv(ID_kolejki, &message, sizeof(message)-sizeof(long), 1, IPC_NOWAIT)==-1)                                    //kurier odbiera zamowienia z glownego kanalu
                {
                    //printf("Bląd odbierania zamowienia z dyspozytorni");
                    continue;
                }     
                else 
                {
                    czas_start_K3 = clock();
                    printf("zamowienie odebral kurier 3 magazunu 3: A: %d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
                   //kurier wysyła do magazynu zapytanie o możliwość realizacji zamówienia
                    if (write(pipe3_desc[1], &message.data_a, sizeof(int)) == -1 ||
                        write(pipe3_desc[1], &message.data_b, sizeof(int)) == -1 ||
                        write(pipe3_desc[1], &message.data_c, sizeof(int)) == -1) 
                    {
                        perror("Błąd podczas zapisywania do potoku");
                        return 1;
                    } 
                    else
                    {
                        //kurier prawidłowo przekazal zamowienie do magazynu
                        //printf("kurier 3 wyslał zapytanie do magazynu 1: A:%d, B: %d, C: %d\n", message.data_a, message.data_b, message.data_c);
                    }
                }  
            } 
    
            //ODBIERANIE WIADOMOŚCI ZWROTNEJ Z POTOKU OD MAGAZYNU (czy możliwa realizacja zamówienia)
            if(getpid() == id_K1)
            {
                if(read(pipe4_desc[0], &feedback_do_zaplaty_K1, sizeof(int))== -1)
                {
                    perror("Błąd odczytu wiadomości zwrotnej od magazynu, czy udało się zrealizować zamówienie");
                } 
                else
                {
                    //printf("mozna zrealizowac zamowienie: %d\n", feedback_do_zaplaty_K1);
                    feedback.data_a = feedback_do_zaplaty_K1;

                    msgsnd(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long),0);                     //wyślij wiadomość zwrotną (kurier --> dyspozytornia)
                    //printf("Wysłano feedback od K1: %d\n", feedback.data_a);
                    
                    if(feedback_do_zaplaty_K1 > 0)                                                      //jeśli feedback jest większy od 0 to znaczy że zamówienie zostało zrealizowane w magazynie
                    {
                        suma_zarobione_K1 += feedback_do_zaplaty_K1;
                    } 
                    else                                                                                //wartość zamówienia = 0 --> zamówienie niemożliwe do realizacji
                    {
                        printf("kurier1 out\n");
                        printf("zarobione przez K1: %d\n", suma_zarobione_K1);
                        //close(pipe1_desc[1]);
                        //close(pipe4_desc[0]);
                        //exit(0);   
                        return 1;                                                                     //zakończ proces
                    }
                }
            }
 
            else if(getpid() == id_K2)
            {
                if(read(pipe5_desc[0], &feedback_do_zaplaty_K2, sizeof(int))== -1)
                {
                    perror("Błąd odczytu wiadomości zwrotnej od magazynu, czy udało się zrealizować zamówienie");
                } 
                else 
                {
                    //printf("mozna zrealizowac zamowienie: %d\n", feedback_do_zaplaty_K2);
                    feedback.data_a = feedback_do_zaplaty_K2;
                    msgsnd(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long),0);                     //wyślij wiadomość zwrotną (kurier --> dyspozytornia)
                    //printf("wysłano feedback 2 %d\n", feedback.data_a);
                    if(feedback_do_zaplaty_K2 > 0)
                    {
                        suma_zarobione_K2 += feedback_do_zaplaty_K2;
                    } 
                    else 
                    {
                        printf("kurier2 out\n");
                        printf("zarobione przez K2: %d\n", suma_zarobione_K2);
                        //close(pipe2_desc[1]);
                        //close(pipe5_desc[0]);
                        //exit(0);
                        return 1;
                    }
                }
            }
 
            else if(getpid() == id_K3)
            {
                if(read(pipe6_desc[0], &feedback_do_zaplaty_K3, sizeof(int))== -1)
                {
                    perror("Błąd odczytu wiadomości zwrotnej od magazynu, czy udało się zrealizować zamówienie");
                } 
                else 
                {
                    //printf("mozna zrealizowac zamowienie: %d\n", do_zaplaty31);
                    feedback.data_a = feedback_do_zaplaty_K3;
                    msgsnd(ID_kolejki, &feedback, sizeof(feedback)-sizeof(long),0);
                    //printf("wysłano feedback 3 %d\n", feedback.data_a);
                    if(feedback_do_zaplaty_K3 > 0)
                    {
                        suma_zarobione_K3 += feedback_do_zaplaty_K3;
                    } 
                    else 
                    {   
                        printf("kurier3 out\n");
                        printf("zarobione przez K3: %d\n", suma_zarobione_K3);
                        //close(pipe3_desc[1]);
                        //close(pipe6_desc[0]);
                        //exit(0);
                        return 1;
                    }
                }  
            }
        }

        }
        else if(getpid()==ID_procesu_pierwotnego)                                       //kod dla procesu macierzystego
        { 
            while(1)
            {   
                if(pracujacy_kurier1 == 0 && pracujacy_kurier2 == 0 && pracujacy_kurier3 == 0)
                {
                    break;
                    printf("-----------------------MAGAZYN3------------------------\n");
                    printf("zarobione GLD: %d\n", zarobione1 + zarobione2 + zarobione3);
                    printf("stan magazynu:\n");
                    printf("A: %d\n", stanA);
                    printf("B: %d\n", stanB);
                    printf("C: %d\n", stanC);
                    printf("koniec\n");
                    exit(0);
                }
  
                if(pracujacy_kurier1 == 1)
                {
                    if(read(pipe1_desc[0], &message2.data_a, sizeof(int)) == -1 ||                                     //magazyn odczytuje wartość zamowienia od kuriera1 z potoku1 
                        read(pipe1_desc[0], &message2.data_b, sizeof(int)) == -1 ||
                        read(pipe1_desc[0], &message2.data_c, sizeof(int)) == -1) 
                    {
                        perror("Błąd odczytu z potoku");
                    } 
                    else                                                                                               // Wartości zostały pomyślnie odczytane
                    {   if(message2.data_a == 999999) 
                        { 
                            pracujacy_kurier1 =0;
                        } 
                        //printf("magazyn mowi: Odczytane wartości: %d, %d, %d\n", message2.data_a, message2.data_b, message2.data_c);
                        else if(message2.data_a <= stanA && message2.data_b <= stanB && message2.data_c <= stanC)            //sprawdzamy, czy magazyn moze zrealizowac zamowienie
                        {
                            stanA -= message2.data_a;
                            stanB -= message2.data_b;
                            stanC -= message2.data_c;
                            do_zaplaty_K1 = (message2.data_a*cenaA) + (message2.data_b*cenaB) + (message2.data_c*cenaC);
                            if(write(pipe4_desc[1], &do_zaplaty_K1, sizeof(int))==-1)
                            {
                                perror("Błąd");
                            }
                            else 
                            {
                                //printf("magazyn: do zaplaty: %d\n", do_zaplaty_K1);
                                zarobione1 += do_zaplaty_K1;
                            }
                        } 
                        else 
                        {
                            do_zaplaty_K1 = 0;
                            printf("---Magazyn3 do kuriera1: NIE mozna zrealizowac zamowienia\n");
                            if(write(pipe4_desc[1], &do_zaplaty_K1, sizeof(int))==-1){
                                perror("Błąd zapisu do potoku\n");
                            }
                            pracujacy_kurier1 = 0;
                        }
                    }   
                } 

                if(pracujacy_kurier2 == 1)
                {
                    if (read(pipe2_desc[0], &message2.data_a, sizeof(int)) == -1 ||
                        read(pipe2_desc[0], &message2.data_b, sizeof(int)) == -1 ||
                        read(pipe2_desc[0], &message2.data_c, sizeof(int)) == -1) 
                    {
                    perror("Błąd odczytu z potoku");
                    } 
                    else 
                    {
                        if(message2.data_a == 999999) 
                        { 
                            pracujacy_kurier2 =0;
                        } 
                        //printf("Odczytane wartości: %d, %d, %d\n", message2.data_a, message2.data_b, message2.data_c);
                        else if(message2.data_a <= stanA && message2.data_b <= stanB && message2.data_c <= stanC) 
                        {
                            stanA -= message2.data_a;
                            stanB -= message2.data_b;
                            stanC -= message2.data_c;
                            do_zaplaty_K2 = (message2.data_a*cenaA) + (message2.data_b*cenaB) + (message2.data_c*cenaC);
                            if(write(pipe5_desc[1], &do_zaplaty_K2, sizeof(int))== -1)
                            {
                                perror("Błąd");
                            }
                            else 
                            {
                                //printf("magazyn: do zaplaty: %d\n", do_zaplaty_K2);
                                zarobione2 += do_zaplaty_K2;
                            }
                        } 
                        else 
                        {
                            do_zaplaty_K2 = 0;
                            printf("---Magazyn3 do kuriera2: NIE mozna zrealizowac zamowienia\n");
                            if(write(pipe5_desc[1], &do_zaplaty_K2, sizeof(int))==-1)
                            {
                                perror("Błąd zapisu do  potoku\n");
                            }
                            pracujacy_kurier2 = 0;
                        }
                    }
                }
 
                if(pracujacy_kurier3==1)
                {
                    if (read(pipe3_desc[0], &message2.data_a, sizeof(int)) == -1 ||
                        read(pipe3_desc[0], &message2.data_b, sizeof(int)) == -1 ||
                        read(pipe3_desc[0], &message2.data_c, sizeof(int)) == -1) 
                    {
                        perror("Błąd odczytu z potoku");
                    } 
                    else 
                    {
                        if(message2.data_a == 999999) 
                        { 
                            pracujacy_kurier3 = 0;
                        } 
                        //printf("Odczytane wartości: %d, %d, %d\n", message2.data_a, message2.data_b, message2.data_c);
                        else if(message2.data_a <= stanA && message2.data_b <= stanB && message2.data_c <= stanC) 
                        {
                            stanA -= message2.data_a;
                            stanB -= message2.data_b;
                            stanC -= message2.data_c;
                            do_zaplaty_K3 = (message2.data_a*cenaA) + (message2.data_b*cenaB) + (message2.data_c*cenaC);
                            if(write(pipe6_desc[1], &do_zaplaty_K3, sizeof(int))== -1)
                            {
                                perror("Błąd");
                            }
                            else 
                            {
                                //printf("magazyn: do zaplaty: %d\n", do_zaplaty_K3);
                                zarobione3 += do_zaplaty_K3; 
                            }
                                       
                        } 
                        else 
                        {
                            do_zaplaty_K3 = 0;
                            printf("---Magazyn3 do kuriera3: NIE mozna zrealizowac zamowienia\n");
                            if(write(pipe6_desc[1], &do_zaplaty_K3, sizeof(int))==-1)
                            {
                                perror("Błąd zapisu do potoku\n");
                            }
                            pracujacy_kurier3 = 0;
                        }
                    }
                }
            }
        }        


    wait(NULL);
    wait(NULL);
    wait(NULL);
    printf("-----------------------MAGAZYN3------------------------\n");
    printf("zarobione GLD: %d\n", zarobione1 + zarobione2 + zarobione3);
    printf("stan magazynu:\n");
    printf("A: %d\n", stanA);
    printf("B: %d\n", stanB);
    printf("C: %d\n", stanC);
    printf("koniec\n");
    exit(0);
}