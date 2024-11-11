#include <Arduino.h>
#include <WiFi.h>

// WiFi and Firebase Credentials Header file
#include "Credentials.h"

// --- Firebase RTDB ---
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


// ID unit pengambil sampah
#define UNIT_ID "1"

#define SECONDS_BETWEEN_SENSOR_READING 2
#define READINGS_BETWEEN_DB_UPDATE 5
#define SECONDS_BETWEEN_HISTORY_UPDATE 60 

//#define SECONDS_BETWEEN_DB_UPDATE 30;
unsigned long milisecTime = 0;
unsigned long lastSensorUpdateTime = 0;
unsigned long lastHistoryUpdate = 0;
//unsigned long lastDBUpdateTime = 0;


// Volume measurement 
const int pin_sensor[3][3] = {{18, 19, 21}, {27, 14, 22}, {13, 12, 23}};
#define SENSOR_TO_GROUND_DIST 47 //cm
#define CONTAINER_HEIGHT 25 //cm

int lastVolumePercent[READINGS_BETWEEN_DB_UPDATE];
int LVPPointer = 0;

void setup() {
    Serial.begin(115200);
    setupWiFi();
    setupRtdb();
    configTime(0, 0, "pool.ntp.org");
    //Firebase.RTDB.setString(&fbdo, "test", "hello world");// TEST CODE
}

void loop() {
  milisecTime = millis();
  if((milisecTime - lastSensorUpdateTime) >= SECONDS_BETWEEN_SENSOR_READING * 1000){ //
    int volumePercentage = measureVolumePercentage();
    lastVolumePercent[LVPPointer] = volumePercentage;

    lastSensorUpdateTime = milisecTime;
    LVPPointer += 1;
  }

  if (LVPPointer == READINGS_BETWEEN_DB_UPDATE){
      LVPPointer = 0;
      if(Firebase.ready() && signupOK){
        int volPercentTotal = 0;
        for (int i=0;i<READINGS_BETWEEN_DB_UPDATE;i++){
            volPercentTotal += lastVolumePercent[i];
        }
        int volPercentAvg = volPercentTotal/READINGS_BETWEEN_DB_UPDATE;
        sendDataToRtdb(volPercentAvg);

        if((milisecTime - lastHistoryUpdate) >= SECONDS_BETWEEN_HISTORY_UPDATE * 1000){//update history
          updateRtdbHistory(volPercentAvg);
        }
      }
  }

}
void sendDataToRtdb(int volumePercentage){
  if(Firebase.RTDB.setInt(&fbdo, "id/" UNIT_ID "/status/percentFilled", volumePercentage) &&
     Firebase.RTDB.setInt(&fbdo, "id/" UNIT_ID "/status/timestamp", getEpochTime())){

       Serial.print("DB updated:");
       Serial.println(volumePercentage);
  }
   else{
        Serial.println("Failed to update DB");
    }

  return;
}

void updateRtdbHistory(int volumePercentage){
    String time = String(getEpochTime());
    Serial.println("attempting to update history");
    if(Firebase.RTDB.setInt(&fbdo, "id/" UNIT_ID "/history/" + time, volumePercentage)){
        Serial.print("History updated:");
        Serial.println(volumePercentage);
    }
    else{
        Serial.println("Failed to update history");
    }

  return;
}


int measureVolumePercentage(){
  float totalSensorToObj = 0;
  int workingSensors = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      float SensorToObj = measureSensor(i,j);
      if (SensorToObj > 0 && SensorToObj < SENSOR_TO_GROUND_DIST+5){
        totalSensorToObj = totalSensorToObj + SensorToObj;
        workingSensors += 1;
      }
      Serial.print(String(SensorToObj, 2));
      Serial.print(" cm");
      if (j < 3) {
        Serial.print(" | ");
      }
    }
    Serial.println();
  }
  Serial.print("totalSensorToObj:");
  Serial.println(totalSensorToObj);


  float avgSensorToObj = totalSensorToObj/workingSensors;
  Serial.print("avgSensorToObj:");
  Serial.println(avgSensorToObj);

  float avgObjectHeight = SENSOR_TO_GROUND_DIST - avgSensorToObj;
  Serial.print("avgObjectHeight:");
  Serial.println(avgObjectHeight);

  int percent = avgObjectHeight/CONTAINER_HEIGHT * 100;
  Serial.print("Volume: ");
  Serial.println(percent);

  return percent;
}

float measureDistance(int sensorPin){
  delay(100);
  pinMode(sensorPin, OUTPUT);
  
  digitalWrite(sensorPin, LOW);
  delayMicroseconds(5);

  digitalWrite(sensorPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorPin, LOW);

  pinMode(sensorPin, INPUT);
  int duration = pulseIn(sensorPin, HIGH);
  return duration * 0.034 / 2;
}

float measureSensor(int i, int j) {
  return measureDistance(pin_sensor[i][j]);
}

// void printDistanceAll() {
//   Serial.println("Current Distance");
//   for (int i = 0; i < 3; i++) {
//     for (int j = 0; j < 3; j++) {
//       Serial.print(String(measureSensor(i,j), 2));
//       Serial.print(" cm");
//       if (j < 3) {
//         Serial.print(" | ");
//       }
//     }
//     Serial.println();
//   }
// }
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
