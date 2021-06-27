321CA Stanca Adelin-Nicolae

****server.cpp****
Pentru a implementa functionalitatea necesara server-ului, am inceput prin a
face in principiu operatiile din cadrul laboratoarelor, anume initializarea
socketului de TCP si a celui de UDP, precum si declararea structurilor de
date auxiliare pe care le-am folosit in rezolvarea temei. Am setat atat
partea de TCP no delay, cat si comanda de setvbuf necesare rezolvarii complete
a temei. Pentru toata partea de inceput din main, am folosit laboratoarele 6-8
ca model. In continuare, verific daca primesc date dinspre clientul UDP, TCP
sau de la tastatura. Daca primesc date de la UDP, nu ramane decat sa iau sirul
de caractere primit si sa il prelucrez. Primele 50 de caractere reprezinta
topicul, astfel ca voi pastra acesti octeti intr-o variabila, dupa care voi
memora urmatorul byte ca fiind tipul de date ce va fi alcatuit din continut,
iar apoi cel mult 1500 octeti de continut propriu zis. Folosind explicatiile
din enuntul temei, am convertit continutul brut la o valoare numerica sau la
un string, in functie de cerinta. Apoi am asamblat toate aceste bucati intr-un
sir de caractere pe care urma sa il trimit catre clientii abonati la topicul
din mesaj. Pentru a face acest lucru, am parcurs lista de clienti, am verificat
daca acestia sunt abonati la topic, dupa care, daca erau online, li se trimitea
mesajul folosind un protocol de comunicatie, iar in cazul in care nu erau on,
li se stoca intr-un dictionar pentru topicul curent un nou mesaj. Pentru a
permite o comunicatie eficienta, am ales ca protocolul meu sa insemne trimitera
inainte pe 32 de biti a dimensiunii buffer-ului ce urmeaza sa fie primit pentru
a avertiza clientul TCP de dimensiunea pe care trebuie sa incaseze octetii.
Mentionez ca am intampinat multe dificultati la aceasta parte, intrucat TCP-ul
lipea mesajele trimise unul dupa altul in cazul in care nu stia exact cati bytes
trebuie sa citeasca.
In cazul in care a primit mesaj de la socketul cu listen, va verifica daca poate
accepta aceasta noua conexiune. In primul rand verifica daca mai exista un alt
client online cu acelasi ID, iar in caz afirmativ, refuza conexiunea si inchide
clientul. Apoi, verifica daca este vorba de un client nou conectat sau clientul
se deconectase la un moment dat, iar acum revine. Daca acesta este conectat
pentru prima oara, creeaza un vector in care vor fi memorate topicurile sale,
impreuna cu continuturile in asteptare, dupa care va fi adaugat impreuna cu
acest vector intr-un dictionar ce mapeaza ID-ul clientului cu lista sa de
subscriptii. In cazul in care clientul mai fusese o data conectat, i se va
parcurge vechea lista de subscriptii si se vor afisa toate mesajele de la
topicurile cu SF = 1 care au fost trimise in timp ce el fusese deconectat, dupa
care acest vector se goleste de continut.
In cazul in care a primit mesaj de la clientul TCP, se va verifica ce tip de
mesaj i-a trimis acesta (subscribe/unsubscribe), insa mai intai se va vedea
daca sunt trimisi bytes sau nu. Daca serverul nu primeste nimic, inseamna ca
clientul va fi deconectat, deci va fi eliminat din lista de online_clients si
va fi scos din lista de fd. In cazul in care primeste un mesaj de subscribe, se
va crea o noua instanta cu topicul cerut si se va adauga in lista de topicuri
la care clientul era deja abonat. In cazul in care mesajul este de "unsubscribe",
se va cauta topicul dat in lista de topicuri a clientului, iar daca va fi gasit,
va fi eliminat. Daca nu este gasit, se printeaza la stderr un mesaj corespunzator.
Daca primesc mesaj de la tastatura, verific daca este unul de exit, moment in
care inchid serverul si automat toti clientii TCP.

****subscriber.cpp***
Initializarile de la inceputul main-ului respecta logica din laboratoarele 6-8.
Dupa declarari si initializari, verific de fiecare data daca primesc un mesaj de
la server, caz in care stiu, potrivit protocolului implementat, ca voi primi o
dimensiune pe 32 de biti, dupa care un numar deja cunoscut de bytes. Aici, doar
afisez mesajul deja corect la iesire.
Daca am primit un mesaj de la tastatura, verific daca acesta respecta structura
ceruta in enuntul temei. In cazul in care formatul este corect, afisez la iesire
mesajul adecvat, dupa care trimit mesajul catre server. In cazul in care comanda
este necunoscuta, afisez acest lucru si inchid executabilul.
