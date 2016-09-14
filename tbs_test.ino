#include <HardwareSerial.h>


#define SA_GET_SETTINGS 0x01
#define SA_GET_SETTINGS_V2 0x09
#define SA_SET_POWER 0x02
#define SA_SET_CHANNEL 0x03
#define SA_SET_FREQUENCY 0x04
#define SA_SET_MODE 0x05


#define SA_POWER_25MW 0
#define SA_POWER_200MW 1
#define SA_POWER_500MW 2
#define SA_POWER_800MW 3

enum SMARTAUDIO_VERSION {
  NONE,
  SA_V1,
  SA_V2
};

static const uint8_t V1_power_lookup[] = {
  7,
  16,
  25,
  40
};

typedef struct {
  SMARTAUDIO_VERSION vtx_version;
  uint8_t channel;
  uint8_t powerLevel;
  uint8_t mode;
  uint16_t frequency;
  
}UNIFY;

static UNIFY unify;

unsigned char crc8tab[256] = {
  0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54,
  0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
  0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06,
  0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
  0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0,
  0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
  0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2,
  0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
  0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9,
  0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
  0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B,
  0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
  0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D,
  0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
  0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F,
  0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
  0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB,
  0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
  0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9,
  0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
  0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F,
  0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
  0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D,
  0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
  0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26,
  0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
  0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74,
  0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
  0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82,
  0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
  0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0,
  0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};

uint8_t crc8(const uint8_t * ptr, uint8_t len)
{
  uint8_t crc = 0;
  for (uint8_t i = 0; i<len; i++) {
    crc = crc8tab[crc ^ *ptr++];
  }
  return crc;
}

static void sa_tx_packet(uint8_t cmd, uint32_t value){
  //here: length --> only payload, without CRC
  //here: CRC --> calculated for complete packet 0xAA ... payload
  uint8_t buff[10];
  uint8_t packetLength = 0;
  buff[0] = 0x00;
  buff[1] = 0xAA; //sync
  buff[2] = 0x55; //sync
  buff[3] = (cmd << 1) | 0x01; //cmd

  switch (cmd){
  case SA_GET_SETTINGS:
    buff[4] = 0x00; //length
    buff[5] = crc8(&buff[1],4);
    buff[6] = 0x00;
    packetLength = 7;
    break;
  case SA_SET_POWER:
    buff[4] = 0x01; //length
    buff[5] = (unify.vtx_version == SA_V1) ? V1_power_lookup[value] : value;
    buff[6] = crc8(&buff[1], 5);
    buff[7] = 0x00;
    packetLength = 8;
    break;
  case SA_SET_CHANNEL:
    buff[4] = 0x01; //length
    buff[5] = value;
    buff[6] = crc8(&buff[1], 5);
    buff[7] = 0x00;
    packetLength = 8;
    break;
  case SA_SET_FREQUENCY:
    buff[4] = 0x02;
    buff[5] = (value>>8); //high byte first
    buff[6] = value;
    buff[7] = crc8(&buff[1], 6);
    buff[8] = 0x00;
    packetLength = 9;
    break;
  case SA_SET_MODE: //supported for V2 only: UNIFY HV and newer
    if (unify.vtx_version == SA_V2){
      //TBD --> Pit mode
      /*
      buffer[4] = 0x01; //length
      buffer[5] = value;
      buffer[6] = crc8(&buffer[1], 5);
      buffer[7] = 0x00;
      packetLength = 8;s
      */
    }
    break;
  }
  for(int i=0;i<packetLength; i++){
    Serial1.write(buff[i]);
  }
}

static void sa_rx_packet(uint8_t *buff, uint8_t len){
  //verify packet
  uint8_t packetStart=0;
  for(int i=0;i<len-3;i++){
    if(buff[i]==0xAA && buff[i+1]==0x55 && buff[i+3]<len){
      packetStart=i+2;
      uint8_t len=buff[i+3];
      uint8_t crcCalc=crc8(&buff[i+2],len+1);
      uint8_t crc=buff[i+3+len];

      if(crcCalc==crc){
        Serial.println("CRC match");
        switch(buff[packetStart]){
          case SA_GET_SETTINGS: //fall-through
          case SA_GET_SETTINGS_V2:
            Serial.println("SA_GET_SETTINGS");
            unify.vtx_version = (buff[packetStart]==SA_GET_SETTINGS) ? SA_V1 : SA_V2;
            packetStart+=2; //skip cmd and length
            unify.channel = buff[packetStart++];
            unify.powerLevel = buff[packetStart++];
            unify.mode = buff[packetStart++];
            unify.frequency = ((uint16_t)buff[packetStart++]<<8)|buff[packetStart++];
            break;
          case SA_SET_POWER:
            Serial.println("SA_SET_POWER");
            packetStart+=2;
            unify.powerLevel = buff[packetStart++];      
            break;
          case SA_SET_CHANNEL:
            Serial.println("SA_SET_CHANNEL");
            packetStart+=2;
            unify.channel = buff[packetStart++];
            break;
          case SA_SET_FREQUENCY:
            Serial.println("SA_SET_FREQUENCY");
            //TBD: Pit mode Freq
            packetStart+=2;
            unify.frequency = ((uint16_t)buff[packetStart++]<<8)|buff[packetStart++];
            break;
          case SA_SET_MODE:
            //SA V2 only!
            break;
        }
        return;

      }else{
        Serial.println("CRC mismatch");
        return;
      }
    }
  }
  
}

void setup()
{
    
  Serial.begin(9600);
  while (!Serial);             // Leonardo: wait for serial monitor

  Serial.println("Start");
  Serial1.begin(4900,SERIAL_8N2);
  UCSR1B &= ~(1<<TXEN1);
}

uint8_t buff[25];
uint8_t rx_len = 0;
uint8_t zeroes = 0;

int incomingByte = 0; 
void loop(){
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    Serial1.begin(4900,SERIAL_8N2);
    switch(incomingByte){
      case 's':
        sa_tx_packet(SA_GET_SETTINGS,0);
        break;
      case '+':
        sa_tx_packet(SA_SET_CHANNEL,unify.channel+1);
        break;
      case '-':
        sa_tx_packet(SA_SET_CHANNEL,unify.channel-1);
        break;
    }
    Serial1.end();//clear buffer, otherwise sa_tx_packet is received
    Serial1.begin(4900,SERIAL_8N2);
    UCSR1B &= ~(1<<TXEN1); //deactivate tx --> rx mode listening for response
  }
  
  delay(100);
  
  while(Serial1.available()){
    buff[rx_len]=Serial1.read();
    if(buff[rx_len]==0){
      zeroes++;
    }
    //Serial.print(buff[rx_len],HEX);
    //Serial.print(",");
    rx_len++;
  }
  //Serial.println();
  if(rx_len>6){

    //because rx is low in idle 0 is received
    //when calculating crc of 0 we have a crc match, so
    //when all bytes are 0 we should avoid parsing the input data
    if(rx_len==zeroes){
      while(Serial1.available()){
        Serial1.read();
      }
    }else{
      sa_rx_packet(buff,rx_len);
      Serial.print("Version:");
      Serial.print(unify.vtx_version);
      Serial.print(", Channel:");
      Serial.print(unify.channel);
      Serial.print(", PowerLevel:");
      Serial.print(unify.powerLevel);
      Serial.print(", Mode:");
      Serial.print(unify.mode);
      Serial.print(", Frequency:");
      Serial.println(unify.frequency);
    }
    zeroes=0;
    rx_len=0;   
  }
  
}

