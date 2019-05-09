

## Server

- Gestisce api interne
- Gestisce api esterned
- Conosce l'id di tutti i rasp che si sono connessi a lui


## Sink (Raspberry + Microbit)

- Id rete (420 hardcoded **per la demo**)
    - usato per distinguere reti diverse nella stessa area 
- Id personale

## Microbit
  
- Tutti con stesso codice e quello attaccato al Raspy si distingue per messaggi sulla seriale
- Ciascuno ha un proprio id
- la join **nella demo** non avviene perche tutti sanno gia a quale rete connettersi (420 hardcoded)


# Protocolli di Comunicazione

## Sink Server
- Rest basso livello (no json o xml o merde varie)

### Nuovo sink che si registra
1) rasp manda ID personale a cloud
2) umano crea una rete
3) cloud manda ID rete a rasp


### Rasp + Microbit => Sink
1) Microbit si connette come seriale
2) Rasp comunica id rete a microbit

I due comunicano con un protocollo molto semplice:
- microbit forwarda tutti i pacchetti ricevuti al raspy
- raspy comunica le richieste al microbit
- Microbit fa da sink e gestisce la routing table della WSN
- Rasp gestisce comunicazioni ad alto livello


### Livello MAC (ack)
<TIPO, DESTINATION, SOURCE, PAYLOAD>
TIPI:
- Data
  - ACK (PAYLOAD=hash(PAYLOAD))
  - Broadcast= <Data, 0, SOURCE, PAYLOAD>
    - in questo caso non viene atteso un ack, ne in caso di ricezione, inviato.
- TODO: Risparmio energetico

### Directed Diffusion unicast (livello rete)
<TIPO, source, forward, origin, data>
TIPI:

- DD_RT_INIT <DD_RT_INIT, source, 0, origin, brcIndex>

- DD_DATA <DATA, source, forward, origin, data>

- DD_COMMAND <DD_COMMAND, source, forward, origin, DATA=< forward_list(int n, int forward [n]), data > >
  - creato solo dal sink per inviare messaggi unicast multi hop

- DD_RT_ACK <Joined, source, forward, origin, DATA=route backward(int n, int backward [n])> 
  - usato dai sensori per comunicare il path verso di loro al sink.

- DD_JOIN se non si conosce nessuno, altrimenti al rely
  - arrivo di nuova pianta
    - messaggio broadcast di join 
    - il primo Microbit che risponde gli comunica di mandare i messaggi a lui
    - al prossimo refresh dell'albero di rete ottiene la route migliore

- DD_LEAVE: e' inviata da un nodo che fallisce l'invio di un DD_COMMAND
            inoltrata tramite il rely verso il sink, per comunicarglielo
  - contiene:
    - l'indirizzo del nodo verso cui l'invio e' fallito
    - broadcast_counter per escludere messaggi con broadcast_counter piu' vecchi