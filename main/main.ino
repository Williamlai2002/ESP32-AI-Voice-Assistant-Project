// Wifi + API
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library for parsing JSON data

//OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//ESPNOW
#include <esp_now.h>

const char* ssid = "";
const char* password = "";
const char* Gemini_API = "";
const char* Gemini_Max_Tokens = "100";         // Define the maximum number of tokens for the Gemini API response
String res = "";                     // Initialize a string to store user input

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ESP-NOW peer address (receiver's MAC address)
uint8_t receiverAddress[] = {0xCC, 0xDB, 0xA7, 0x3F, 0xB6, 0x80};

typedef struct struct_message {
    char text[250];
} struct_message;

struct_message myData;

// Callback when data is sent
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void setup() {
  // put your setup code here, to run once:
  // connect to wifi
  Serial.begin(115200);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }
  Serial.println("\n ESP32 Is Now Connected To Wifi");
  Serial.println("Local ESP32 IP:");
  Serial.println(WiFi.localIP());

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextWrap(false);


   // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(onSent);

    // Register peer
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("");                // Print a new line on the serial monitor
  Serial.println("Ask your Question : ");  // Prompt the user to ask a question
  while (!Serial.available()); // Wait for user input on the serial monitor
  while (Serial.available()) {       // While there is data available on the serial monitor
    res = Serial.readString();
  }
  int len = res.length();            // Get the length of the 'res' string
  res = res.substring(0, (len - 1)); // Remove the last character from the 'res' string (usually a newline)
  res = "\"" + res + "\"";           // Add double quotes around the 'res' string
  Serial.println("");                // Print a new line on the serial monitor
  Serial.print("Asking Your Question : ");  // Print "Asking Your Question : " on the serial monitor
  Serial.println(res);               // Print the 'res' string (user's question) on the serial monitor

  if(WiFi.status() == WL_CONNECTED){
    HTTPClient https;
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(Gemini_API);
    https.begin(url);
    https.addHeader("Content-Type", "application/json");  // Add the Content-Type header to the HTTP request
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":" + res + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)Gemini_Max_Tokens + "}}");  // Create the JSON payload for the POST request
    int httpCode = https.POST(payload);
    // 200: Success
    // 404: Not Found
    // 500: Server Error
    Serial.println(httpCode);
    if (httpCode >0){
      String payload = https.getString();
      Serial.println("\nStatus Code:" + String(httpCode));
      Serial.println(payload);

      // Trim the Post Request 
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      String Answer = doc["candidates"][0]["content"]["parts"][0]["text"];
      Answer.replace("\n", " ");
      Serial.println(Answer);

      Answer.toCharArray(myData.text, 250);

            // Send data via ESP-NOW
            esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));
            if (result == ESP_OK) {
                Serial.println("Sent with success");
            } else {
                Serial.println("Error sending the data");
            }

      //Print on OLED
      displayquestion(res,50);
      displayanswer(Answer, 50);
      
      }
    else{
      Serial.println("Error on HTTPS Req");
    }
    https.end();
  }
  else{
    Serial.println("\No Wifi");
  }
  res = "";
}
void displayquestion(String question,int scrollSpeed){
    int textLength = question.length() * 6;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0); 
    display.print("You Asked:");
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 16); 
    display.print(question);
  //   while (true) {
  //     for (int16_t x = display.width(); x >= -textLength; x--) {
  //     display.fillRect(0, 16, display.width(), 8, BLACK); // Clear only the area where the scrolling text will appear
  //     display.setTextSize(1);
  //     display.setTextColor(WHITE);
  //     display.setCursor(x, 16); // Slightly below the "You Asked:" text
  //     display.print(question);
  //     display.display();
  //     delay(scrollSpeed);
  //   }
  // }
}
void displayanswer(String responce, int scrollSpeed){
      // display.setTextSize(1);
      // display.setTextColor(WHITE);
      // display.setCursor(0,40);
      // display.println("Responce:" + responce);
      // display.display();

      //enable scrolling if text longer than screen
      int textLength = responce.length() * 6;
      // Serial.print(textLength);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 32); // Adjust Y position as needed
      display.print("Response:");
      Serial.println("");                // Print a new line on the serial monitor
      Serial.println("Ask your Question : ");  // Prompt the user to ask a question
      for (int16_t x = display.width(); x >= -textLength; x--) {
      display.fillRect(0, 48, display.width(), 64, BLACK); // Clear only the area where the scrolling text will appear
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(x, 48); // Adjust Y position for response text
      display.print(responce);
      display.display();
      delay(scrollSpeed);
    }
}

