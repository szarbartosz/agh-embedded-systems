#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LiquidCrystal.h>
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include <Preferences.h>
#include "ESPAsyncWebServer.h"
 
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

#define ASCI_MOD 33

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

int numOfStations = 3;
int currentStationIndex = 0;
String fmStations[1000][2] = {{"RMF FM", "http://rs6-krk2.rmfstream.pl/rmf_fm"}, {"PLUS", "http://stream.plus.legnica.pl:8000/radioplus"}, {"RADIO KRAKOW", "http://stream3.nadaje.com:9116/radiokrakow"}};

String wifi_ssids[100] = {};
int wifi_no = 0;
 
String AP_SSID       = "ESP32-Web-Radio-Config";
String AP_PASSWORD   = "123456789";
 
String WIFI_SSID     = "";
String WIFI_PASSWORD = "";
String RADIO_STATION_URL = "";
String RADIO_STATION_NAME = "";
bool CONFIG_END = false;

Preferences preferences;

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
 
// WiFiServer server(80);
AsyncWebServer server(80);
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


void save_new_station(String station_name, String station_url){
  
  preferences.begin("stations_names", false);
  int i=0;
  for(i;i<numOfStations;i++){
    if(fmStations[i][0] == station_name)
      break;
  }
  if(i>=numOfStations){
    numOfStations++;
    preferences.putUInt("stations_count", numOfStations);
    preferences.putString("station_" + (char)(i+ASCI_MOD),station_name);
    preferences.end();

    preferences.begin("stations_urls", false);
    preferences.putString(station_name.c_str(), station_url);
    preferences.end();

    fmStations[i][0] = station_name;
    fmStations[i][1] = station_url;
  }else{
    preferences.end();

    preferences.begin("stations_urls", false);
    preferences.putString(station_name.c_str(), station_url);
    fmStations[i][1] = station_url;
    preferences.end();
  }
}


void load_saved_stations(){
  preferences.begin("stations_names", false);

  unsigned int stations_count = preferences.getUInt("stations_count", 0);
  numOfStations = stations_count;
  for(int i=0;i<stations_count;i++){
    fmStations[i][0] = preferences.getString("station_" + (char)(i+ASCI_MOD),"RMF FM");
  }

  preferences.end();

  preferences.begin("stations_urls", false);
  
  for(int i=0;i<stations_count;i++){
    fmStations[i][1] = preferences.getString(fmStations[i][0].c_str(),"http://rs6-krk2.rmfstream.pl/rmf_fm");
  }

  preferences.end();
}

void save_settings(){
  preferences.begin("settings", false);
  preferences.putUInt("stations_count", numOfStations);
  preferences.putUInt("current_station", currentScreenIndex);
  preferences.putString("wifi_SSID", WIFI_SSID);
  preferences.putString("wifi_passwd", WIFI_PASSWORD);
  preferences.end();
}

void load_settings(){
  load_saved_stations();
  preferences.begin("settings", false);
  numOfStations = preferences.getUInt("stations_count", 0);
  currentScreenIndex = preferences.getUInt("current_station", 0);
  WIFI_SSID = preferences.getString("wifi_SSID","");
  WIFI_PASSWORD = preferences.getString("wifi_passwd", "");
  
  String RADIO_STATION_URL = fmStations[currentStationIndex][1];
  String RADIO_STATION_NAME = fmStations[currentStationIndex][0];
  preferences.end();
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
}

void startServer(){
  server.on("/settings/radio", HTTP_GET, [] (AsyncWebServerRequest *request){
    Serial.print("ON SERVER - got request1");
    String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Settings form</title></head>"
                "<body>"
                ""
                "<h2>Web radio settings</h2>"
                ""
                "<form action=\"/action_page.php\">"
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
                            "var radio_url = document.getElementById(\"radio_url\").value;"
                            "var radio_name = document.getElementById(\"radio_name\").value;"
                            "window.location.href = `/settings/radio_new?radio_url=${radio_url}&radio_name=${radio_name}`;"
                            "document.getElementById(\"test\").innerHTML = \"Settings saved.\";"
                        "}"
                "</script>"
                "</body>"
                "</html>"
                "";
    request->send(200,"text/html",resp_str);
  });

  server.on("/select/radio", HTTP_GET, [] (AsyncWebServerRequest *request){
    Serial.print("ON SERVER - got request1");
    String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Settings form</title></head>"
                "<body>"
                ""
                "<h2>Web radio select</h2>"
                ""
                "<form action=\"/action_page.php\">"
                  "<label for=\"fname\">web radio station:</label><br>"
                  "<select id=\"station\" name=\"station\">";
                      for(int i=0;i<numOfStations;i++){
                        resp_str = resp_str + "<option value=\"" + fmStations[i][0] + "." + fmStations[i][1] + "\">" + fmStations[i][0] + "</option>";
                      }
                  resp_str = resp_str + "</select><br>"
                "</form>"
                "<button id=\"btnsave\" >Select</button>"
                    "<div id=\"response\"></div>"
                    "<p id=\"test\"></p>"
                ""
                "<script>"
                        "document.getElementById (\"btnsave\").addEventListener (\"click\", sendData, false);"
                        "function sendData()"
                        "{"
                            "var select = document.getElementById(\"station\");"
                            "var radio_string = select.options[select.selectedIndex].value.split(\'.\');"
                            "var radio_url = radio_string[1];"
                            "var radio_name = radio_string[0];"
                            "window.location.href = `/settings/radio_new?radio_url=${radio_url}&radio_name=${radio_name}`;"
                            "document.getElementById(\"test\").innerHTML = \"Settings saved.\";"
                        "}"
                    "</script>"
                "</body>"
                "</html>"
                "";
    request->send(200,"text/html",resp_str);
  });

  server.on("/settings/radio_new", HTTP_GET, [] (AsyncWebServerRequest *request){
    if (request->hasParam("radio_url") && request->hasParam("radio_name")) {
      RADIO_STATION_URL = request->getParam("radio_url")->value();
      RADIO_STATION_NAME = request->getParam("radio_name")->value();
      save_new_station(RADIO_STATION_NAME, RADIO_STATION_URL);
      save_settings();
    }else{
      request->send(400,"text/plain","Invalid request! Required radio name and radio url params!");
    }
    String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Successfully applied</title></head>"
                "<body>"
                ""
                "<h1>New radio connection configured</h1>"
                "<h2>Now you will connect to: " + RADIO_STATION_NAME + "<h2>"
                "<br><br>"
                "<a href=\"/\">Main page</a>"
                "</body>"
                "</html>"
                "";
    request->send(200,"text/html",resp_str);
  });
  server.on("/settings/wifi", HTTP_GET, [] (AsyncWebServerRequest *request){
    Serial.print("ON SERVER - got request1");
    String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Settings form</title></head>"
                "<body>"
                ""
                "<h2>Web radio settings</h2>"
                ""
                "<form action=\"/action_page.php\">"
                  "<label for=\"fname\">wifi SSID:</label><br>"
                  "<select id=\"ssid\" name=\"ssid\">";
                      for(int i=0;i<wifi_no;i++){
                        resp_str = resp_str + "<option value=\"" + wifi_ssids[i] + "\">" + wifi_ssids[i] + "</option>";
                      }
                  resp_str = resp_str + "</select><br>"
                  "<label for=\"lname\">wifi password:</label><br>"
                  "<input type=\"text\" id=\"passwd\" name=\"passwd\" value=\"\"><br>"
                "</form>"
                "<button id=\"btnsave\" >Register</button>"
                    "<div id=\"response\"></div>"
                    "<p id=\"test\"></p>"
                ""
                "<script>"
                        "document.getElementById (\"btnsave\").addEventListener (\"click\", sendData, false);"
                        "function sendData()"
                        "{"
                            "var wifi_passwd = document.getElementById(\"passwd\").value;"
                            "var select = document.getElementById(\"ssid\");"
                            "var wifi_ssid = select.options[select.selectedIndex].value;"
                            "window.location.href = `/settings/wifi_new?wifi=${wifi_ssid}&passwd=${wifi_passwd}`;"
                            "document.getElementById(\"test\").innerHTML = \"Settings saved.\";"
                        "}"
                    "</script>"
                "</body>"
                "</html>"
                "";
    request->send(200,"text/html",resp_str);
  });

  server.on("/settings/wifi_new", HTTP_GET, [] (AsyncWebServerRequest *request){
    if (request->hasParam("wifi") && request->hasParam("passwd")) {
      WIFI_SSID = request->getParam("wifi")->value();
      WIFI_PASSWORD = request->getParam("passwd")->value();
      save_settings();
    }else{
      request->send(400,"text/plain","Invalid request! Required wifi and passwd url params!");
    }
    String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Successfully applied</title></head>"
                "<body>"
                ""
                "<h1>New wifi connection configured</h1>"
                "<h2>Now you will connect to: " + WIFI_SSID + "<h2>"
                "<br><br>"
                "<a href=\"/\">Main page</a>"
                "</body>"
                "</html>"
                "";
    request->send(200,"text/html",resp_str);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String resp_str =  "<!DOCTYPE html>"
                "<html>"
                    "<head><title>Main page</title></head>"
                "<body>"
                ""
                "<h1>Your current configuration</h1>"
                "<h2>WI-FI network: " + WIFI_SSID + "</h2>"
                "<h2>Radio: " + RADIO_STATION_NAME + " url: "+ RADIO_STATION_URL + "</h2>"
                "<br><br>"
                "<a href=\"/settings/radio\">New radio</a><br>"
                "<a href=\"/select/radio\">Select radio</a><br>"
                "<a href=\"/settings/wifi\">Wi-Fi settings</a><br>"
                "<a href=\"/ok\"><h3>CONFIRM</h3></a><br>"
                "</body>"
                "</html>"
                "";
    request->send(200, "text/html", resp_str);
  });
  server.on("/ok", HTTP_GET, [](AsyncWebServerRequest *request){
    save_settings();
    CONFIG_END = true;
    request->send(200, "text/plain", "Restarting..");
  });

  server.begin();
    while(!CONFIG_END){
      Serial.print("waiting...");
      delay(2000);
    }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  load_saved_stations();
  load_settings();
  listNetworks();
 
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
  // waitForConfiguration();
  startServer();
  Serial.println("Configuration finished, setting up connection");
 
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
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
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    Serial.println("Couldn't get a wifi connection");
    while (true);
  }
 
  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);
  wifi_no = numSsid;
 
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    wifi_ssids[thisNet] = WiFi.SSID(thisNet);
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
