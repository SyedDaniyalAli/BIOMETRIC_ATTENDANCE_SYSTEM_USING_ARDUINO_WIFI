/*
   Finger print verification
   To send the the enrolled finger prints to fire base
   We can't extract the finger print image from the sensor as it gives output in bmp code formate
   We can save upto 1000 different fingers in the sensor.
*/
#include <Adafruit_Fingerprint.h> // for connecting the fingerprint sensor 
#include <SoftwareSerial.h> // for serial communication

#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD

// Wiring: SDA pin is connected to A4 and SCL pin to A5.
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,20,4) for 20x4 LCD.

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//For LCD Display
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// Provide the token generation process info.
#include "addons/TokenHelper.h"

// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


// Insert Firebase project API Key
#define API_KEY "AIzaSyCm9JsiUtqT1ko0rUSj16RTUUawRms4j84"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://biometric-attendance-systm-default-rtdb.firebaseio.com/"

#define WIFI_SSID "DreamNetSDA" // enter the wifi address
#define WIFI_PASSWORD "Daniyal444" // enter it's password

#define isTouchedPin D3
#define registerationMode D5
#define buzzerPin D6





// Define Firebase objects~~~~~~~~~~~~~~
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;


unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

uint8_t getFingerprintEnroll(int id);

SoftwareSerial mySerial(D7, D8); // 12 - D6 - yellow // 13 - d7 - white

const long utcOffsetInSeconds = 3600;


// Variable to save USER UID
String uid;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

struct firebaseUser {

  int ID;
  String NAME;
};

firebaseUser firebase_user;

String id;


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");

  lcd.clear();
  lcd.print("Connecting WiFi");


  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  lcd.clear();
  lcd.print("WiFi Connected");

  Serial.println(WiFi.localIP());
  Serial.println();
}


// One Time Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup()
{
  Serial.begin(115200);
  // Initiate the LCD:
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.print(" Bio-Attendance");
  delay(2500);


  pinMode(isTouchedPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(registerationMode, INPUT);
  noTone(buzzerPin);

  // Initialize WiFi
  initWiFi();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Signed up (ok)");
    lcd.clear();
    lcd.print("Device is Ready");

    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
    lcd.clear();
    lcd.print(" No Internet");
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  //Firebase ~~~~~~~~~~~~~~~~~END~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  finger.begin(57600);
  timeClient.begin();

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");  // to check if the finger print sensor is avaible
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    lcd.clear();
    lcd.print(" No F_Sensor");

    while (1);
  }

  Serial.println();
}


// Repeating Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop()
{
  timeClient.update();

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 200 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (digitalRead(isTouchedPin) == LOW) {

      if (digitalRead(registerationMode) == HIGH)
      {
        lcd.clear();
        lcd.print("Place Finger");
        lcd.setCursor(1, 1);
        lcd.print("Tightly..");
      }

      if (digitalRead(registerationMode) == LOW) {

        tone(buzzerPin, 1000, 100); // Send 1KHz sound signal...
        delay(180);
        tone(buzzerPin, 1000, 100); // Send 1KHz sound signal...

        if (isDeviceActivated()) {

          firebase_user = getDataFromFirebase();

          while (!  getFingerprintEnroll(firebase_user.ID) );
        }
      }
      else {
        getFingerprintID();
      }
    }
  }


  delay(50);
}


// Firebase Methods~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~For Registration~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool isDeviceActivated() {

  lcd.clear();
  lcd.print("Downloading Data");

  bool isUploaded = false;

  // get value
  isUploaded = Firebase.RTDB.getString(&fbdo, "device/state");
  String value = fbdo.to<const char *>();

  isUploaded = Firebase.RTDB.getString(&fbdo, "device/id");
  id = fbdo.to<const char *>();

  Serial.println("device/state: " + String(value));
  Serial.println("device/id: " + String(value));

  if (!isUploaded)   // to find any error in uploading the code
  {
    Serial.print("Failed to get activation record:");
    Serial.println(fbdo.errorReason().c_str());
    lcd.clear();
    lcd.print("No Internet.");
  }

  if (value == "true") {
    lcd.clear();
    lcd.print("Data Downloaded");
    return true;
  }
  else {
    lcd.clear();
    lcd.print("No Record Found");
    return false;
  }

}

//Getting status from firebase~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct firebaseUser getDataFromFirebase() {

  lcd.clear();
  lcd.print("Getting Record");

  bool isUploaded = false;

  isUploaded = Firebase.RTDB.getString(&fbdo, "users/" + String(id) + "/f_id");
  firebase_user.ID = String(fbdo.to<const char *>()).toInt();

  isUploaded =  Firebase.RTDB.getString(&fbdo, "users/" + String(id) + "/name");
  firebase_user.NAME = fbdo.to<const char *>();

  if (!isUploaded)  // to find any error in uploading the code
  {
    Serial.print("Failed to get user record:");
    Serial.println(fbdo.errorReason().c_str());
    lcd.clear();
    lcd.print("Internet Error");
  }

  return firebase_user;
}

//Update status to firebase~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void acknowledgeToFirebase() {

  bool isUploaded = false;

  isUploaded = Firebase.RTDB.setString(&fbdo, "device/state", "false");
  isUploaded = Firebase.RTDB.setString(&fbdo, "users/" + String(firebase_user.ID) + "/registration_status", String("true")); // update child in firebase database
  isUploaded = Firebase.RTDB.setString(&fbdo, "users/" + String(firebase_user.ID) + "/last_status", String("false")); // update child in firebase database


  if (!isUploaded)   // to find any error in uploading the code
  {
    Serial.print("Failed to upload record:");
    Serial.println(fbdo.errorReason().c_str());
    lcd.clear();
    lcd.print("Failed to Upload");
  }
  else   // to find any error in uploading the code
  {
    Serial.print("method(acknowledgeToFirebase) Successfully uploaded");
    lcd.clear();
    lcd.print("User Registered");
  }

}

// Firebase Methods~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~For Attendance~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void getNameAndMarkAttendance(int f_id) {

  bool isUploaded = false;

  Serial.print("Getting data from firebase...");
  lcd.clear();
  lcd.print("Getting Data..");

  isUploaded =  Firebase.RTDB.getString(&fbdo, "users/" + String(f_id) + "/id");
  firebase_user.ID = String(fbdo.to<const char *>()).toInt();

  isUploaded =  Firebase.RTDB.getString(&fbdo, "users/" + String(f_id) + "/name");
  firebase_user.NAME = fbdo.to<const char *>();


  isUploaded = Firebase.RTDB.getString(&fbdo, "users/" + String(f_id) + "/last_status");
  String lastStatus = fbdo.to<const char *>();

  if (lastStatus == "false" || lastStatus == "checkout") {
    isUploaded = Firebase.RTDB.setString(&fbdo, "users/" + String(f_id) + "/last_status", String("checkin")); // update child in firebase database
    isUploaded = Firebase.RTDB.pushString(&fbdo, "attendance/" + String(firebase_user.ID) + "/", "{timestamp:" + String(timeClient.getEpochTime()) + ",status:checkin}");
    lcd.clear();
    lcd.print("Welcome " + String(firebase_user.NAME));
    lcd.setCursor(1, 4);
    lcd.print("Check-In");
  }
  else if (lastStatus == "true" || lastStatus == "checkin") {
    isUploaded = Firebase.RTDB.setString(&fbdo, "users/" + String(f_id) + "/last_status", String("checkout")); // update child in firebase database
    isUploaded = Firebase.RTDB.pushString(&fbdo, "attendance/" + String(firebase_user.ID) + "/", "{timestamp:" + String(timeClient.getEpochTime()) + ",status:checkout}");
    lcd.clear();
    lcd.print("Bye " + String(firebase_user.NAME));
    lcd.setCursor(1, 4);
    lcd.print("Check-Out");
  }
  else {
    Serial.print("method(getNameAndMarkAttendance) Error to get conditions");
    lcd.clear();
    lcd.print("Condition Error");
  }

  if (!isUploaded)   // to find any error in uploading the code
  {
    Serial.print("method(getNameAndMarkAttendance) Failed to get user record:");
    lcd.clear();
    lcd.print("Network Error");
    Serial.println(fbdo.errorReason().c_str());
  }

}

// End of Firebase Methods~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//Finding Finger print Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SlowButAccurate~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      lcd.clear();
      lcd.print("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      lcd.clear();
      lcd.print(" Put Finger");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      lcd.clear();
      lcd.print("Comm7_Error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      lcd.clear();
      lcd.print("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      lcd.clear();
      lcd.print("Unknown Error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      lcd.clear();
      lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      lcd.clear();
      lcd.print("Too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      lcd.clear();
      lcd.print("Comm8_Error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.print("Finding Error");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.print("Invalid Img");
      return p;
    default:
      Serial.println("Unknown error");
      lcd.clear();
      lcd.print("Unknown Error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    lcd.clear();
    lcd.print("Print Matched");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    lcd.clear();
    lcd.print("Comm9_Error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    lcd.clear();
    lcd.print("No Matched");
    return p;
  } else {
    Serial.println("Unknown error");
    lcd.clear();
    lcd.print("Unknown Error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  lcd.clear();
  lcd.print("Found ID: " + String(finger.fingerID));
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  getNameAndMarkAttendance(finger.fingerID);

  return finger.fingerID;
}



//Fast Finding Finger print Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~FASTER~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  lcd.clear();
  lcd.print("Found ID: " + String(finger.fingerID));
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}





//Finger print registration Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#################################~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint8_t getFingerprintEnroll(int id) {
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll");
  lcd.clear();
  lcd.print("Wait to Enroll..");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        lcd.clear();
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        lcd.clear();
        lcd.print("Put Finger..");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        lcd.clear();
        lcd.print(" Comm_Error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        lcd.clear();
        lcd.print("Imaging Error");
        break;
      default:
        Serial.println("Unknown error");
        lcd.clear();
        lcd.print("Unknown Error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      lcd.clear();
      lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      lcd.clear();
      lcd.print("Too Messy..");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      lcd.clear();
      lcd.print("Comm2_Error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.print("Feature_Error");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.print("No Features");
      return p;
    default:
      Serial.println("Unknown error");
      lcd.clear();
      lcd.print("Unknown Error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  p = -1;
  Serial.println("Place same finger again");  // to improve the security and confidence in the finger print measurement
  lcd.clear();
  lcd.print("Put Same Finger");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        lcd.clear();
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        lcd.clear();
        lcd.print("Put Finger..");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        lcd.clear();
        lcd.print("Comm3_Error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        lcd.clear();
        lcd.print("Imaging Error");
        break;
      default:
        Serial.println("Unknown error");
        lcd.clear();
        lcd.print("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      lcd.clear();
      lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      lcd.clear();
      lcd.print("Too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      lcd.clear();
      lcd.print("Comm4_Error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.print("Put Finger.");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.print("Put Finger..");
      return p;
    default:
      Serial.println("Unknown error");
      lcd.clear();
      lcd.print("Unknown Error");
      return p;
  }


  // OK converted!
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");     // got a match and your ready to go
    lcd.clear();
    lcd.print("Print Matched");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    lcd.clear();
    lcd.print("Comm5_Error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.print("F_Not Matched");
    return p;
  } else {
    Serial.println("Unknown error");
    lcd.clear();
    lcd.print("Unknown Error");
    return p;
  }

  //When model is registered~~~~~~~~~~~~~~~~~~~~~~~
  acknowledgeToFirebase();

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    lcd.clear();
    lcd.print("Saved to Local");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    lcd.clear();
    lcd.print("Comm6_Error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    lcd.clear();
    lcd.print("Location Error");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    lcd.clear();
    lcd.print("Writing Error");
    return p;
  } else {
    Serial.println("Unknown error");
    lcd.clear();
    lcd.print("Unknown Error");
    return p;
  }


}
