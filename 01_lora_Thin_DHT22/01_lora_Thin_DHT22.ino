
#include "LoRaWan_APP.h"
#include "DHT.h"

#define DHTPIN 4    // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

boolean Band_HmdTmp = false;
float DhtValHum,DhtValTem = 0;

// Humedad y temperatura 
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x33, 0x75 };
uint8_t appEui[] = { 0xA8, 0x40, 0x41, 0xFF, 0xFF, 0x26, 0x61, 0xE1 };
uint8_t appKey[] = { 0x67, 0x6C, 0x44, 0x96, 0xC8, 0x79, 0x97, 0x88, 0xF8, 0x38, 0xDB, 0x89, 0x85, 0xE9, 0xDB, 0xAA };

/* ABP para*/
uint8_t nwkSKey[] = { 0x08, 0x1D, 0x97, 0x5A, 0x76, 0xF9, 0xFC, 0x0B, 0xCF, 0xEE, 0xE6, 0xE2, 0x2B, 0x56, 0x3D, 0x8D};
uint8_t appSKey[] = { 0xCE, 0xBE, 0xFB, 0x10, 0x68, 0x22, 0x27, 0xA0, 0x59, 0x4E, 0xED, 0xBB, 0xB6, 0x09, 0x30, 0x74};
uint32_t devAddr =  ( uint32_t )0x260CFF7B;


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
static void prepareTxFrame( float OxDs, float ValTemp );

String DataLoraSend;

void setup() {
  Serial.begin(115200);
  Mcu.begin();
  
  deviceState = DEVICE_STATE_INIT;

  dht.begin();
  Band_HmdTmp = true;
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
      prepareTxFrame( DhtValHum, DhtValTem );
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

  if (Band_HmdTmp == true)
  {
        Band_HmdTmp = false;

         // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        DhtValHum = dht.readHumidity();
        // Read temperature as Celsius (the default)
        DhtValTem = dht.readTemperature();

        if (isnan(DhtValHum) || isnan(DhtValTem)) {
           DhtValHum = 0; DhtValTem = 0;
           Serial.println(F("Error de lectura del sensor DHT22!"));
        }
        
        Serial.print("DhtValHum: ");Serial.print(DhtValHum);Serial.print("   DhtValTem: ");Serial.println(DhtValTem);

        deviceState = DEVICE_STATE_SEND;

  }


  // ------------------------------------------------------------
}

/* Prepares the payload of the frame */
static void prepareTxFrame( float ValR_Hmd, float ValR_Tmp )
{
    /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
      appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
      if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
      if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
      for example, if use REGION_CN470,
      the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
    */
  
    DataLoraSend = String(ValR_Hmd,2)+"@"+ String(ValR_Tmp,2)+"@";

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


