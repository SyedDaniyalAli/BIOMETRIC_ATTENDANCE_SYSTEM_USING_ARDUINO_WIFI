#include <FirebaseObject.h>
#include <FirebaseCloudMessaging.h>
#include <Firebase.h>
#include <FirebaseError.h>
#include <FirebaseArduino.h>
#include <FirebaseHttpClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>



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
#define FIREBASE_AUTH "bJhOeblIKLSUWKr1MKrbMV5sKbYB6wrwSq1El8TC" // copy the secret code from the service account settings in firebase
#define WIFI_SSID "DreamNetSDA" // enter the wifi address
#define WIFI_PASSWORD "Daniyal444" // enter it's password  

uint8_t getFingerprintEnroll(int id);

SoftwareSerial mySerial(12, 13); // 12 - D6 - yellow // 13 - d7 - blue

const long utcOffsetInSeconds = 3600;



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
  timeClient.begin();

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
  timeClient.update();
  //  if (isDeviceActivated()) {
  //
  //    //TODO  Make Buzzer on for a second
  //
  //    firebase_user = getDataFromFirebase();
  //
  //    while (!  getFingerprintEnroll(firebase_user.ID) );
  //
  //  }
  //  delay(8000);

  getFingerprintID();
  delay(50);


}


// Firebase Methods~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~For Registration~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

//Getting status from firebase~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

//Update status to firebase~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void acknowledgeToFirebase() {

  Firebase.setString("device/state", "false");
  Firebase.setString("users/" + String(firebase_user.ID) + "/registration_status", String("true")); // update child in firebase database
  Firebase.setString("users/" + String(firebase_user.ID) + "/last_status", String("false")); // update child in firebase database

  if (Firebase.failed())   // to find any error in uploading the code
  {
    Serial.print("Failed to upload record:");
    Serial.println(Firebase.error());
  }
  Serial.println("sent to online database");

  if (Firebase.success())   // to find any error in uploading the code
  {
    Serial.print("method(acknowledgeToFirebase) Successfully uploaded");
  }

}

// Firebase Methods~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~For Attendance~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void getNameAndMarkAttendance(int f_id) {

  Serial.print("Getting data from firebase...");
  firebase_user.ID = Firebase.getString("users/" + String(f_id) + "/id").toInt();

  firebase_user.NAME = Firebase.getString("users/" + String(f_id) + "/name");

  String lastStatus = Firebase.getString("users/" + String(f_id) + "/last_status"); // update child in firebase database

  if (lastStatus == "false" || lastStatus == "checkout") {
    Firebase.setString("users/" + String(f_id) + "/last_status", String("checkin")); // update child in firebase database
    Firebase.pushString("attendance/" + String(firebase_user.ID) + "/", "{timestamp:" + String(timeClient.getEpochTime()) + ",status:checkin}");
  }
  else if (lastStatus == "true" || lastStatus == "checkin") {
    Firebase.setString("users/" + String(f_id) + "/last_status", String("checkout")); // update child in firebase database
    Firebase.pushString("attendance/" + String(firebase_user.ID) + "/", "{timestamp:" + String(timeClient.getEpochTime()) + ",status:checkout}");
  }
  else {
    Serial.print("method(getNameAndMarkAttendance) Error to get conditions");
  }

  if (Firebase.failed())   // to find any error in uploading the code
  {
    Serial.print("method(getNameAndMarkAttendance) Failed to get user record:");
    Serial.println(Firebase.error());
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
