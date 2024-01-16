#include <Arduino.h>
#include "LoRaWan_APP.h"
#include <TinyGPS.h>
#include <HardwareSerial.h>

#define RXD2 45
#define TXD2 46

boolean Band_GPS = false;

// SoftwareSerial SerialGPS(RXD2, TXD2);
HardwareSerial SerialGPS ( 1 );
TinyGPS gps;

bool newData = false;
unsigned long chars;
unsigned short sentences, failed;
String StrLatitud , StrLongitud;


uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x32, 0x48 };
uint8_t appEui[] = { 0xA8, 0x40, 0x41, 0xFF, 0xFF, 0x26, 0x61, 0xE0 };
uint8_t appKey[] = { 0x87, 0xB2, 0x6B, 0x39, 0xF9, 0xC9, 0x36, 0x9E, 0x3A, 0xDB, 0xFC, 0x0A, 0x94, 0xCD, 0x12, 0xDC };

/* ABP para*/
uint8_t nwkSKey[] = { 0x14, 0xB3, 0xDD, 0xD3, 0x46, 0x89, 0xA0, 0x29, 0xA5, 0xF7, 0xA3, 0xEA, 0x06, 0x81, 0x4F, 0xCB };
uint8_t appSKey[] = { 0xF0, 0x78, 0x53, 0x79, 0xD1, 0x31, 0xA5, 0x3E, 0x32, 0x8B, 0x4B, 0x31, 0x15, 0xA8, 0xCB, 0x37 };
uint32_t devAddr =  ( uint32_t )0x260CCAC7;

//LoraWan channelsmask, default channels 0-7/ 
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };

//LoraWan region, select in arduino IDE tools/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

//LoraWan Class, Class A and Class C are supported/
DeviceClass_t  loraWanClass = CLASS_A;

//the application data transmission duty cycle.  value in [ms]./
uint32_t appTxDutyCycle = 15000;

//OTAA or ABP/
bool overTheAirActivation = true;

//ADR enable/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ 
static void prepareTxFrame( String StrLat, String StrLog);

String DataLoraSend;

void setup() {
  Serial.begin(115200);
  Mcu.begin();
  
  deviceState = DEVICE_STATE_INIT;

  SerialGPS.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Band_GPS = true;
  Serial.println("INICIANDO ------>>>>>>>>>>>>> ------->>>>>>>>>");

}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
      #if(LORAWAN_DEVEUI_AUTO)
            LoRaWAN.generateDeveuiByChipID();
      #endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {

      prepareTxFrame( StrLatitud, StrLongitud );

      LoRaWAN.send();
      //sprintf(txpacket,"Hello world number %0.2f",txNumber);
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }

  // ------------------------------------------------------------

  if (Band_GPS == true)
  {
      Band_GPS = false;

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

      deviceState = DEVICE_STATE_SEND;

  }

  // ------------------------------------------------------------
}

/* Prepares the payload of the frame */
static void prepareTxFrame( String StrLat, String StrLog)
{
    /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
      appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
      if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
      if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
      for example, if use REGION_CN470,
      the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
    */

    DataLoraSend = String(StrLat)+"@" + String(StrLog)+"@";

    //DataLoraSend = String(ValR_Hmd,2);

    Serial.print("DataLoraSend: "); Serial.println(DataLoraSend);
  
    int str_len = DataLoraSend.length() + 1;
    char char_array[str_len];
    DataLoraSend.toCharArray(char_array, str_len);

    unsigned char *puc;
    puc = (unsigned char*)(&char_array);
    appDataSize = str_len;
    
     for (int i = 0; i < str_len; i++) 
     {
        appData[i] = puc[i];
       //Serial.print(char_array[i]); Serial.print("-");
     } 
}

