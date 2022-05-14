# PCD
Proiectul conține:
- Server UNIX, scris in C, platformă Linux, care se ocupă cu diverse operații de procesare de imagini
- Client de tip admin, scris in C, platformă Linux, care comunică cu serverul de pe aceeași mașină prin intermediul unui socket UNIX
- Client ordinar, scris in C, platformă Linux, care comunică cu serverul prin intermediul socket-urilor INET, și care pun cereri către server pentru procesarea de imagini
- Client ordinar, scrins in Python, platformă Windows, care comunică cu serverul prin intermediul socket-urilor INET, și care pun cereri către server pentru procesarea de imagini
## Dependințe
Pentru procesarea de imagini serverul de folosește de aplicația GraphicsMagick, care trebuie să fie instalată pe mașina host.
Similar funcția de tagg-ing a pozelor, este bazată pe un script python de machine learning care are ca dependințe python3 și librăria tensorflow
