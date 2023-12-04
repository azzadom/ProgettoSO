# ProgettoSO

Tesina per l'esame di Sistemi Operativi 22/23 con il professor Quaglia presso l'Università di Roma Tor Vergata.

Il progetto consiste in un servizio di scambio messaggi supportato tramite un server concorrente. 

Il servizio deve accettare messaggi provenienti da client (ospitati generalmente su macchine diverse 
da quella in cui risiede il server) ed archiviarli. 
L'applicazione client deve fornire ad un utente le seguenti funzioni: 
1. Lettura tutti i messaggi spediti all'utente 
2. Spedizione di un nuovo messaggio a uno qualunque degli utenti del sistema 
3. Cancellare dei messaggi ricevuti dall'utente
   
Un messaggio contiene i campi Destinatario, Oggetto e Testo. Inoltre, il servizio potrà essere 
utilizzato solo da utenti autorizzati attraverso un meccanismo di autenticazione. 

Il sistema operativo per cui è stato scritto il servizio è il sistema LINUX. 

Per tutti i dettagli implementativi e di esecuzione, consultare il file "relazione.pdf" .
