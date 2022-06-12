Odtwarzacz radia internetowego w oparciu o platformę ESP-32

Michał Faciszewski
Bartosz Szar

02.01.2022 r.
1. # Cel projektu
Głównym celem projektu jest utworzenie systemu wbudowanego oferującego możliwość odtwarzania radia internetowego przez połączenie z siecią WIFI. Gotowe urządzenie powinno umożliwić połączenie się z wybranym punktem WIFI oraz przechwycenie i odtworzenie udostępnianego przez stacje radiowe internetowego streamu audio. Dodatkowo, system ten wyposażony powinien być w wyświetlacz umożliwiający odczytywanie komunikatów oraz aktualnie odtwarzanej stacji radiowej, manipulator pozwalający poruszać się w prostym menu, głośnik pozwalający odtwarzać dźwięk audio oraz opcjonalnie zestaw bateryjny przekształcający urządzenie w urządzenie przenośne. 
1. # Wykorzystywany sprzęt
W wykonywanym przez nas projekcie wykorzystamy następujące elementy:

- płytkę ESP-32 wyposażoną w dwurdzeniowy procesor,
- wzmacniacz audio,
- głośnik o stosunkowo niskiej mocy wyjściowej,
- wyświetlacz OLED o rozdzielczości 0.91 cala,
- płytkę stykową do prototypowania oraz zestaw okablowania

Oraz opcjonalnie:

- ogniwa Li-Ion o napięciu 5v
1. # Opis projektu
   1. ## Schemat budowy
Bazowym elementem tworzonego systemu będzie oczywiście płytka ESP-32. Połączona będzie ona bezpośrednio z wzmacniaczem sygnału audio oraz z wykorzystaniem wbudowanej magistrali I2C połączona z monochromatycznym wyświetlaczem. Kolejno, wzmacniacz będzie podłączony już bezpośrednio do głośnika. Cały zestaw, zarówno wzmacniacz, jak i sama płytka ESP-32 zasilana będzie przez 5V połączenie USB. Wyświetlacz OLED w pełni będzie czerpał zasilanie z dostępnego na płytce zasilania 3.3V. 

Dodatkowo, możliwe będzie zaprojektowanie wbudowanego źródła zasilania w postaci ogniwa Li-Ion, z możliwością powtórnego ładowania. 

Cały system zamknięty będzie w specjalnie zaprojektowanej obudowie z tworzywa sztucznego, bądź cienkiego drewna.
1. ## Dekodowanie strumienia audio
W trakcie tworzenia projektu będziemy dążyć do wykorzystania biblioteki [mp3_shine_esp32](https://github.com/fknrdcls/mp3_shine_esp32), będącej portem rozwiązania z 2006 roku. Zgodnie ze znalezionymi na forach związanych z wykorzystywaniem urządzeń ESP-32, pozwala ona na software’owe dekodowanie strumienia mp3 na sygnał analogowy przy jednoczesnym obciążeniu jednego rdzenia procesora w ok. 40%, co pozostawiałoby wystarczający zapas mocy na jednoczesne pobieranie strumienia audio z Internetu.

W razie niepowodzenia bądź niewystarczającej wydajności powyższego rozwiązania, zastosujemy hardware’owy dekoder mp3, który w pełni przejmie problem przekształcania sygnału audio. 
1. ## Konfiguracja i uruchomienie urządzenia
Planujemy wykorzystanie jednego z dwóch rozwiązań lub w razie możliwości jednoczesne ich wykorzystanie:

- Podejście oparte o stronę internetową – urządzenie będzie posiadało dwa stany: stan konfiguracji i stan odtwarzania. W przypadku stanu konfiguracyjnego urządzenie będzie występowało w roli access pointa, które będzie udostępniało otwartą sieć WIFI, w której dostępna będzie strona internetowa umożliwiająca wprowadzenie danych konfiguracyjnych tj. adres http odtwarzacza, nazwa stacji, SSID i hasło WIFI. Następnie przy zatwierdzeniu ustawień urządzanie przechodziłoby do stanu odtwarzania, łączyło się z siecią WIFI oraz odtwarzaczem internetowym, a następnie rozpoczynało nadawanie sygnału audio. W każdym momencie istniałaby możliwość resetu urządzenia, przy pomocy dedykowanego przycisku. Dodatkowo na ekranie wyświetlane byłyby bieżące komunikaty o stanie urządzenia.
- Drugie podejście opierałoby się o konfigurację przez wyświetlane na wbudowanym wyświetlaczu proste menu konfiguracyjne, umożliwiające wpisanie SSID i hasła WIFI oraz adresu odtwarzacza internetowego przy pomocy dołączonych przycisków/manipulatora. W tym podejściu urządzenie permanentnie znajdowałoby się w stanie odtwarzania wyróżnionym w poprzednim punkcie.
3. # Przydatne linki i inne podobne rozwiązania
- [Biblioteka mp3_shine_esp32](https://github.com/fknrdcls/mp3_shine_esp32)
- [ESP32_MP3_WebRadio by MrBuddyCasino](https://github.com/MrBuddyCasino/ESP32_MP3_Decoder)
- [Port of ESP8266_MP3_DECODER](https://github.com/pewit-tech/esp32-mp3-decoder)
- [Espressif mp3 decoder](https://espressif-docs.readthedocs-hosted.com/projects/esp-adf/en/latest/api-reference/codecs/mp3_decoder.html)
- [Radio internetowe oparte o ESP-32 i zewnetrzny dekoder mp3](https://www.elektroda.pl/rtvforum/topic3542547.html)
3. # Wykonanie projektu
Ostatecznie zdecydowaliśmy się na wykorzystanie podejścia opartego o sprzętowy dekoder mp3, a dokładniej płytkę MAX98357A w konfiguracji mono, wykorzystującej połowę mocy kanału prawego i lewego. Z płytką ESP32 komunikuje się poprzez protokół I2S

Dodatkowo w celu ułatwienia korzystania z urządzenia, a także adaptacji projektu w innych zastosowaniach zaimplementowaliśmy obsługę dwóch rodzajów wyświetlaczy LCD 2x16 połączonego szeregowo oraz ekranu OLED 128x32px (sterownik SSD1306) obsługiwanego przez magistralę I2C.

***SCHEMAT PODŁĄCZENIA***

![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.001.png)
3. # Zaimplementowane funkcjonalności
   1. ## Radio internetowe 
Najistotniejsza funkcjonalność stworzonego przez nas systemu wbudowanego jest w pełni oparta o dostępną dla ESP32 w środowisku ArduinoIDE bibliotekę *Audio.h*. Umożliwia ona wskazanie źródła strumieniowania audio i w pełni automatycznie buforuje i przekazuje na wskazane piny sygnał w standardzie I2S, który jest później konwertowany do sygnału analogowego audio poprzez dekoder MAX98357A. Dodatkowo, w celu uzyskania połączenia z siecią WI-FI wykorzystujemy bibliotekę *WiFi.h.*
1. ## Wyświetlanie informacji pomocniczych
Zaraz po uruchomieniu urządzenia na ekranie dołączonym do obwodu wyświetlane są informacje pomocnicze, które pozwalają użytkownikowi na bezproblemową konfigurację. Komunikaty dotyczą zarówno wykonywanej aktualnie przez mikrokontroler konfiguracji, jak również stanowią bezpośrednią instrukcję dla użytkownika, wskazując np. adres IP pod którym istnieje możliwość konfiguracji radia w trybie AccessPointa.
1. ## Sterowanie głośnością przełączanie stacji
Sterowanie głośnością i przełączanie stacji zostało zrealizowane w oparciu o proste menu wyświetlane na ekranie LCD 2x16, oraz użycie 4 przycisków typu Tact Switch. Dwa przyciski służą do przełączania widoku między dwoma ekranami - ekranem wyboru stacji i ekranem ustawienia głośności. Pozostałe 2 guziki, w zależności od aktualnie wybranego widoku menu, służą do zmiany poziomu głośności, lub zmiany wyboru stacji z której aktualnie pobierany jest strumień audio. 
1. ## Konfiguracja za pośrednictwem Wi-Fi
Stworzony przez nas system wbudowany posiada możliwość konfiguracji tj. ustawienia stacji radiowej bądź zestawienia nowego połączenia Wi-Fi za pośrednictwem połączenia bezprzewodowego. Wszystko opiera się o możliwość uruchomienia modułu Wi-Fi płytki ESP32 w trybie AccessPoint. Pozwala to połączyć się bezpośrednio z płytką i za udostępnianych przez nią stron internetowych ustawić, bądź wybrać najważniejsze parametry uruchomieniowe.

Do wygodnego zarządzania serwerem internetowym i jego endpointami wykorzystana została biblioteka *ESPAsyncWebServer.h*. Pozwala ona praktycznie za pomocą jednej komendy zdefiniować endpoint i zapewnić jego asynchroniczną obsługę.
1. ## Zapisywanie konfiguracji
ESP32 udostępnia użytkownikowi 255 bajtów pamięci EEPROM, która umożliwia nieulotny zapis danych. Do jej wykorzystania można posłużyć się specjalną biblioteką proponowaną przez twórców, jednakże wymaga ona zapisu bajt po bajcie, co jest bardzo niewygodne w przypadku przechowywania łańcuchów znaków. 

Z tego powodu zdecydowaliśmy się na wykorzystanie bardziej rozbudowanej biblioteki *Preference.h*, która umożliwia zapis różnych typów danych w postaci danych słownikowych. Jest to znacznie wygodniejsze rozwiązanie, jednakże wprowadza niezerowy narzut na przechowywane dane. Z tego względu często zdarza się, sytuacja, że część danych zostaje ucięta.
6. # Ogólne działanie systemu
W pierwszej kolejności, zaraz po włączeniu zasilania bądź restarcie urządzenia, oprogramowanie próbuje odzyskać zapisane w pamięci nieulotnej dane. Następnie skanuje dostępne sieci Wi-Fi i zapisuje w zmiennej tymczasowej w celu wyświetlenia ich w oknie konfiguracyjnym. 

Następnie, radio przechodzi w tryb AccessPoint i nasłuchuje na nadchodzące połączenia i serwuje strony konfiguracyjne. Użytkownik w tym czasie może dokonać ustawienia parametrów zgodnie z jego preferencjami, a następnie zatwierdzić je odpowiednim przyciskiem, co spowoduje przejście do kolejnego kroku.

Teraz urządzenie będzie próbowało dokonać połączenia ze wskazaną siecią i o jego statusie na bieżąco będzie informowało, zarówno o powodzeniu, jak i niepowodzeniu.

W kolejnym etapie po udanym połączeniu z siecią, urządzenie rozpoczyna streaming audio z podanego w trakcie konfiguracji źródła oraz wyświetli nazwę obecnie używanej radiostacji.

W trakcie działania na potrzeby użytkownika oddana jest możliwość przełączania radiostacji, zmiany głośności oraz restartu urządzenia w celu wprowadzenia zmian w konfiguracji.



6. # Instrukcja obsługi
1. Po uruchomieniu urządzenia należy podłączyć się do sieci WiFi udostępnianej przez mikrokontroler:

![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.002.png)

1. Następnie, w celu konfiguracji urządzenia w trybie AccessPointa, należy udać się pod wyświetlany na ekranie adres IP:

![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.003.png)

1. Naszym oczom ukazuje się strona konfiguracji, umożliwiająca między innymi: dodanie nowej stacji, wybór ulubionej stacji radiowej, jak również konfiguracja sieci WiFi.  

![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.004.png)

1. Widok formularza dodawania nowej stacji:![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.005.png)

1. Widok formularza wyboru ulubionej stacji:
1. Widok formularza konfiguracji sieci Wi-Fi:![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.006.png)![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.007.png)

1. Po zatwierdzeniu konfiguracji, urządzenie przerywa działanie w trybie AccessPointa i rozpoczyna pracę w trybie radia internetowego - przyciskami ↑ oraz ↓ przełączamy się pomiędzy ekranami w menu, natomiast przyciskami ← oraz → zmieniamy ustawienie na danym ekranie (zmiana stacji, lub regulacja poziomu głośności)




1. Widok menu wyświetlającego aktualnie odtwarzaną stację.

![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.003.png)

1. Widok menu wyświetlającego aktualny poziom głośności.

![](Aspose.Words.b25d37fe-05f3-400c-ba85-a1c5a7f26502.003.png)
6. # Budowa i wgranie projektu na ESP32
W celu wgrania projektu na urządzenie ESP32 zalecamy użycie ArduinoIDE, wraz z opisanymi w opracowaniu bibliotekami. 

Sam kod projektu można pobrać tutaj: [dodać link](https://google.com)
