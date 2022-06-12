#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LiquidCrystal.h>
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// #define I2S_DOUT 25
// #define I2S_BCLK 27
// #define I2S_LRC 26
#define I2S_DOUT 15
#define I2S_BCLK 32
#define I2S_LRC 33

#define I2C_SDA 18
#define I2C_SCL 19

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
LiquidCrystal lcd(13, 12, 14, 27, 26, 25);

//Input & Button Logic
const int numOfInputs = 4;
const int inputPins[numOfInputs] = {0,5,23,21};
int inputState[numOfInputs];
int lastInputState[numOfInputs] = {LOW,LOW,LOW,LOW};
bool inputFlags[numOfInputs] = {LOW,LOW,LOW,LOW};
long lastDebounceTime[numOfInputs] = {0,0,0,0};
long debounceDelay = 2;
 
//LCD Menu Logic
const int numOfScreens = 2;
int currentScreenIndex = 0;
String screens[numOfScreens][2] = {{"Volume level", "vol"}, {"FM station", ""}};

int currentVolumeIndex = 10;
int volumeLevels[21] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};

const int numOfStations = 3;
int currentStationIndex = 0;
String fmStations[numOfStations][2] = {{"RMF FM", "http://rs6-krk2.rmfstream.pl/rmf_fm"}, {"PLUS", "http://stream.plus.legnica.pl:8000/radioplus"}, {"RADIO KRAKOW", "http://stream3.nadaje.com:9116/radiokrakow"}};

 
String AP_SSID       = "ESP32-Web-Radio-Config";
String AP_PASSWORD   = "123456789";
 
String WIFI_SSID     = "";
String WIFI_PASSWORD = "";
String RADIO_STATION_URL = "";
String RADIO_STATION_NAME = "";

void scrolltext(String text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(text.c_str());
  display.display();
  delay(100);
  display.startscrollleft(0x00, 0x0F);
}

void printScreen() {
  lcd.clear();
  lcd.print(screens[currentScreenIndex][0]);
  lcd.print(":");
  lcd.setCursor(0,1);
  if (currentScreenIndex == 0) {
    lcd.print(volumeLevels[currentVolumeIndex]);
  } else {
    lcd.print(fmStations[currentStationIndex][0]);
  }
  lcd.print(" ");
  lcd.print(screens[currentScreenIndex][1]);
}

void printInfo(String str1, String str2) {
  lcd.clear();
  lcd.print(str1);
  lcd.setCursor(0,1);
  lcd.print(str2);
}
 
WiFiServer server(80);
String header;
 
Audio audio;
 
IPAddress IP;
 
String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}
 
void createAP(){
  Serial.print("Setting AP (Access Point)…");
  scrolltext("Setting AP (Access Point)…");
  delay(500);
  WiFi.softAP(AP_SSID.c_str(), AP_PASSWORD.c_str());
  IP = WiFi.softAPIP();
  
  Serial.print("AP IP address: ");
  Serial.println(IP);
  printInfo("AP IP address:", IpAddress2String(IP));
  
  delay(500);
  server.begin();
}
 
void waitForConfiguration(){
  Serial.print("Waiting for configutration");
  scrolltext("WIFI: " + AP_SSID + ", PASSWD: " + AP_PASSWORD);
  delay(2000);
  while(WIFI_PASSWORD == "" || RADIO_STATION_URL == "" || WIFI_SSID == "" || RADIO_STATION_NAME == ""){
    scrolltext("Configure at: " + IpAddress2String(IP));
    printInfo("Configure at:", IpAddress2String(IP));
    WiFiClient client = server.available();   // Listen for incoming client
   
    while(!client){
      client = server.available();
      delay(500);
    }
   
    if (client) {                             // If a new client connects,
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
             
              // turns the GPIOs on and off
              if (header.indexOf("wifi=") >= 0) {
                Serial.println("wifi");
                int val_start = header.indexOf("wifi=");
                int val_end = val_start;
                while(header.length()>val_end && header[val_end]!='&'){
                  val_end++;
                }
                WIFI_SSID = header.substring(val_start+5,val_end);
                Serial.println(WIFI_SSID);
              }
              if (header.indexOf("passwd=") >= 0) {
                Serial.println("passwd");
                int val_start = header.indexOf("passwd=");
                int val_end = val_start;
                while(header.length()>val_end && header[val_end]!='&'){
                  val_end++;
                }
                WIFI_PASSWORD = header.substring(val_start+7,val_end);
                Serial.println(WIFI_PASSWORD);
              }
              if (header.indexOf("radio_url=") >= 0) {
                Serial.println("radio_url");
                int val_start = header.indexOf("radio_url=");
                int val_end = val_start;
                while(header.length()>val_end && header[val_end]!='&'){
                  val_end++;
                }
                RADIO_STATION_URL = header.substring(10+val_start,val_end);
                Serial.println(RADIO_STATION_URL);
              }
              if (header.indexOf("radio_name=") >= 0) {
                Serial.println("radio_name");
                int val_start = header.indexOf("radio_name=");
                int val_end = val_start;
                while(header.length()>val_end && header[val_end]!=' ' && header[val_end]!='&'){
                  val_end++;
                }val_end--;
                RADIO_STATION_NAME = header.substring(val_start+11,val_end);
                Serial.println(RADIO_STATION_NAME);
              }
 
              if(WIFI_PASSWORD != "" && RADIO_STATION_URL != "" && WIFI_SSID != "" && RADIO_STATION_NAME != "")
                return;

              String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Settings form</title></head>"
                "<body>"
                ""
                "<h2>Web radio settings</h2>"
                ""
                "<form action=\"/action_page.php\">"
                  "<label for=\"fname\">wifi SSID:</label><br>"
                  "<input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"\"><br>"
                  "<label for=\"lname\">wifi password:</label><br>"
                  "<input type=\"text\" id=\"passwd\" name=\"passwd\" value=\"\"><br>"
                  "<label for=\"radio_url\">web-radio url</label><br>"
                  "<input type=\"text\" id=\"radio_url\" name=\"radio_url\" value=\"http://rs6-krk2.rmfstream.pl/rmf_fm\"><br>"
                  "<label for=\"radio_name\">web-radio name</label><br>"
                  "<input type=\"text\" id=\"radio_name\" name=\"radio_name\" value=\"RMF FM\"><br><br>"
                "</form>"
                "<button id=\"btnsave\" >Register</button>"
                    "<div id=\"response\"></div>"
                    "<p id=\"test\"></p>"
                ""
                "<script>"
                        "document.getElementById (\"btnsave\").addEventListener (\"click\", sendData, false);"
                        "function sendData()"
                        "{"
                          
                            "var wifi_ssid = document.getElementById(\"ssid\").value;"
                            "var wifi_passwd = document.getElementById(\"passwd\").value;"
                            "var radio_url = document.getElementById(\"radio_url\").value;"
                            "var radio_name = document.getElementById(\"radio_name\").value;"
                            "window.location.href = `/settings?wifi=${wifi_ssid}&passwd=${wifi_passwd}&radio_url=${radio_url}&radio_name=${radio_name}`;"
                            "document.getElementById(\"test\").innerHTML = \"Settings saved.\";"
                        "}"
                    "</script>"
                "</body>"
                "</html>";
             
              // Display the HTML web pag
              client.println(resp_str);
                         
              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      // Clear the header variable
      header = "";
      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}
 
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
 
  lcd.begin(16, 2);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
 
  // Clear the buffer
  display.clearDisplay();
 
  createAP();
  waitForConfiguration();
  Serial.println("Configuration finished, setting up connection");
 
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  listNetworks();
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  while (WiFi.status() != WL_CONNECTED){
       
    delay(1500);
    Serial.println("Trying connect to wifi: " + WIFI_SSID + " with password: " + WIFI_PASSWORD);
    scrolltext("Trying connect to wifi: " + WIFI_SSID + " with password: " + WIFI_PASSWORD);
    printInfo("Connecting...", WIFI_SSID);
  }
 
  for(int i = 0; i < numOfInputs; i++) {
    Serial.write("write this");
    pinMode(inputPins[i], INPUT_PULLUP);
    digitalWrite(inputPins[i], HIGH);
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
  audio.connecttohost(RADIO_STATION_URL.c_str());
//    audio.connecttohost("http://rs6-krk2.rmfstream.pl/rmf_fm");
 
  scrolltext(RADIO_STATION_NAME);
  // scrolltext("RMF FM");
}
void loop()
{
  // Serial.write("write this");
  setInputFlags();
  resolveInputFlags();
  audio.loop();
  // Serial.println(volumeLevels[currentVolumeIndex]);
}
 
void listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a wifi connection");
    while (true);
  }
 
  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);
 
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.println(" dBm");
  }
}

void setInputFlags() {
  for(int i = 0; i < numOfInputs; i++) {
    int reading = digitalRead(inputPins[i]);
    if (reading != lastInputState[i]) {
      lastDebounceTime[i] = millis();
    }
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != inputState[i]) {
        inputState[i] = reading;
        if (inputState[i] == HIGH) {
          inputFlags[i] = HIGH;
        }
      }
    }
    lastInputState[i] = reading;
  }
}

void resolveInputFlags() {
  for(int i = 0; i < numOfInputs; i++) {
    if(inputFlags[i] == HIGH) {
      inputAction(i);
      inputFlags[i] = LOW;
      printScreen();
    }
  }
}

void parameterChange(int key) {
  if(key == 0) {
    if (currentScreenIndex == 0) {
      if (currentVolumeIndex < 20) {
        currentVolumeIndex++;
        audio.setVolume(volumeLevels[currentVolumeIndex]);
      }
    } else {
      if (currentStationIndex == numOfStations - 1) {
        currentStationIndex = 0;
        audio.connecttohost(fmStations[currentStationIndex][1].c_str());
      } else {
        currentStationIndex++;
        audio.connecttohost(fmStations[currentStationIndex][1].c_str());
      }
    }
  }else if(key == 1) {
    if (currentScreenIndex == 0) {
      if (currentVolumeIndex > 0) {
        currentVolumeIndex--;
        audio.setVolume(volumeLevels[currentVolumeIndex]);
      }
    } else {
      if (currentStationIndex == 0) {
        currentStationIndex = numOfStations - 1;
        audio.connecttohost(fmStations[currentStationIndex][1].c_str());
      } else {
        currentStationIndex--;
        audio.connecttohost(fmStations[currentStationIndex][1].c_str());
      }
    }
  }
}


void inputAction(int input) {
  if(input == 0) {
    if (currentScreenIndex == 0) {
      currentScreenIndex = numOfScreens-1;
    }else{
      currentScreenIndex--;
    }
  }else if(input == 1) {
    if (currentScreenIndex == numOfScreens-1) {
      currentScreenIndex = 0;
    }else{
      currentScreenIndex++;
    }
  }else if(input == 2) {
    parameterChange(0);
  }else if(input == 3) {
    parameterChange(1);
  }
}