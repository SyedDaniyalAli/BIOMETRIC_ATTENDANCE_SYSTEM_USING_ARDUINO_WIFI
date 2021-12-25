#include <FirebaseObject.h>
#include <FirebaseCloudMessaging.h>
#include <Firebase.h>
#include <FirebaseError.h>
#include <FirebaseArduino.h>
#include <FirebaseHttpClient.h>

/*
   Finger print verification
   To send the the enrolled finger prints to fire base
   We can't extract the finger print image from the sensor as it gives output in bmp code formate
   We can save upto 1000 different fingers in the sensor.
*/

#include <ESP8266WiFi.h>    //for node mcu
#include <FirebaseArduino.h> // to connect node mcu and firebase
#include <Adafruit_Fingerprint.h> // for connecting the fingerprint sensor 
#include <SoftwareSerial.h> // for serial communication

#define FIREBASE_HOST "biometric-attendance-systm-default-rtdb.firebaseio.com"  // app database
#define FIREBASE_AUTH "lfMw6rF1guSFtbiGDt9srL4uhnSKhDUb3uYBDKMe" // copy the secret code from the service account settings in firebase
#define WIFI_SSID "DreamNetSDA" // enter the wifi address
#define WIFI_PASSWORD "Daniyal444" // enter it's password  

uint8_t getFingerprintEnroll(int id);

SoftwareSerial mySerial(12, 13); // 12 - D6 - yellow // 13 - d7 - blue


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

struct firebaseUser {

  int ID;
  String NAME;
};

firebaseUser firebase_user;

String id;


// One Time Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup()
{
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // to check for the wifi connection
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to internet");
  Serial.println(WiFi.localIP()); // to print the ip address


  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");  // to check if the finger print sensor is avaible
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }
  Serial.println();


  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); // initilize the firebase
}


// Repeating Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop()
{
  if (isDeviceActivated()) {

    //TODO  Make Buzzer on for a second

    firebase_user = getDataFromFirebase();

    while (!  getFingerprintEnroll(firebase_user.ID) );

  }
  delay(2000);

}

bool isDeviceActivated() {

  // get value
  String value = Firebase.getString("device/state");
  id = Firebase.getString("device/id");
  Serial.println("device/state" + String(value));
  Serial.println("device/id" + String(value));

  if (Firebase.failed())   // to find any error in uploading the code
  {
    Serial.print("Failed to get activation record:");
    Serial.println(Firebase.error());
  }

  if (value == "true") {
    return true;
  }
  else {
    return false;
  }

}

//Update status from firebase~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct firebaseUser getDataFromFirebase() {

  firebase_user.ID = Firebase.getString("users/" + String(id) + "/f_id").toInt();
  firebase_user.NAME = Firebase.getString("users/" + String(id) + "/name");

  if (Firebase.failed())   // to find any error in uploading the code
  {
    Serial.print("Failed to get user record:");
    Serial.println(Firebase.error());
  }

  return firebase_user;
}

//Finger print registration Code~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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


void acknowledgeToFirebase(){

   Firebase.setString("device/state", "false");
   Firebase.setString("users/"+String(firebase_user.ID)+"/registration_status", String("true")); // update child in firebase database

  if (Firebase.failed())   // to find any error in uploading the code
  {
    Serial.print("Failed to upload record:");
    Serial.println(Firebase.error());
  }
  Serial.println("sent to online database");
  
}
