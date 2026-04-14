#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

//---------------------------------------------------------------------------------------------------------
// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbyPen94YOBL5UzgCRWpEsu59akoWD4H641nRirgQeXrQoMIGHrTbfl_q__JindY32xHwQ";
//---------------------------------------------------------------------------------------------------------
// Enter network credentials:
const char* ssid     = "sandy";
const char* password = "RedmiA45G";
//---------------------------------------------------------------------------------------------------------
// Enter command (insert_row or append_row) and your Google Sheets sheet name:
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Attendance\", \"values\": ";
String payload = "";
//---------------------------------------------------------------------------------------------------------
// Google Sheets setup
const char* host        = "script.google.com";
const int   httpsPort   = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;
//------------------------------------------------------------
// Variables to be published
String student_Rollno, first_name, last_name, dept, year_address;
//------------------------------------------------------------
int blocks[] = {4,5,6,8,9};
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))
//------------------------------------------------------------
#define RST_PIN  0  //D3
#define SS_PIN   2  //D4
#define BUZZER   4  //D2
//------------------------------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;
//------------------------------------------------------------
byte bufferLen = 18;
byte readBlockData[18];

//***************************************************************************************************
// setup function
//***************************************************************************************************
void setup() {
  pinMode(D8, OUTPUT);
  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');
  SPI.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting to");
  lcd.setCursor(0,1);
  lcd.print("WiFi...");

  WiFi.begin(ssid, password);             
  Serial.print("Connecting to "); Serial.print(ssid); Serial.println(" ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting to");
  lcd.setCursor(0,1);
  lcd.print("Google ");
  delay(5000);

  Serial.print("Connecting to "); Serial.println(host);

  bool flag = false;
  for(int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(msg);
      delay(2000);
      break;
    }
    else Serial.println("Connection failed. Retrying...");
  }

  if (!flag){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connection fail");
    Serial.print("Could not connect to server: "); Serial.println(host);
    delay(5000);
    return;
  }

  delete client;
  client = nullptr;
}

//***************************************************************************************************
// loop function
//***************************************************************************************************
void loop() {
  static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){ client->connect(host, httpsPort); }
  } else{ Serial.println("Error creating client object!"); }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scan your Tag");

  mfrc522.PCD_Init();

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  Serial.println();
  Serial.println(F("Reading last data from RFID..."));

  String values = "", data;
  // read blocks one by one
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);

    data = String((char*)readBlockData);
    data.trim();

    if(i == 0) { student_Rollno = data; values = "\"" + data + ","; }
    else if(i == 1) { first_name = data; values += data + ","; }
    else if(i == 2) { last_name = data; values += data + ","; }
    else if(i == 3) { dept = data; values += data + ","; }
    else if(i == 4) { year_address = data; values += data + "\"}"; }
  }

  payload = payload_base + values;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Publishing Data");
  lcd.setCursor(0,1);
  lcd.print("Please Wait...");

  Serial.println("Publishing data...");
  Serial.println(payload);

  if(client->POST(url, host, payload)){ 
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Roll no: " + student_Rollno);
    lcd.setCursor(0,1);
    lcd.print("Present");
    digitalWrite(D8, HIGH);
    delay(2000);
    digitalWrite(D8, LOW);
    delay(2000);
  } else {
    Serial.println("Error while connecting");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Failed.");
    lcd.setCursor(0,1);
    lcd.print("Try Again");
  }

  delay(5000);
}

//***************************************************************************************************
// ReadDataFromBlock function
//***************************************************************************************************
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  } else { Serial.println("Authentication success"); }

  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: "); Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    // Ensure null termination
    for(int i=0;i<16;i++){ if(readBlockData[i]==0) readBlockData[i]=' '; }
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }
}

//***************************************************************************************************
// END
//***************************************************************************************************
