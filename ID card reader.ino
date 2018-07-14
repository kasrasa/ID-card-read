#include <EEPROM.h>  
#include <SPI.h>     
#include <MFRC522.h>  
 
#include <Servo.h>
#define COMMON_ANODE
 
#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif
 
#define redLed 7
#define greenLed 6
#define blueLed 5
#define relay 4
 
boolean match = false; 
boolean programMode = false; 
int successRead;
 
byte storedCard[4];  
byte readCard[4];          
byte masterCard[4]; 

 
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN); 
 
//settings
void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_OFF); 
  
  
  Serial.begin(9600);  
  SPI.begin();           
  mfrc522.PCD_Init();    
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
 
  
  
  if (EEPROM.read(1) != 143) {
    Serial.println("No Master Card Defined");
    Serial.println("Scan A PICC to Define as Master Card");
    do {
      successRead = getID();
      digitalWrite(blueLed, LED_ON);
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead); 
    for ( int j = 0; j < 4; j++ ) { 
      EEPROM.write( 2 +j, readCard[j] ); 
    }
    EEPROM.write(1,143); 
    Serial.println("Master Card Defined");
  }
  Serial.println("Master Card's UID");
  for ( int i = 0; i < 4; i++ ) {    
    masterCard[i] = EEPROM.read(2+i); 
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println("Waiting PICCs to bo scanned :)");
  cycleLeds();    
}
 
 
//main loop
void loop () {
  do {
    successRead = getID(); 
    if (programMode) {
      cycleLeds(); 
    }
    else {
      normalModeOn(); 
    }
  }
  while (!successRead);
  if (programMode) {
    if ( isMaster(readCard) ) { 
      Serial.println("This is Master Card"); 
      Serial.println("Exiting Program Mode");
      Serial.println("-----------------------------");
      programMode = false;
      return;
    }
    else {  
      if ( findID(readCard) ) { 
        Serial.println("I know this PICC, so removing");
        deleteID(readCard);
        Serial.println("-----------------------------");
      }
      else {                    
        Serial.println("I do not know this PICC, adding...");
        writeID(readCard);
        Serial.println("-----------------------------");
      }
    }
  }
  else {
    if ( isMaster(readCard) ) {  
      programMode = true;
      Serial.println("Hello Master - Entered Program Mode");
      int count = EEPROM.read(0); 
      Serial.print("I have ");    
      Serial.print(count);
      Serial.print(" record(s) on EEPROM");
      Serial.println("");
      Serial.println("Scan a PICC to ADD or REMOVE");
      Serial.println("-----------------------------");
    }
    else {
      if ( findID(readCard) ) {       
        Serial.println("Welcome, You shall pass");
      }
      else {        
        Serial.println("You shall not pass");
        failed(); 
      }
    }
  }
}
 

int getID() {
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) { 
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  
  Serial.println("Scanned PICC's UID:");
  for (int i = 0; i < 4; i++) { 
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); 
  return 1;
}
 
//LED loop
void cycleLeds() {
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_ON);
  digitalWrite(blueLed, LED_OFF); 
  delay(200);
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_ON);
  delay(200);
  digitalWrite(redLed, LED_ON);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_OFF);
  delay(200);
}
 
// Normal Mode Led 
void normalModeOn () {
  digitalWrite(blueLed, LED_ON);
  digitalWrite(redLed, LED_OFF); 
  digitalWrite(greenLed, LED_OFF);
  }
 
//Reading ID from EEPROM
void readID( int number ) {
  int start = (number * 4 ) + 2; 
  for ( int i = 0; i < 4; i++ ) { 
    storedCard[i] = EEPROM.read(start+i); 
  }
}
 
// adding ID
void writeID( byte a[] ) {
  if ( !findID( a ) ) { 
    int num = EEPROM.read(0);
    int start = ( num * 4 ) + 6;
    num++;
    EEPROM.write( 0, num );
    for ( int j = 0; j < 4; j++ ) {
      EEPROM.write( start+j, a[j] );
    }
    successWrite();
  }
  else {
    failedWrite();
  }
}
 
// deleting ID from EEPROM
void deleteID( byte a[] ) {
  if ( !findID( a ) ) { 
    failedWrite();
  }
  else {
    int num = EEPROM.read(0);
    int slot; // 
    int start;// = ( num * 4 ) + 6;
    int looping;
    int j;
    int count = EEPROM.read(0);
    slot = findIDSLOT( a );
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;
    EEPROM.write( 0, num );
    for ( j = 0; j < looping; j++ ) {
      EEPROM.write( start+j, EEPROM.read(start+4+j));
    }
    for ( int k = 0; k < 4; k++ ) {
      EEPROM.write( start+j+k, 0);
    }
    successDelete();
  }
}
 
// Check Bytes   
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL )
    match = true;
  for ( int k = 0; k < 4; k++ ) { 
    if ( a[k] != b[k] )
      match = false;
  }
  if ( match ) {
    return true;
  }
  else  {
    return false; 
  }
}
 

int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0); 
  for ( int i = 1; i <= count; i++ ) {
    readID(i); 
    if( checkTwo( find, storedCard ) ) { 
      
      return i; 
      break;
    }
  }
}
 

boolean findID( byte find[] ) {
  int count = EEPROM.read(0);
  for ( int i = 1; i <= count; i++ ) { 
    readID(i);
    if( checkTwo( find, storedCard ) ) {
      return true;
      break;
    }
    else { 
    }
  }
  return false;
}
 
void successWrite() {
  digitalWrite(blueLed, LED_OFF);
  digitalWrite(redLed, LED_OFF); 
  digitalWrite(greenLed, LED_OFF);
  delay(200);
  digitalWrite(greenLed, LED_ON);
  delay(200);
  digitalWrite(greenLed, LED_OFF);
  delay(200);
  digitalWrite(greenLed, LED_ON);
  delay(200);
  digitalWrite(greenLed, LED_OFF);
  delay(200);
  digitalWrite(greenLed, LED_ON);
  delay(200);
  Serial.println("Succesfully added ID record to EEPROM");
}
  

void failedWrite() {
  digitalWrite(blueLed, LED_OFF);
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_OFF);
  delay(200);
  digitalWrite(redLed, LED_ON);
  delay(200);
  digitalWrite(redLed, LED_OFF); 
  delay(200);
  digitalWrite(redLed, LED_ON);
  delay(200);
  digitalWrite(redLed, LED_OFF);
  delay(200);
  digitalWrite(redLed, LED_ON);
  delay(200);
  Serial.println("Failed! There is something wrong with ID or bad EEPROM");
}
 

void successDelete() {
  digitalWrite(blueLed, LED_OFF);
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_OFF);
  delay(200);
  digitalWrite(blueLed, LED_ON);
  delay(200);
  digitalWrite(blueLed, LED_OFF);
  delay(200);
  digitalWrite(blueLed, LED_ON);
  delay(200);
  digitalWrite(blueLed, LED_OFF);
  delay(200);
  digitalWrite(blueLed, LED_ON); //
  delay(200);
  Serial.println("Succesfully removed ID record from EEPROM");
}
 
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}
 

void failed() {
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_OFF);
  digitalWrite(redLed, LED_ON);
  delay(1200);
}
