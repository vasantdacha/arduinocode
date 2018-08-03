/**
 * ----------------------------------------------------------------------------
 * This is a MFRC522 library example; see https://github.com/miguelbalboa/rfid
 * for further details and other examples.
 * 
 * NOTE: The library file MFRC522.h has a lot of useful info. Please read it.
 * 
 * Released into the public domain.
 * ----------------------------------------------------------------------------
 * This sample shows how to read and write data blocks on a MIFARE Classic PICC
 * (= card/tag).
 * 
 * BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).
 * 
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 * 
 */

#include <SPI.h>
#include <MFRC522.h>
#include "RTClib.h"

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;
MFRC522::MIFARE_Key mykey;

struct cardInit
{
  static int amount;
  static byte dataBlock[16];
  static byte cardKey[16];
  static byte combo[5];
  static byte customerName[10];
};

int cardInit::amount = 0;
byte cardInit::dataBlock[16]={};
byte cardInit::cardKey[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0xFF,0x07,0x80,0x69,
                              0x00,0x00,0x00,0x00,0x00,0x00};  
byte cardInit::combo[5] = {};
byte cardInit::customerName[10]={};
RTC_DS3231 rtc;
 DateTime now;
/**
 * Initialize.
 */
void setup() {
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card
    
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    mykey.keyByte[0] = 0x11;
    mykey.keyByte[1] = 0x22;
    mykey.keyByte[2] = 0x33;
    mykey.keyByte[3] = 0x44;
    mykey.keyByte[4] = 0x55;
    mykey.keyByte[5] = 0x66;

    pinMode(3,OUTPUT);
    digitalWrite(3,HIGH);
    Serial.println("Setup");

   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
    }
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    now = rtc.now();
    Serial.println(now.day());
    Serial.println(now.month());
    Serial.println(now.year());
    
}

/**
 * Main loop.
 */
void loop() {
      
          // Look for new cards
      if ( ! mfrc522.PICC_IsNewCardPresent())
      {
        return;
      }
      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial())
      {
          return;
      }  
    String k = getCardUID(mfrc522.uid.uidByte, mfrc522.uid.size);  
    Serial.println(k); // In this sample we use the second sector,
    Serial.flush();
    MFRC522::StatusCode status; 
    
    byte sector1        = 1;
    byte blockAddr      = 4;
    byte trailerBlock   = 7;
    
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &mykey, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR PCD_Authenticate() 1st Key-A failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    //Serial.println("Rupee authentication success");
    //Serial.flush();
      
    byte buffer[18];
    byte size = sizeof(buffer);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    cardInit::amount = word(buffer[0],buffer[1]);
    Serial.print("RUPEES ");
    Serial.println(cardInit::amount);
    Serial.flush();
   
    // Sector #1, block#5 to store DATE information
    //byte sector = 1;
    byte blockAddrDate = 5;
    byte trailerBlockDate = 7;

    byte dateBuffer[18];
    byte dateSize = sizeof(dateBuffer);
    
    // Sector #4, block#16 to store customer name
    byte sector  = 4;
    byte blockAddrName      = 16;
    byte trailerBlockName   = 19;
    byte nameBuffer[18];
    byte nameSize = sizeof(nameBuffer);
    
    delay(10);
/**********************************************************************************************/
    if( Serial.available() > 0)
    {
      String recv = Serial.readString();
      Serial.println("recv:" + recv);
      Serial.flush();
      if( recv.startsWith("name") )
      {
        String customerName = recv.substring(4);
        customerName.getBytes(cardInit::customerName,customerName.length()+1);
        
                
        status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlockName, &mykey, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR PCD_Authenticate() 1st Key-A failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
        }
        for(int i=0;i < customerName.length();i++)
        {
          nameBuffer[i] = cardInit::customerName[i];  
        }
        for(int i=customerName.length();i<16;i++)
        {
          nameBuffer[i] = 0x00;
        }
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddrName, nameBuffer, 16);
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("Name Write failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
        }
      }   
      else if( recv == "reset" )
      {
        for(byte data=0;data<16;data++)
        {
             cardInit::dataBlock[data] = 0;      
        }
                status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &mykey, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR PCD_Authenticate() 1st Key-A failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
        }

        status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, cardInit::dataBlock, 16);
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("MIFARE_Write() failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
        }
        Serial.println("SUCCESS amount is reset to '0' ");
        delay(10);
        cardInit::amount = 0;
        Serial.print("RUPEES ");
        Serial.println(cardInit::amount);
        Serial.flush();
        
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
      }
      else
      {
      int amt = recv.toInt();
      amt = amt+cardInit::amount;
      
      byte high = highByte(amt);
      byte low = lowByte(amt);
        for(byte data=0;data<16;data++)
        {
            if( data == 0 && high > 0 )
            {
              cardInit::dataBlock[data] = high;
            }
            else if( data == 1 && low > 0 )
            {
              cardInit::dataBlock[data] = low;
            }
            else
            {
              cardInit::dataBlock[data] = 0x00;
            }
        }
      
    delay(10);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, cardInit::dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    
    Serial.println("SUCCESS amount recharged ");
    Serial.flush();
    delay(10);
    
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    delay(10);
          
    int totalAmount = word(buffer[0],buffer[1]);      
    Serial.print("RUPEES ");
    Serial.println(totalAmount);

    delay(10);
   
      }//else

   unsigned int currentDay = now.day();
   //Serial.print(currentDay);
   unsigned int currentMonth = now.month();
   //Serial.print(" " + currentMonth);
   int currentYear = now.year();
   //Serial.print(" " + currentYear);

   for( byte data=0;data<16;data++)
   {
        if( data == 0 )
        {
          dateBuffer[data] = currentDay;
        }
        else if( data == 1 )
        {
          dateBuffer[data] = currentMonth;
        }
        else if( data == 2 )
        {
            byte h = highByte(currentYear);
            dateBuffer[data] = h;
        }
        else if( data == 3 )
        {
            byte l = lowByte(currentYear);
            dateBuffer[data] = l;
        }
        else
        {
          dateBuffer[data] = 0;  
        }
   }

          status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &mykey, &(mfrc522.uid));
        if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR Date Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
        }

   
   status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddrDate, dateBuffer, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.println(F("ERROR Date write failed "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        Serial.flush();
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    delay(10);
    
    //Serial.println("Reading Date...");
   status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddrDate, dateBuffer, &dateSize);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR Date read failed "));
        Serial.flush();
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    
    }    
 
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}

// Returns card UID
String getCardUID(byte *buffer, byte bufferSize) {
   String UID="UID ";
    for (byte i = 0; i < bufferSize; i++) {
        UID += String(buffer[i],HEX);
        UID += " ";
    }    
    return UID;
}

// This function returns only integer in string
//e.g string = "100combo"; returns 100
int getAmount( String str )
{
  int index = 0;
  int len = str.length();
  String temp = "";
  for( index = 0; index < len; index++ )
  {
    if(isDigit(str[index]))
    {
      temp+=str[index];
    }
  }
  temp += "\0";
  if( temp.length() == 0 )
  {
    return -1;
  }
  return temp.toInt();
}

