/*
   Finger print verification
   To send the the enrolled finger prints to fire base
   We can't extract the finger print image from the sensor as it gives output in bmp code formate
   We can save upto 1000 different fingers in the sensor.
*/
#include <Adafruit_Fingerprint.h> // for connecting the fingerprint sensor 
#include <SoftwareSerial.h> // for serial communication

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>


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

#define isTouchedPin 4
#define buzzerPin 2
#define registerationMode 12




// Define Firebase objects~~~~~~~~~~~~~~
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;


unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

uint8_t getFingerprintEnroll(int id);

SoftwareSerial mySerial(12, 13); // 12 - D6 - yellow // 13 - d7 - blue

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
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}


// One Time Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup()
{
  Serial.begin(115200);

  pinMode(isTouchedPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(registerationMode, INPUT);

  // Initialize WiFi
  initWiFi();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;


  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Signed up (ok)");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
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
    while (1);
  }

  Serial.println();
}


// Repeating Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop()
{

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 10000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (digitalRead(isTouchedPin)) {

      tone(buzzerPin, 1000); // Send 1KHz sound signal...
      delay(500);        // ...for 1 sec
      noTone(buzzerPin);     // Stop sound...

      timeClient.update();

      if (digitalRead(registerationMode)) {

        tone(buzzerPin, 1000); // Send 1KHz sound signal...
        delay(50);
        noTone(buzzerPin);     // Stop sound...
        tone(buzzerPin, 1000); // Send 1KHz sound signal...
        delay(50);
        noTone(buzzerPin);     // Stop sound...

        if (isDeviceActivated()) {

          //TODO  Make Buzzer on for a second

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

  bool isUploaded = false;

  // get value
  isUploaded = Firebase.RTDB.getString(&fbdo, "device/state");
  String value = fbdo.to<const char *>();

  isUploaded = Firebase.RTDB.getString(&fbdo, "device/id");
  id = fbdo.to<const char *>();

  Serial.println("device/state" + String(value));
  Serial.println("device/id" + String(value));

  if (!isUploaded)   // to find any error in uploading the code
  {
    Serial.print("Failed to get activation record:");
    Serial.println(fbdo.errorReason().c_str());
  }

  if (value == "true") {
    return true;
  }
  else {
    return false;
  }

}

//Getting status from firebase~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct firebaseUser getDataFromFirebase() {

  bool isUploaded = false;

  isUploaded = Firebase.RTDB.getString(&fbdo, "users/" + String(id) + "/f_id");
  firebase_user.ID = String(fbdo.to<const char *>()).toInt();

  isUploaded =  Firebase.RTDB.getString(&fbdo, "users/" + String(id) + "/name");
  firebase_user.NAME = fbdo.to<const char *>();

  if (!isUploaded)  // to find any error in uploading the code
  {
    Serial.print("Failed to get user record:");
    Serial.println(fbdo.errorReason().c_str());
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
  }
  else   // to find any error in uploading the code
  {
    Serial.print("method(acknowledgeToFirebase) Successfully uploaded");
  }

}

// Firebase Methods~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~For Attendance~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void getNameAndMarkAttendance(int f_id) {

  bool isUploaded = false;

  Serial.print("Getting data from firebase...");

  isUploaded =  Firebase.RTDB.getString(&fbdo, "users/" + String(f_id) + "/id");
  firebase_user.ID = String(fbdo.to<const char *>()).toInt();

  isUploaded =  Firebase.RTDB.getString(&fbdo, "users/" + String(f_id) + "/name");
  firebase_user.NAME = fbdo.to<const char *>();


  isUploaded = Firebase.RTDB.getString(&fbdo, "users/" + String(f_id) + "/last_status");
  String lastStatus = fbdo.to<const char *>();

  if (lastStatus == "false" || lastStatus == "checkout") {
    isUploaded = Firebase.RTDB.setString(&fbdo, "users/" + String(f_id) + "/last_status", String("checkin")); // update child in firebase database
    isUploaded = Firebase.RTDB.pushString(&fbdo, "attendance/" + String(firebase_user.ID) + "/", "{timestamp:" + String(timeClient.getEpochTime()) + ",status:checkin}");
  }
  else if (lastStatus == "true" || lastStatus == "checkin") {
    isUploaded = Firebase.RTDB.setString(&fbdo, "users/" + String(f_id) + "/last_status", String("checkout")); // update child in firebase database
    isUploaded = Firebase.RTDB.pushString(&fbdo, "attendance/" + String(firebase_user.ID) + "/", "{timestamp:" + String(timeClient.getEpochTime()) + ",status:checkout}");
  }
  else {
    Serial.print("method(getNameAndMarkAttendance) Error to get conditions");
  }

  if (!isUploaded)   // to find any error in uploading the code
  {
    Serial.print("method(getNameAndMarkAttendance) Failed to get user record:");
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
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
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
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}





//Finger print registration Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#################################~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint8_t getFingerprintEnroll(int id) {
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
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
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }


  // OK converted!
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");     // got a match and your ready to go
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  //When model is registered~~~~~~~~~~~~~~~~~~~~~~~
  acknowledgeToFirebase();

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }


}
