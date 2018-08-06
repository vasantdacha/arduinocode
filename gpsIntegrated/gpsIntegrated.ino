/**
Author Vasant Dacha
Code for AirHockey Game
This code does following things
1. Date validation for 1 year
2. Key Check
3. Combo meal count
4. 
**/

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include "Timer.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

RTC_DS3231 rtc;
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
// New board lcd setting
LiquidCrystal lcd(5,6, A3, A2, A1, A0);
//Prototype lcd settings
//LiquidCrystal lcd(7,6, 5, 4, 3, 2);
SoftwareSerial gpsSerial(3,4);
TinyGPSPlus gps;
MFRC522::MIFARE_Key key;

struct cardInit 
{
  static int gameAmount;
  static int amountInCard;
  static byte dataBlock[16];
  static String welcomeText;
  static int gameTime;
  static int powerPin;
  static int GND;
  static int dayCollection;
  static String gameName;
  static int eepromAddr;
  static int prevDate;
  static int tId;
  static int swipeId;
  static int tempCount;
  static bool isRollBack;
  static bool swipeLedFlag;
  static int count;
  static int swipeLedPin;
  static byte cardKey[16];
  static int comboCount;
  static int dayInCard;
  static int monthInCard;
  static int yearInCard;
};

enum STATE {
  START,
  IDLESTATE
}state;


int cardInit::gameAmount = 40;
int cardInit::amountInCard = 0;
byte cardInit::dataBlock[16] = {};
String cardInit::welcomeText = " FunFirst Games ";
int cardInit::gameTime = 20;

int cardInit::powerPin = 8;
int cardInit::dayCollection = 0;

String cardInit::gameName = "### Airhockey-2 ";
int cardInit::eepromAddr = 0;
int cardInit::prevDate = 0;
int cardInit::tId = 0;
int cardInit::swipeId = 0;
int cardInit::tempCount = 0;
bool cardInit::isRollBack = false;
bool cardInit::swipeLedFlag = HIGH;
int cardInit::count = cardInit::gameTime - 5;
int cardInit::swipeLedPin = 2;
byte cardInit::cardKey[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0xFF,0x07,0x80,0x69,
                              0x00,0x00,0x00,0x00,0x00,0x00};  
int cardInit::comboCount = 0;
int cardInit::dayInCard = 0;
int cardInit::monthInCard = 0;
int cardInit::yearInCard = 0;
Timer t;

/**
 * Initialize.
 */
void setup() {
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    gpsSerial.begin(9600);
    int tId = 0;
    cardInit::tempCount = cardInit::gameTime;
    
    state = IDLESTATE;
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    
    key.keyByte[0] = 0x11;
    key.keyByte[1] = 0x22;
    key.keyByte[2] = 0x33;
    key.keyByte[3] = 0x44;
    key.keyByte[4] = 0x55;
    key.keyByte[5] = 0x66;
    
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(cardInit::welcomeText);
    delay(1000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(cardInit::gameName);
    delay(1000);

    
    cardInit::eepromAddr = 1;
    // Read day collection from eeprom
    cardInit::dayCollection = EEPROMReadInt(cardInit::eepromAddr);

    pinMode(cardInit::swipeLedPin,OUTPUT);
    digitalWrite(cardInit::swipeLedPin,LOW);
    pinMode(cardInit::powerPin,OUTPUT);
    digitalWrite(cardInit::powerPin,HIGH);
    //Serial.println("setup");
    //Serial.flush();
    
    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
    }
 
    if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2017, 10, 11, 12, 0, 0));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    DateTime now = rtc.now();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(now.day());
    lcd.print(" ");
    lcd.print(now.month());
    lcd.print(" ");
    lcd.print(now.year());
    delay(1000);
    
    lcdInitialise();
 
  resetDayCollection();
  unsigned long callbackPeriod = 60000;//900000; // every fifteen minutes
  t.every(callbackPeriod,resetDayCollection);        
}

void lcdInitialise()
{
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Tap to Play!");
    lcd.setCursor(0,1); 
    lcd.print("Price:");
    lcd.print(cardInit::gameAmount);
}

void resetDayCollection()
{
  DateTime now = rtc.now();
  cardInit::eepromAddr = 0;
  
  // Read previous date from eeprom
  cardInit::prevDate = EEPROM.read(cardInit::eepromAddr);
  
  // If date is changed, then update with todays's date
  if( cardInit::prevDate != now.day() )
  {
    // day is eepromAddress=0
    EEPROM.update(cardInit::eepromAddr,now.day());
    cardInit::eepromAddr = cardInit::eepromAddr + 1;
    cardInit::dayCollection = 0;
    EEPROMWriteInt(cardInit::eepromAddr,cardInit::dayCollection);
    delay(4);
    cardInit::eepromAddr = 3;
    cardInit::comboCount = 0;
    EEPROMWriteInt(cardInit::eepromAddr,cardInit::comboCount);
    Serial.print(cardInit::gameName);
    Serial.println(cardInit::dayCollection);
    Serial.flush();
  }
}


/**
 * Main loop.
 */
void loop() {
    
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (gpsSerial.available())
    {
      char c = byte(gpsSerial.read());
      if(gps.encode(c))
      {
      if( gps.location.isValid() )
      {
		    Serial.print("Lat:");
        Serial.println(gps.location.lat(), 6);
        Serial.print("Lon:");
        Serial.println(gps.location.lng(), 6);
        TinyGPSSpeed speed;
        Serial.print("Speed:");
        Serial.println(speed.kmph());
	  }
	 }
	}
  }
    t.update();
    if( state == IDLESTATE )
    { 
     
      // Look for new cards
      if ( ! mfrc522.PICC_IsNewCardPresent())
      {
          return;
      }
      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial())
      {
        haltCard();
        return;
      }
      //Serial.println(" LOG ************ AirHockey-2 ***********" );
      //Serial.flush();

    // Sector #1 Block #5 to store DATE information
    byte blockAddrDate = 5;
    byte trailerBlockDate = 7;

    byte dateBuffer[18];
    byte dateSize = sizeof(dateBuffer);
    MFRC522::StatusCode dateStatus;
    dateStatus = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlockDate, &key, &(mfrc522.uid));
    if (dateStatus != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR Date Authentication failed "));
        //Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
   delay(10);
    //Serial.println("Reading Date...");
   dateStatus = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddrDate, dateBuffer, &dateSize);
    if (dateStatus != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR Date read failed "));
        Serial.flush();
        //Serial.println(mfrc522.GetStatusCodeName(status));
        
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    else
    {
      //Serial.println("Printing Date...");
      //Serial.print(dateBuffer[0]);
      cardInit::dayInCard = dateBuffer[0];
      //Serial.print(" ");
      //Serial.print(dateBuffer[1]);
      cardInit::monthInCard = dateBuffer[1];
      //Serial.print(" ");
      //Serial.println(word(dateBuffer[2],dateBuffer[3]));
      cardInit::yearInCard = word(dateBuffer[2],dateBuffer[3]);
    
      DateTime now = rtc.now();       
      if( ( ( now.year() - cardInit::yearInCard ) > 1 ) || ( cardInit::monthInCard > now.month() ) || ( cardInit::dayInCard
      > now.day() ) )
      {
            Serial.println("ERROR Amount in card expired");
            haltCard();
            return;
      }
       
    }// End of date authentication
 
      
      //Serial.println("Scan Card");
    cardInit::cardInit::isRollBack = false;
    String master = "45 e4 93 63 ";
    String k = getCardUID(mfrc522.uid.uidByte, mfrc522.uid.size);
    if( k == master )
    { 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Today's Colln:");
      lcd.setCursor(0,1);
      lcd.print(cardInit::dayCollection);
      delay(2000);
      k = "";
      lcdInitialise();
      return;
    }
    
            
    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte bufferSize = sizeof(buffer);

    // Authenticate using key A
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR PCD_Authenticate() Key-A failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        haltCard();
        return;
    }
   delay(10);
      
   status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &bufferSize);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR first MIFARE_Read() failed: "));
        Serial.flush();
        Serial.println(mfrc522.GetStatusCodeName(status));
        haltCard();
        return;
    }
    delay(10);
    cardInit::amountInCard = word(buffer[0],buffer[1]);
    if( readCombo() )
    {
      haltCard();
      return;
    }
    //Serial.print("LOG Before deduct AmountInCard:");
    //Serial.println(cardInit::amountInCard);
    if( cardInit::amountInCard >= cardInit::gameAmount )
    {
        //cardInit::swipeId = t.every(1000,blinkLedOnce,2);
        cardInit::eepromAddr = 1;
        cardInit::amountInCard = cardInit::amountInCard - cardInit::gameAmount;
        //Serial.print("LOG After deduct AmountInCard:");
        //Serial.println(cardInit::amountInCard);
        EEPROMReadInt(cardInit::eepromAddr);
        delay(5);
        cardInit::dayCollection += cardInit::gameAmount;
        //Serial.print("LOG dayCollection:");
        //Serial.println(cardInit::dayCollection);
        EEPROMWriteInt(cardInit::eepromAddr,cardInit::dayCollection);
        delay(5);
        DateTime now = rtc.now();
        // Check if combo offer is present 
        
        /*if( readCombo(buffer,bufferSize) )
        {
          // Check if Combo offer is for the same date of purchase.
          if( ( now.year() == cardInit::yearInCard ) && ( now.month() == cardInit::monthInCard) 
          && (now.day() == cardInit::dayInCard) )
          {
            cardInit::comboCount++;
            // Store combo count in EEPROM
            cardInit::eepromAddr = 4;
            EEPROMWriteInt(cardInit::eepromAddr,cardInit::comboCount);
            Serial.print("COMBO ");
            Serial.println(EEPROMReadInt(cardInit::eepromAddr));
            Serial.flush();
          }
          else
          {
            Serial.println("ERROR Combo Expired !");
            Serial.flush();  
          }
        }*/
        /*cardInit::eepromAddr = 4;
        Serial.print("COMBO ");
        Serial.println(EEPROMReadInt(cardInit::eepromAddr));
        //Serial.println(cardInit::comboCount);
        Serial.flush();*/
    }
    else
    {
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Zero Balance !");
       
       status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
       if (status != MFRC522::STATUS_OK) {
          Serial.print(F("ERROR PCD_Authenticate() Key-A failed: "));
          Serial.flush();
          Serial.println(mfrc522.GetStatusCodeName(status));
        /*  if( !cardInit::isRollBack )
          {
              rollBack();
          }*/
          haltCard();
          delay(1500);
          lcdInitialise();
          return;
       }
       for(int i=0;i<16;i++)
       {
        cardInit::dataBlock[i] = 0;
       }
       delay(10);
       status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, cardInit::dataBlock, 16);
       if (status != MFRC522::STATUS_OK) {
          Serial.print(F("ERROR Zero balance MIFARE_Write() failed: "));
          Serial.flush();
          Serial.println(mfrc522.GetStatusCodeName(status));
          //rollBack();
          haltCard();
          delay(1500);
          lcdInitialise();
          return;
      }
      delay(10); 
    
    haltCard();
    delay(2000);
    lcdInitialise();
    return;
   }
    Serial.print(cardInit::gameName);
    Serial.println(cardInit::dayCollection);
    Serial.flush(); 
    int h = highByte(cardInit::amountInCard);
    int l = lowByte(cardInit::amountInCard);
    for(int i=0;i<16;i++)
    {
      if( i == 0 )
      {
          cardInit::dataBlock[i] = h; 
      }
      else if( i == 1 )
      {
          cardInit::dataBlock[i] = l;
      }
      else
      {
          cardInit::dataBlock[i] = 0;
      }
    }  
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR second PCD_Authenticate() Key-A failed: "));
        Serial.flush();
        Serial.println(mfrc522.GetStatusCodeName(status));
        if( !cardInit::isRollBack )
        {
            rollBack();
        }
        
        haltCard();
        return;
    }
     delay(10);
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, cardInit::dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("ERROR second MIFARE_Write() failed: "));
        Serial.flush();
        Serial.println(mfrc522.GetStatusCodeName(status));
        if( !cardInit::isRollBack )
        {
            rollBack();
        }
        
        haltCard();
        return;
    }
    delay(10);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Deducted amt:");
    lcd.print(cardInit::gameAmount);
    lcd.setCursor(0,1);
    lcd.print("Balance amt:");
    lcd.print(cardInit::amountInCard);
   } 

   if( cardInit::eepromAddr == EEPROM.length() )
   {
       cardInit::eepromAddr = 0; 
   }
    haltCard();
    cardInit::isRollBack = false;
}

void blinkLedOnce()
{
  cardInit::swipeLedFlag = !cardInit::swipeLedFlag;
  digitalWrite(cardInit::swipeLedPin,cardInit::swipeLedFlag);
}


void rollBack()
{
        Serial.println("LOG *** RollBack ***");
        cardInit::isRollBack = true;
        t.stop(cardInit::tId);
        state = IDLESTATE;        
        cardInit::eepromAddr = 1;
        cardInit::amountInCard = cardInit::amountInCard + cardInit::gameAmount;
        cardInit::dayCollection = EEPROMReadInt(cardInit::eepromAddr);
        delay(5);
        // To avoid day collection in negative value
        if( cardInit::dayCollection >= cardInit::gameAmount )
        {
            cardInit::dayCollection -= cardInit::gameAmount;
        }
        EEPROMWriteInt(cardInit::eepromAddr,cardInit::dayCollection);
        delay(5);
        Serial.print("LOG Rollbacked AmountInCard:");
        Serial.println(cardInit::amountInCard);
        Serial.print("LOG Rollbacked dayColl:");
        Serial.println(cardInit::dayCollection);
        Serial.println("LOG **********************************");
}

// Halt PICC card
void haltCard()
{
  // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
}

//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int p_address, int p_value)
     {
     byte lowByte = ((p_value >> 0) & 0xFF);
     byte highByte = ((p_value >> 8) & 0xFF);

     EEPROM.write(p_address, lowByte);
     EEPROM.write(p_address + 1, highByte);
     }

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
     {
     byte lowByte = EEPROM.read(p_address);
     byte highByte = EEPROM.read(p_address + 1);

     return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
     }


String getCardUID(byte *buffer, byte bufferSize) {
   String UID="";
    for (byte i = 0; i < bufferSize; i++) {
        UID += String(buffer[i],HEX);
        UID += " ";
    }
       
    return UID;
}

// Returns 'true' if it is a COMBO recharge
bool readCombo(byte *buffer,byte bufferSize)
{
  String combo="";
  for(int index = 2; index <= 6; index++ )
  {
    combo += char(buffer[index]);
  }
  combo+="\0";
  if( combo.equals("combo") )
  {
    return true;
  }
  return false;
}

bool readCombo()
{
  byte sector = 1;
  byte blockAddrCombo = 6;
  
  byte comboBuffer[18];
  byte comboSize = sizeof(comboBuffer);

  MFRC522::StatusCode status;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddrCombo, comboBuffer, &comboSize);
  if (status != MFRC522::STATUS_OK) {
  Serial.print(F("ERROR MIFARE_Read() failed: "));
  Serial.println(mfrc522.GetStatusCodeName(status));
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return;
  }

  if( comboBuffer[0] > 0 )
  {
    for( int index = 0; index < 16; index++ )
    {
      
      if( index == 0 )
      {
        Serial.print("combo count:");
        Serial.println(comboBuffer[index]);
        Serial.flush();
        comboBuffer[index] = comboBuffer[index] - 1;
        cardInit::comboCount++;
      }
      else
      {
        comboBuffer[index] = 0x00;
      }
    }
    DateTime now = rtc.now();
  if( ( now.year() == cardInit::yearInCard ) && ( now.month() == cardInit::monthInCard) 
          && (now.day() == cardInit::dayInCard) )
          {
            // Store combo count in EEPROM
            cardInit::eepromAddr = 4;
            EEPROMWriteInt(cardInit::eepromAddr,cardInit::comboCount);
            
          }
          else
          {
            Serial.println("ERROR Combo Expired !");
            Serial.flush();  
          }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(6, comboBuffer, 16);
  if (status != MFRC522::STATUS_OK) {
  Serial.print(F("ERROR Combo Re-write failed! "));
  Serial.println(mfrc522.GetStatusCodeName(status));
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return;
  }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(6, comboBuffer, &comboSize);
  if (status != MFRC522::STATUS_OK) {
  Serial.print(F("ERROR MIFARE_Read() failed: "));
  Serial.println(mfrc522.GetStatusCodeName(status));
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return;
  }
  
  cardInit::eepromAddr = 4;
  Serial.print("combo's present:");
  Serial.println(EEPROMReadInt(cardInit::eepromAddr));
  Serial.flush();
  return true;
  }
  else
  {
    return false;
  }
  
}

