# PCD

Proiectul conține:
- Server UNIX, scris in C, platformă Linux, care se ocupă cu diverse operații de procesare de imagini. Fișier `server.c`
- Client de tip admin, scris in C, platformă Linux, care comunică cu serverul de pe aceeași mașină prin intermediul unui socket UNIX. Fișier `admin.c`
- Client ordinar, scris in C, platformă Linux, care comunică cu serverul prin intermediul socket-urilor INET, și care pun cereri către server pentru procesarea de imagini. Fișier `client.c`
- Client ordinar, scrins in Python, platformă Windows, care comunică cu serverul prin intermediul socket-urilor INET, și care pun cereri către server 
pentru procesarea de imagini. Fișier `client.py`

## Dependințe

Pentru procesarea de imagini serverul se folosește de aplicația `GraphicsMagick`, care trebuie să fie instalată pe mașina host a serverului.
Similar funcția de tagg-ing a pozelor, este bazată pe un script python de machine learning care are ca dependințe `python3` și biblioteca `tensorflow`

## Configurare și Compilare

Proiectul este configurat pentru a funcționa pe rețeaua `localhost` (`127.0.0.1`) port `5555`, iar clientul de python pentru windows a fost configurat pentru a accesa rețeaua localhost din interiorul unei maișini virtuale din VirtualBox, astfel se conectează la adresa `10.0.2.2:5555`.

Fișierele se generează folosind următoarele comenzi
- Pentru server `make server_unix`, care generează fișierul `server`
- Pentru clientul de tip admin, UNIX `make admin_client`, care generază fișierul `admin`
- Pentru clientul ordinar, UNIX `make ordinary_client`, care generează fișierul `client`
- Pentru clientul ordinar, Windows, nu este nevoie de compilare, este rulat direct folosind `python client.py`

## Structură Cereri Admin
Pentru operațiunile `A_GET_PENDING`și `A_GET_FINISHED`cererile vor fi de forma

|  Operațiune  |
|     :---:    |
|    2 bytes   |

Pentru aceste cereri, adminul va primi un număr de cereri cu următoarea structură, definită în `socket_utils.h`

```c
struct orders {
  unsigned long long order_number;
  uuid_t client_id;
  unsigned short operation;
  char argument[64];
  char file_path[30];
  struct orders *next;
  int cfd;
};
```

Ultima cerere va avea pe cămpul `order_number` valoarea `0` și va fi considerată terminatorul listei.

Iar pentru cererea de tip `A_CANCEL`


|  Operațiune  | Numărul de ordine al cererii |
|     :---:    |            :---:             |
|    2 bytes   |           8 bytes            |


Pentru această cerere adminul nu va primi răspuns.

## Operațiuni Admin

Adminul alege din 3 operațiuni. Acestea sunt definite în fișierul `operations.h`

- `A_GET_PENDING` sau `200`. Pentru a obține lista de cereri în așteptare de pe server.
- `A_GET_FINISHED` sau `201`. Pentru a obține lista de cereri terminate de pe server.
- `A_CANCEL` sau `202`. Pentru a anula o cerere în așteptare, a unui client.

## Structură cereri Client

Cererile trimise de client vor fi de forma, unde `ID Client` este primit de la server odată cu conectarea la acesta.

| ID Client |  Operațiune  |  Argument  |  Mărime fișier | Extensie fișie |  Fișier               |
|   :--:    |     :--:     |    :--:    |      :--:      |      :--:      |   :--:                |
|  16 bytes |    2 bytes   |  64 bytes  |     4 bytes    |     6 bytes    | `Mărime fișier` bytes |

Pentru operațiunile `FLIP` și `TAGS` nu se va trimite câmpul `Argument`.

La operațiunile de tipul `RESIZE`, `CONVERT` și `FLIP` severul va răspunde cu fișierul procesat, sub forma

|  Mărime fișier | Extensie fișie |  Fișier               |
|      :--:      |      :--:      |   :--:                |
|     4 bytes    |     6 bytes    | `Mărime fișier` bytes |

Iar pentru operațiunea `TAGS` răspunsul va fi de forma

|    Tags    |
|    :--:    |
|  128 bytes |

Unde `Tags` va fi un șir de caractere cu 3 tag-uri separate prin `;`

## Operațiuni Client

Clientul alege din 4 operațiuni. Acestea sunt definite în fișierul `operations.h`

- `RESIZE` sau `100`. Pentru a redimensiona o imagine trimisă către server. (Exemplu: din mărimea inițială în `1280x720`)
- `CONVERT` sau `101`. Pentru a schimba formatul unei imagini trimise către server. (Exemplu: din formatul `webp` în formatul `png`)
- `FLIP` sau `102`. Pentru a răsturna o imagine trimisă către server.
- `TAGS` sau `199`. Pentru a primi automat 3 cuvinte cheie care descriu conținutul imaginii.
