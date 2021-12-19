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

  Serial.println("Enter ID"); // enter a new id for every new finger , it will be used for identification in future
  int id = 0;
  while (true) {
    while (! Serial.available());    // to get the user id
    char c = Serial.read();
    if (! isdigit(c)) break;
    id *= 10;
    id += c - '0';
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);



  while (!  getFingerprintEnroll(id) );
}






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


  String Enroll = String("users") + String(id); // adding these two everytime creates a new child in fire base

  Firebase.pushString("fingerprint database" , (Enroll)); // To get multiple entries in the database

  if (Firebase.failed())   // to find any error in uploading the code
  {
    Serial.print("Failed to upload record:");
    Serial.println(Firebase.error());
  }
  Serial.println("sent to online database");

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
