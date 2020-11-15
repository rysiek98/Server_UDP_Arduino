/*  Program "Serwer UPD wykonujący mnożenie"
    Program powstał na podstawie programu
    "Responsywny serwer UDP"
    autorstwa:  Aleksander Pruszkowski.

    Opis:
    Dziala w taki sposob, ze nasluchuje na porcie 2393 (deklaracja w makro UDP_SERVER_PORT)
    przyjmuje żądania: NIECHE jakasliczba, NIECHF jakasliczba,* - wykonanie mnozenia na
    podanych liczbach,DAJ - zwraca wynik. Notacja Polska Odwrotna. Liczby do 8 znaków,
    mogą być ujemne.

    Autor: Michał Ryszka
*/
#include <ObirDhcp.h>           //dla pobierania IP z DHCP - proforma dla ebsim'a 
#include <ObirEthernet.h>       //niezbedne dla klasy 'ObirEthernetUDP'
#include <ObirEthernetUdp.h>    //sama klasa 'ObirEthernetUDP'

#define UDP_SERVER_PORT         2393

byte MAC[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01}; //MAC adres karty sieciowej, to powinno byc unikatowe - proforma dla ebsim'a

//dlugosc pakietu z danymi dla/z UDP
#define PACKET_BUFFER_LENGTH        17

float eNumber = 0.0;
float fNumber = 0.0;
float result = 0.0;
unsigned char ready[3] = {0};

//numer portu na jakim nasluchujemy
unsigned int localPort = UDP_SERVER_PORT;

//dla podejscia z biblioteka ObirEthernetUdp, nie powinno sie kreowac
//wiecej niz jednego obiektu klasy 'ObirEthernetUDP'
ObirEthernetUDP Udp;

void setup() {
  //Zwyczajowe przywitanie z userem (niech wie ze system sie uruchomil poprawnie)
  Serial.begin(115200);
  Serial.print(F("Program starts work... [")); Serial.print(F(__FILE__));
  Serial.print(F(", ")); Serial.print(F(__DATE__)); Serial.print(F(", ")); Serial.print(F(__TIME__)); Serial.println(F("]"));

  //inicjaja karty sieciowe - proforma dla ebsim'a
  ObirEthernet.begin(MAC);

  //potwierdzenie na jakim IP dzialamy - proforma dla ebsim'a
  Serial.print(F("My IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    Serial.print(ObirEthernet.localIP()[thisByte], DEC); Serial.print(F("."));
  }
  Serial.println();

  //Uruchomienie nasluchiwania na datagaramy UDP
  Udp.begin(localPort);
}

void loop() {

  unsigned char packetBuffer[PACKET_BUFFER_LENGTH];
  //czekamy na pakiet - sprawdzajac jaka jest jego dlugosc (<=0 oznacza ze nic nie otrzymalismy)

  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    //czytamy pakiet - maksymalnie do 'PACKET_BUFFER_LENGTH' bajtow

    int len = Udp.read(packetBuffer, PACKET_BUFFER_LENGTH);
    Serial.println(len);
    //prezentujemy otrzymany pakiet (zakladajac ze zawiera znaki ASCII)
    Serial.print("Recieved: ");
    packetBuffer[len] = '\0';
    Serial.println((char*)packetBuffer);
    //przyjmowane żądania
    unsigned char NIECHE[7] = {"NIECHE "};
    unsigned char NIECHF[7] = {"NIECHF "};
    unsigned char DAJ[3] = {"DAJ"};

    unsigned char number[8] = {0};
    boolean flag = true;
    //wyłuskanie żądania NIECHE lub NIECHF
    if (len > 7) {
      for (int i = 0; i < 7; i++) {
        if (packetBuffer[i] != NIECHE[i] && (packetBuffer[i] != NIECHF[i])) {
          flag = false;
        }
      }
      //zapis liczby do zmiennej
      if (len > 8 && flag) {
        for (int i = 0; i < (len - 6 - 1); i++) {
          number[i] = packetBuffer[i + 7];
        }
        if (packetBuffer[5] == 'E') {
          eNumber = atof(number);
          Serial.println(eNumber);
          ready[0] = 'e';
        } else {
          fNumber = atof(number);
          Serial.println(fNumber);
          ready[1] = 'f';
        }
        return;
      }
    }
    //wyłuskanie żądania i wykonanie mnożenia
    if (packetBuffer[0] == '*' && len < 5) {
      if (ready[0] == 'e' && ready[1] == 'f') {
        result = eNumber * fNumber;
        Serial.print("Result: ");
        Serial.println(result);
        ready[2] = 'z';
      } else {
        char packetToSend[17] = {"Not enough data!"};
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(packetToSend, 17);
        Udp.endPacket();
      }
      return;
    } else {
      flag = false;
    }

    //wyłuskanie żądania DAJ i wysłanie wyniku
    if (len > 3 && len < 7) {
      flag = true;
      for (int i = 0; i < 3; i++) {
        if (packetBuffer[i] != DAJ[i]) {
          flag = false;
        }
      }
      if (flag) {
        if (ready[2] == 'z') {
          byte resultLength = 0;
          float resultCopy = result;
          while (abs(resultCopy) > 1) {
            resultCopy = resultCopy / 10;
            resultLength++;
          }
          if (result < 0) {
            resultLength++;
          }
          char packetToSend[resultLength];
          dtostrf(result, resultLength, 0, packetToSend);
          Serial.println(result);
          Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
          Udp.write(packetToSend, resultLength);
          Udp.endPacket();
        } else {
          char packetToSend[17] = {"Not enough data!"};
          Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
          Udp.write(packetToSend, 17);
          Udp.endPacket();
        }
      }
    }
    //odpowiedz na niepoprawne żaania
    if (!flag) {
      char packetToSend[11] = {"Bad request"};
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(packetToSend, 11);
      Udp.endPacket();
    }
  }
}

