
#include <Arduino.h>
#include <TinyGPS.h>
#include <HardwareSerial.h>

#define RXD2 45
#define TXD2 46

// SoftwareSerial SerialGPS(RXD2, TXD2);
HardwareSerial SerialGPS ( 1 );
TinyGPS gps;

bool newData = false;
unsigned long chars;
unsigned short sentences, failed;
String StrLatitud , StrLongitud;

void setup() 
{
  Serial.begin(115200);
  // -----------------------------------------------------
  // -------------------  NUCLEO ESP32  ------------------
  // SERIAL_8N1 (the default) 8-bit No parity 1 stop bit
  SerialGPS.begin(9600, SERIAL_8N1, RXD2, TXD2);
  // SerialGPS.begin(9600);

  Serial.print("Uso Libreia TinyGPS"); Serial.println(TinyGPS::library_version());
  Serial.println();
}

void loop()
{

  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (SerialGPS.available())
    {
      char c = SerialGPS.read();
      //Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }
  

  if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" LON=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    Serial.print(" SAT=");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    Serial.print(" PREC=");
    Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());

    Serial.println("");
    Serial.println("------------- DATOS DE UTILIDAD -------------");
    StrLatitud = String(flat,6);
    StrLongitud = String(flon,6);
    Serial.print(" StrLatitud: ");Serial.println(StrLatitud);
    Serial.print(" StrLongitud: ");Serial.println(StrLongitud);
    
  }
  

  gps.stats(&chars, &sentences, &failed);
  Serial.print(" CHARS=");
  Serial.print(chars);
  Serial.print(" SENTENCES=");
  Serial.print(sentences);
  Serial.print(" CSUM ERR=");
  Serial.println(failed);
  if (chars == 0)
  Serial.println("** No characters received from GPS: check wiring **");


}


