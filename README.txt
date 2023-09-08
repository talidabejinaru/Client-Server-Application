Struncturi: struct client -> avem un vector de acest tip, reprezentand fiecare
client care s a conectat la server. Acest vector contine si un vector de topicuri
la care s a abonat fiecare client.
            struct message -> contine datele necesare comunicarii dintre server si
clienti. Aceasta are de asemenea un struct udp_msg, care reprezinta mesajele venite
de la udp.
    Pentru server am multiplexat intre 4 citiri:
        -stdin: la comanda "exit" iteram prin clienti si le trimitem un mesaj 
        de inchidere, iar apoi inchidem si serverul
        -socket inactiv tcp: pe acest socket vom primi cereri de conexiune din
        partea clientilor TCP, iar daca nu este conectat in acel moment, vom
        realiza comexiunea, insa in caz contrat ii vom trimite un mesaj de 
        inchidere.
        -socket udp: pe acest socket vom primii mesajele de la clientii UDP, iar
        apoi prin interediul structurii "message" vom trimite mesajele catre
        clientii TCP care sunt conectati si sunt abonati la acel topic.
        -socketi clienti tcp: pe aceste socketuri, vom comunica cu clientii tcp.
        Practic le vom asculta cele 3 situatii din tema(exit/subscribe/unsubscribe)
        Pentru prima situatie, vom incheia conexiunea cu socketul respectivului
        clien. Pentru cea de-a doua situatie, vom cautam in lista de topicuri a 
        respectivului client, daca aceasta se abonase in prealabil. Daca nu, 
        atunci vom agauga acel topic in lista de topicuri a acelui client.
        Pentru cea de-a treia varainta vom cauta in lista de topicuri a clientului,
        iar apoi daca acel topic se regaseste in acea lista, il vom sterge..
    Pentru subscriber am multiplexat intre 2 citiri:
        -sdin: am citit doar comenzile prezentate mai sus, si am trimis un mesaj
        serverului pentru a-l instiinta cu ce doresc clientii.
        -socket tcp: pentru socketul care face conexiunea TCP cu serverul, am 
        primit mesajele pe care serverul la randul lui le-a primit de la clientii
        UDP. Acesta regaseau in campurile "addr_port" si "content.payload" din 
        structura "message", insa cele din "content.payload" nu au fost prelucrate.
        Astfel, in functie de tipul acestor mesaje, am printat la STDOUT mesajul
        corespunzator.