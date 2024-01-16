#include "LoRaWan_APP.h"
boolean Band_Diox = false;

/************************Hardware Related Macros************************************/
#define         MG_PIN                       (4)     //define which analog input channel you are going to use
#define         DC_GAIN                      (8.5)   //define the DC gain of amplifier

/***********************Software Related Macros************************************/
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interval(in milisecond) between each samples in
                                                     //normal operation

/**********************Application Related Macros**********************************/
//These two values differ from sensor to sensor. user should derermine this value.
#define         ZERO_POINT_VOLTAGE           (0.220) //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define         REACTION_VOLTGAE             (0.030) //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2

/*****************************Globals***********************************************/
float           CO2Curve[3]  =  {2.602,ZERO_POINT_VOLTAGE,(REACTION_VOLTGAE/(2.602-3))};
                                                     //two points are taken from the curve.
                                                     //with these two points, a line is formed which is
                                                     //"approximately equivalent" to the original curve.
                                                     //data format:{ x, y, slope}; point1: (lg400, 0.324), point2: (lg4000, 0.280)
                                                     //slope = ( reaction voltage ) / (log400 –log1000)

int percentage;
float volts;


uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x33, 0x77 };
uint8_t appEui[] = { 0xA8, 0x40, 0x41, 0xFF, 0xFF, 0x26, 0x61, 0xE2 };
uint8_t appKey[] = { 0xDF, 0x32, 0x55, 0x26, 0x63, 0x02, 0x5D, 0x98, 0x61, 0x30, 0xAC, 0xF5, 0x8B, 0x80, 0x02, 0xA3 };

/* ABP para*/
uint8_t nwkSKey[] = { 0xF1, 0x64, 0x52, 0x06, 0x75, 0xAB, 0xB5, 0xE7, 0xD9, 0xF9, 0x65, 0x8B, 0x1C, 0x70, 0xB5, 0x5A };
uint8_t appSKey[] = { 0xF5, 0x04, 0xCE, 0x1F, 0x49, 0xFE, 0xED, 0x30, 0x2E, 0xA1, 0x83, 0xD4, 0x91, 0x0D, 0x5A, 0x9D };
uint32_t devAddr =  ( uint32_t )0x260C042E;

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
static void prepareTxFrame( int OxDioxPpm);

String DataLoraSend;

void setup() {
  Serial.begin(115200);
  Mcu.begin();
  
  deviceState = DEVICE_STATE_INIT;

  Band_Diox = true;
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
      prepareTxFrame( percentage );
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

  if (Band_Diox == true)
  {
        Band_Diox = false;

         volts = MGRead(MG_PIN);
          Serial.print( "SEN0159: " );
          Serial.print(volts);
          Serial.print( "V           " );

          percentage = MGGetPercentage(volts,CO2Curve);
          Serial.print("CO2: ");
          if (percentage == -1) {
              Serial.print( "<400" );
          } else {
              Serial.print(percentage);
          }

          Serial.println( " ppm" );

        


        deviceState = DEVICE_STATE_SEND;

  }


  // ------------------------------------------------------------
}

/* Prepares the payload of the frame */
static void prepareTxFrame( int OxDioxPpm)
{
    /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
      appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
      if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
      if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
      for example, if use REGION_CN470,
      the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
    */
  
    DataLoraSend = String(OxDioxPpm)+"@57@";

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

/*****************************  MGRead *********************************************
Input:   mg_pin - analog channel
Output:  output of SEN-000007
Remarks: This function reads the output of SEN-000007
************************************************************************************/
float MGRead(int mg_pin)
{
    int i;
    float v=0;

    for (i=0;i<READ_SAMPLE_TIMES;i++) {
        v += analogRead(mg_pin);
        delay(READ_SAMPLE_INTERVAL);
    }
    v = (v/READ_SAMPLE_TIMES) *3.3 / 4095 ;
    return v;
}

/*****************************  MQGetPercentage **********************************
Input:   volts   - SEN-000007 output measured in volts
         pcurve  - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
         of the line could be derived if y(MG-811 output) is provided. As it is a
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
         value.
************************************************************************************/
int  MGGetPercentage(float volts, float *pcurve)
{
   if ((volts/DC_GAIN )>=ZERO_POINT_VOLTAGE) {
      return -1;
   } else {
      return pow(10, ((volts/DC_GAIN)-pcurve[1])/pcurve[2]+pcurve[0]);
   }
}


