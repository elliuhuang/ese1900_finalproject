/**
 * @file sketch.ino
 *
 * This Arduino sketch connects to Wi-Fi and sends a request to the OpenAI API
 * to generate a pattern for the built-in LED matrix. It then displays the pattern
 * on the LED matrix.
 *
 * Before using:
 * - Enter your Wi-Fi SSID and password.
 * - Create an OpenAI API key and enter it in the code.
 */

// To use ArduinoGraphics APIs, please include BEFORE Arduino_LED_Matrix
// #include "ArduinoGraphics.h"
#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include "ArduinoJson.h"
#include <LiquidCrystal.h>

// wifi credentials and openai api key
#include "secrets.h"

// WiFi credentials
// char ssid[] = SECRET_SSID;  // your network SSID (name)
// char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
char server[] = "api.openai.com";

WiFiSSLClient client;

String userRequest = "";

const String kins[] = {"Dwarf", "Fiery", "Goblin", "Human", "Knight of Yore", "Horned Beast", "Worm"};
const String traits[] = {"Lifting and pushing", "Singing and dancing", "Sneaking and hiding", "Listening and spotting", "Endurance and bravery", "Running and jumping"};
const String flaws[] = {"Blunt", "Forgetful", "Coward", "Naive", "Overconfident", "Selfish"};

// setup LCD display (liquid crystal)
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// define/setup rotary encoder
#define outputA 6 /* pin */
#define outputB 7 /* pin */
const int button = 9/* pin */;

int counter = 0;
int CurrentState;
int LastState;
int buttonState;

int colors[3] = {11, 1, 1}; // R, G, B values
int currentSelection = 0;   // 0 = Red, 1 = Green, 2 = Blue


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  pinMode (outputA,INPUT);
  pinMode (outputB,INPUT);
  pinMode(button, INPUT_PULLUP);

  Serial.begin (9600);
  // Reads the initial state of the outputA
  LastState = digitalRead(outputA);
  updateLCD();

  delay(2000);

  Serial.println("Connecting to wifi");
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // status = WiFi.begin(ssid, pass);
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(10000);  // wait 10 seconds for connection
  }

  printWifiStatus();

  

  
}



String getOpenAIResponse(int kinIndex, int traitIndex, int flawIndex) {
  WiFiSSLClient client;
  String response = "";

  kinIndex = encode(kinIndex);
  traitIndex -= 1;
  flawIndex -= 1;

  Serial.println(kins[kinIndex]);
  Serial.println(kinIndex);
  // Construct the message content
  String content = "Generate a 1 sentence description of a character in the Labyrinth game for the " +
                   kins[kinIndex] + " with the trait of " + traits[traitIndex] +
                   " and flaw of " + flaws[flawIndex];
  Serial.println(content);
  

  // Build JSON payload using ArduinoJson
  StaticJsonDocument<512> doc;

  doc["model"] = "gpt-4-1106-preview";
  doc["max_tokens"] = 2000;

  JsonArray messages = doc.createNestedArray("messages");
  JsonObject userMessage = messages.createNestedObject();
  userMessage["role"] = "user";
  userMessage["content"] = content;

  String requestBody;
  serializeJson(doc, requestBody);
  Serial.println("Sending request:");
  Serial.println(requestBody);

  // Connect to OpenAI API
  if (client.connect("api.openai.com", 443)) {
    Serial.println("Connected to OpenAI API server");

    // Send HTTP POST request
    client.println("POST /v1/chat/completions HTTP/1.1");
    client.println("Host: api.openai.com");
    // client.println("Authorization: Bearer " + String(openai_api_key));
    client.println("Authorization: Bearer " + String(OPEN_AI_KEY));
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(requestBody.length());
    client.println(); // end of headers
    client.println(requestBody); // send body

    // Read response
    while (client.connected()) {
      while (client.available()) {
        char c = client.read();
        response += c;
      }
    }
    // client.stop();

    // Serial.println("response complete!");
    // Serial.println(response);
    return response;

    
  } else {
    Serial.println("Failed to connect to OpenAI API server");
    return "Connection failed.";
  }
}

String getDescription(const String& response) {
  int contentIndex = response.indexOf("\"content\": \"");
  if (contentIndex != -1) {
    int start = contentIndex + 11; // move past the "content": "
    int end = response.indexOf("\"", start);
    
    if (end != -1) {
      String description = response.substring(start, end);
      Serial.println(description);  // Just print the clean reply
      return description;
    }
  }

  Serial.println("Failed to extract content from response.");
  return "Error: No content found.";
}


void loop() {
  // Read encoder state
  CurrentState = digitalRead(outputA);
  if (CurrentState != LastState) {
    if (digitalRead(outputB) != CurrentState) {
      if (currentSelection == 0) {
        colors[currentSelection] = max(11, colors[currentSelection] - 1);
      } else {
        colors[currentSelection] = max(1, colors[currentSelection] - 1);
      }
    } else {
      if (currentSelection == 0) {
        colors[currentSelection] = min(65, colors[currentSelection] + 1);
      } else {
        colors[currentSelection] = min(6, colors[currentSelection] + 1);
      }
    }
    updateLCD();
  }
  LastState = CurrentState;

  // Button press to move to next color
  if (digitalRead(button) == LOW) {
    delay(200); // Debounce delay
    currentSelection++;
    if (currentSelection > 2) {
      currentSelection = 0; // Reset to red
      String response = getOpenAIResponse(colors[0], colors[1], colors[2]);
      Serial.println("response success!");
      int contentIndex = response.indexOf("content") + 11;
      Serial.println(contentIndex);
      int end = response.indexOf("\"", contentIndex);
      Serial.println(end);
      String description = response.substring(contentIndex, end);
      Serial.println(description);
      // applyColor(); // Apply color to LED
      updateLCD(description);
    }
    updateLCD();
    while (digitalRead(button) == LOW); // Wait for release
  }
  
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

int encode(int value) {
  if (value == 16 || value == 26 || value == 36 || value == 46 || value == 56) return 6;
  else if (value >= 11 && value <= 16) return 0;
  else if (value >= 21 && value <= 26) return 1;
  else if (value >= 31 && value <= 36) return 2;
  else if (value >= 41 && value <= 46) return 3;
  else if (value >= 51 && value <= 56) return 4;
  else if (value >= 61 && value <= 65) return 5;
  else return 0; // Invalid input
}

void updateLCD() {
  lcd.clear();
  switch (currentSelection) {
    case 0: lcd.print("Set KIN: "); break;
    case 1: lcd.print("Set TRAIT: "); break;
    case 2: lcd.print("Set FLAW: "); break;
  }
  lcd.print(colors[currentSelection]);
  lcd.setCursor(0, 1);
  lcd.print("K:");
  lcd.print(colors[0]);
  lcd.print(" T:");
  lcd.print(colors[1]);
  lcd.print(" F:");
  lcd.print(colors[2]);
}

void updateLCD(String description) {
  lcd.clear();
  const int displayWidth = 16; 
  String padding = "                "; // 16 spaces
  String scrollText = padding + description + padding;
  
  unsigned long startTime = millis();
  while (millis() - startTime < 30000) {
    for (int i = 0; i <= scrollText.length() - displayWidth; i++) {
      lcd.setCursor(0, 0);
      lcd.print(scrollText.substring(i, i + displayWidth));
      delay(250);  // Adjust this value for faster or slower scrolling
      // if (millis() - startTime >= 60000) break;
    }
  }
  updateLCD();
}

