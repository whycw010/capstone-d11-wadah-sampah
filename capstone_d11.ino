#include <Arduino.h>
#include <WiFi.h>

// --- Firebase RTDB ---
#define DATABASE_URL "URL"
#define API_KEY "KEY"
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;
// --- Firebase RTDB ---

// WiFi info
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"

// ID unit pengambil sampah
#define UNIT_ID "1"

const int secondsBetweenUpdates = 60; // 1 hour for deployment
unsigned long milisecTime = 0;
unsigned long lastUpdateTime = 0;

void setup() {
    Serial.begin(115200);
    setupWiFi();
    setupRtdb();
    configTime(0, 0, "pool.ntp.org");
    //Firebase.RTDB.setString(&fbdo, "test", "hello world");// TEST CODE
}

void loop() {
  milisecTime = millis();
  if(Firebase.ready() && signupOK && ((milisecTime - lastUpdateTime) >= secondsBetweenUpdates * 1000)){ //
    int volumePercentage = measureVolumePercentage();
    sendDataToRtdb(volumePercentage);
    lastUpdateTime = milisecTime;

  }
}
void sendDataToRtdb(int volumePercentage){
  Firebase.RTDB.setInt(&fbdo, "id/" UNIT_ID "/status/percentFilled", volumePercentage);   //
  Firebase.RTDB.setInt(&fbdo, "id/" UNIT_ID "/status/timestamp", getEpochTime());  // TODO: send both data at once
  return; //TODO: report if call successfull
}

int measureVolumePercentage(){
  int percent = 75; // test
  return percent;
}

void setupRtdb(){
  /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Sign up */
    if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("connected to rtdb");
      signupOK = true;
    }
    else {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  return;
}
void setupWiFi(){
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    return;
}

unsigned long getEpochTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
