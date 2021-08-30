#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include <SD.h>
#include <SPI.h>


const char* ssid = "TC-A1ED4 2G";
const char* password = "hRCcu8kMf38R";

// Potential values for test: later not static
const char* sensor_id = "abc123";
const char* gateway_id = "123abc";
const char* station_name = "ESP Mosquito detector";

// JWT
char key[] = "tester";

// History file
const char *filename = "/test.jso";
const int chipSelect = 4;


unsigned long previousMillis = 0;
const long interval = 1000;

long aedes = 0;
long culex = 0;
long anopheles = 0;


ESP8266WebServer server(80);


void printFile() {
    // Open file for reading
    File file = SD.open(filename);
    if (!file) {
        Serial.println(F("Failed to read file"));
        return;
    }
 
    // Extract each characters by one by one
    while (file.available()) {
        Serial.print((char) file.read());
    }
    Serial.println();
 
    // Close the file
    file.close();
}

// Create json history
void createJsonHistory() {
  DynamicJsonDocument doc(1024);

  JsonObject obj;
  // Open file
  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Failed to create file, probably not exists"));
    Serial.println(F("Create an empty one!"));
    obj = doc.to<JsonObject>();
  } else {

    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      // if the file didn't open, print an error:
      Serial.println(F("Error parsing JSON "));
      Serial.println(error.c_str());

      // create an empty JSON object
      obj = doc.to<JsonObject>();
    } else {
      // GET THE ROOT OBJECT TO MANIPULATE
      obj = doc.as<JsonObject>();
    }

  }

  // close the file already loaded:
  file.close();

  obj[F("millis")] = millis();

  JsonArray data;
  // Check if exist the array
  if (!obj.containsKey(F("data"))) {
    Serial.println(F("Not find data array! Crete one!"));
    data = obj.createNestedArray(F("data"));
  } else {
    Serial.println(F("Find data array!"));
    data = obj[F("data")];
  }

  // create an object to add to the array
  JsonObject objArrayData = data.createNestedObject();

  objArrayData["prevNumOfElem"] = data.size();
  objArrayData["newNumOfElem"] = data.size() + 1;

  SD.remove(filename);

  // Open file for writing
  file = SD.open(filename, FILE_WRITE);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();

  // Print test file
  Serial.println(F("Print test file..."));
  printFile();

  delay(5000);
}

// Serving all detections
void getDetections() {
  DynamicJsonDocument doc(512);
  doc["sensor_id"] = sensor_id;
  doc["gateway_id"] = gateway_id;
  doc["station_name"] = station_name;
  doc["elapsed_time"] = millis() / 1000;

  // create an object to add to the array
  JsonArray detection_count_mosquitos;
  detection_count_mosquitos = doc.createNestedArray(F("detection_count_mosquitos"));
  JsonObject objArrayData = detection_count_mosquitos.createNestedObject();
  objArrayData["aedes"] = aedes;
  objArrayData["culex"] = culex;
  objArrayData["anopheles"] = anopheles;

  Serial.print(F("Stream..."));
  String buf;
  //serializeJson(doc, buf);
  serializeJsonPretty(doc, buf);

  server.send(200, F("application/json"), buf);
  Serial.print(F("done."));
}

// Serving specific detection
void getDetection() {
  DynamicJsonDocument doc(512);

  doc["millis"] = millis();
  if (server.arg("type") == "aedes")
    doc["aedes"] = aedes;
  else if (server.arg("type") == "culex")
    doc["culex"] = culex;
  else if (server.arg("type") == "anopheles")
    doc["anopheles"] = anopheles;
  else
    handleNotFound();

  Serial.print(F("Stream..."));
  String buf;
  //serializeJson(doc, buf);
  serializeJsonPretty(doc, buf);
  server.send(200, F("application/json"), buf);
  Serial.print(F("done."));
}

// reset detections
void resetDetections() {
  aedes = 0;
  culex = 0;
  anopheles = 0;
  server.send(200, F("text/html"), F("Done"));
}


// Define routing
void restServerRouting() {
  server.on("/", HTTP_GET, []() {
    server.send(200, F("text/html"),
                F("Welcome to the REST Web Server"));
  });
  server.on(F("/getDetections"), HTTP_GET, getDetections);
  server.on(F("/getDetection"), HTTP_GET, getDetection);
  server.on(F("/resetDetections"), HTTP_POST, resetDetections);
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");

  // Initialize SD library
  while (!SD.begin(chipSelect)) {
    Serial.println(F("Failed to initialize SD library"));
    delay(1000);
  }

  Serial.println(F("SD library initialized"));

  Serial.println(F("Delete original file if exists!"));
  SD.remove(filename);
}

void loop(void) {
  unsigned long currentMillis = millis();

  server.handleClient();

  // Data generator
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    aedes += 1;
    culex += 1;
    anopheles += 1;

    createJsonHistory();
  }

}
