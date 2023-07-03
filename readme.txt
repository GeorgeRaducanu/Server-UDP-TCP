Readme Tema 2 PCOM
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Copyright Raducanu George Cristian 321CAb 2022-2023
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
Tema 2 PCOM - Server & Subscriber
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
! Ca schelet de pornire pentru aceasta tema am folosit laboratorul 7 de la PCOM,
intrucat are o structura asemanatoare cu server si client !
De asemenea si DIE si anumite structuri si functii sunt din lab ul 7.
Scheletul de laborator a fost de mare ajutor intrucat am avut un punct de plecare!!!
-------------------------------------------------------------------------------------------------------

In fisierul structs.h mi-am realizat propriile structuri atat pentru mesaje de tip 
udp si tcp cat si structuri auxiliare pentru gestionarea interna a serverului.

Acele structuri sunt modalitatea de a tine cont de toate corespondentele 
id-client, topic uri (subiecte), dar si avand rolul de a stoca in cazul in care clientul 
nu este conectat mesajele, pentru a fi forwardate in cazul in care clientul se reconecteaza.

Intrucat am ales sa implementez tema in C, fiind cel mai familiar limbaj de programare, nu am 
avut la dispozitie structuri de date de tip HashMap si atunci pentru o mai mare lizibilitate a 
codului am ales sa fac vectori de structuri care sa ma ajute in gestionarea interna a programului.

Inainte de a incepe o detaliere a organizarii codului si a "flow-ului" executiei programului, voi 
mai mentiona cateva detalii legate de constrangerile impuse in tema.
Pentru conexiuni, utilizez ca in laboratorul 7 un vector poll_fd. (Folosesc poll in loc de epoll 
din cauza faptului ca imi este mai cunoscuta modalitatea aceasta)
Majoritatea vectorilor sunt alocati dinamic pentru a nu aloca prea multe date pe stiva dar de asemenea 
si pentru un cod mai eficient. Se poate aloca vectorul mai mare pentru a nu limita numarul de clienti.


---------------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------------

Descrierea modului de functionare a serverului

Serverul se porneste din linia de comanda cu parametrul 12345 (portul). Am dezactivat buffering-ul 
pt server. Am creat doi socketi initial, unul listenfd pentru ascultare si unul udp_sock de tip 
SOCK_DGRAM pentru conexiunea de tip UDP. Apoi am dezactivat algoritmul lui Nagel, asa cum este si mentionat 
in cerinta pentru a procesa pachetele fix in ordinea lor reala. De asemenea am setat structurile de sockaddr_in 
si am dat bind (identic ca in laborator). In main se apeleaza in continuare functia ce rulezaza serverul si dupa 
se inchid cei 2 socketi.

------------------------------------------------------------------------------------------------------------------
Functia ce ruleaza toata mentenanta serverului (run_chat_multi_server) (am pastrat numele functiei din laboratorul 7)

Functia aceasta este de fapt comportamentul serveului dupa pornire. Am decis sa las functia aceasta mai mare si sa nu 
o modularizez fiindca am considerat ca se vede mai bine flow-ul programului astfel.
Initial imi declar dinamic vectorii mei de structuri: poll_fds - asemanator cu cel din lab,
id_map_topics (vector in care fiecare id activ tine topicurile la care e abonat), topic_map_id -> vectorul in care 
se tin id urile abonate la topicul respectiv, tinand cont si de sf ul corespunzator, dar si o structura ajutatoare pentru
 a stoca mesajele netrimise corespunzatoare unui id.

Se introduc in vectorul de poll_fd socketii pentru listenfd (cel de ascultare), STDIN si pt UDP.

Intr-o bucla while, care se va opri doar in momentul opririi serverului se continua pasii.

Iterez prin vectorul de poll_fd si verific la fiecre pas daca pe socketul respectiv
 am primit informatii.

 Daca socketul este cel listenfd inseamna ca se doreste o noua conexiune din partea 
 unui client TCP. Se primeste in formatul cerut un pachet (de tip chat - cel din lab) 
 si se verifica existenta id-ului clientului printre clientii activi.
 Daca clientul este deja activ se specifica acest lucru si se trimite un mesaj de tip udp 
 cu conventia alesa ce are rolul de oprire pentru client.

 In caz in care nu exista un client cu acelasi ID se accepta conexiunea si se 
 introduce in vectorul de poll fd si id-uri active, avand grija ca sa trimitem eventualele 
 mesaje netrimise intre ultima conectare si prezent.

Daca socketul este cel de udp se primeste mesajul de la clientii udp.
Am grija ca in structura mea de tip udp sa completez si campurile corespunzatoare 
portului si adresei ip de unde a venit mesajul.

In continuare se trimite mesajul tuturor clientilor activi care sunt abonati cu sf0.
In cazul celor cu sf1 (cum se vede din structuri) mesajul se trimite celor activi,iar in cazul 
celor neconectati acum, dar au fost in trecut se memoreaza pachetul in structura cu id ul 
corespunzator.

In cazul in care socketul este cel coresponzator pt STDIN, se trimite prin conventia 
aleasa tot un mesaj de udp cu un tip nefolosit ce va fi recunoscut de clienti drept semnal 
de oprire. Apoi, serverul isi va inchide toate conexiunile si isi va elibera toata 
memoria alocata dinamic.

Ultimul caz este cel al primirii unui mesaj pe o conexiune de tip tcp de la unul din 
clientii mei tcp. Acesta poate fi un mesaj de exit, abonare sau dezabonare.
In cazul mesajului de exit se scoate clientul respectiv din randul celor activi.
In cazul abonarii la un topic cu un anumit tip de sf se inroduce acest 
fapt in toate structurile corespunzatoare.
In cazul dezabonarii trebuie scoasa asocierea client-subiect si viceversa din toate 
structurile interne ale serverului.


--------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------

Descrierea modului de functionare a subscriberului tcp

---------------------------------------------------------------------------------------------

Subscriberul isi ia din linia de comanda, parametrii de id, ip dar si de port.

Se deschide si pregateste socketul de comunicare. Se trimite initial pentru identificare 
asa cum am specificat si la server, folosind structura de chat message, un mesaj.
Apoi se apeleaza functia ce ruleza de fapt tot clientul meu.

Functia run_client, lucreaza asemanator cu cea de la server cu vector de poll_fd.
Iterand prin vectorul de fd se asteapta primirea unui mesaj de la server, caz in care se interpreteaza
 daca clientul este deja conectat, daca serverul s-a inchis sau daca am primit mesaj valid.
 In primele 2 cazuri, clientul is va elibera toata memoria alocata si se va inchide.
 In cazul primirii unui mesaj valid se va parsa mesajul in mod coespunzator si 
 se va afisa informatia transportata in formatul impus.


 De asemenea se citesc mesaje de la tatstatura, si anume comenzi din partea utilizatorului.
 Acestea pot fi de exit, de abonare sau de dezabonare, dar si incorecte (format incorect).
 Daca comanda este de exit, se va trimite un mesaj catre server prin care acesta va fi anuntat, 
 pentru a isi putea si el gestiona structura interna. Apoi se vor dealoca structurile folosite si 
 inchide porturile. 
 Daca comanda este de tip subscribe se parseaza corespunzator inputul avand grija sa includem si sf ul.
 Se completeaza in structura mea si se trimite mesajul.
 Cazul cu unsubscribe este asemanator dar fara sa se tina cont de sf.


 ------------------------------------------------------------------------------------------------
 ------------------------------------------------------------------------------------------------

 Surse:
    Am folosit laboratorul 7 de pcom atat ca schelet initial cat si ca model de cum trebuie 
    gandit. In main scheltul nu a suferit mari modificari, cel putin la subscriber, la server fiind 
    mai multe modificari in main. Schimbarile majore sunt in structurile folosite si in codul functiei 
    ce ruleaza efectiv serverul respectiv subscriberul.

------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------

Mentiuni:
    Intrucat nu s-a specificat in enunt, in cazul in care un client se aboneaza la un topic cu un sf 
    si apoi inca o data cu alt sf (not(sf)) codul meu trateaza ca si cum ar fi conectat la ambele.

    Am ales sa scriu in C, fiind mai familiar. Desi este mai ineficient am realizat in C, fara Map sau
    alte structuri de date mai mult pentru o lizibilitate mai mare, dar si pt ca am considerat 
    ca astfel de facilitati existau si inainte de C++ si probabil fara prea multe SD avansate.
--------------------------------------------------------------------------------------------------
