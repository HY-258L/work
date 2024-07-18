#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <U8g2lib.h>
#include <TimeLib.h> 
#include <SoftwareSerial.h>

const char* whost = "api.seniverse.com";
const char* chost = "api.openai.com";
const int httpPort = 80;
const char* WIFINAME = "your wifi name";
const char* WIFIPASSWORD = "your wifi password";
int temperature_int;
String text_String;
int code_int;

String reqUserKey = "api key";
String reqLocation = "city name";
String reqUnit = "c"; //c or f  
bool delayFlag = true;
char *icon_index[4] = { (char *)"@", (char *)"A", (char *)"C", (char *)"E" }; //defalut weather display code
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
typedef struct
{
  int year;
  int mon;
  int day;
  int hour;
  int min;
  int sec;
  int week;
} ntp_time;

typedef struct
{
  String weather;
  int iconIdx;
  int temp;
} xinzhiWeather;

WiFiClient Client;
U8G2_SH1106_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE); //SH1106 depends on oled drive
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void connectWifi(const char* wifiName, const char* wifiPassword, int waitTime) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(wifiName, wifiPassword);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.printf("connecting...%ds\r\n", count++);
    if (count >= waitTime) {
      Serial.println("connection failed");
      return;
    }
  }
  Serial.printf("connected to %s!!!\r\n local IP %s\r\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void getWeather() {
  if (weatherDelay()) {
  String reqWeather = "/v3/weather/now.json?key=" + reqUserKey +
                    + "&location=" + reqLocation + 
                    "&language=en&unit=" + reqUnit;
  String httpRequest = String("GET ") + reqWeather + " HTTP/1.1\r\n" + 
                              "Host: " + whost + "\r\n" + 
                              "Connection: close\r\n" +
                              "\r\n";                      
  Serial.print("Connecting to ");
  Serial.print(whost);

  if (Client.connect(whost, 80)){
    Serial.println("\r\n Success!");
 
    Client.print(httpRequest);
    Serial.println("Sending request");

    String weather_response = Client.readStringUntil('\n');
    Serial.print("weather_response: ");
    Serial.println(weather_response);

    if (Client.find("\r\n\r\n")) {
      Serial.println("Found Header End. Start Parsing.");
    }
    DynamicJsonDocument doc(1024);

    deserializeJson(doc, Client);
  
    JsonObject results = doc["results"][0];
    const char* name = results["location"]["name"];
    const char* country = results["location"]["country"];
    const char* path = results["location"]["path"];
  
    JsonObject now = results["now"];
    const char* text = now["text"];
    const char* code = now["code"];
    const char* temperature = now["temperature"];
  
    const char* last_update = results["last_update"];

    String name_String = results["location"]["name"].as<String>();
    int temperature_int = results["now"]["temperature"].as<int>();
    String text_String = now["text"].as<String>();
    String text_date = results["last_update"].as<String>();
    int code_int = now["code"].as<int>();  
    String text_time = text_date.substring(text_date.indexOf("T") + 1, text_date.indexOf("+"));
    text_date = text_date.substring(0, text_date.indexOf("T"));

    Serial.println(name_String);
    Serial.println(temperature_int);
    Serial.println(text_String);
    Serial.println(text_date);
    Serial.println(text_time);
//    OLED_Display(name_String, temperature_int, text_String, text_date, text_time);
  }   
  else {
    Serial.println("connection failed!");
  }   
  Client.stop(); 
  }
}

void initOLED() {
  u8g2.begin();
  u8g2.enableUTF8Print();                  //enable UTF-8 print
  u8g2.setFont(u8g2_font_wqy14_t_gb2312);  //set font as GB2312, Chinese character

  u8g2.clearBuffer();
  u8g2.setCursor(0, 14);
  u8g2.print("等待WiFi连接");
  u8g2.setCursor(0, 30);
  u8g2.print("WiFi connection...");
  u8g2.sendBuffer();
}

void my_strcat(char *srcstr, int t) {
  char str_tmp[5];
  if (t < 10) {
    strcat(srcstr, "0");
  }
  itoa(t, str_tmp, 10);
  strcat(srcstr, str_tmp);
}

// show date, time and weather information
void testShowTimeAndWeather(ntp_time now_time, xinzhiWeather weather_info) {
  u8g2.clearBuffer();
  int year = now_time.year;
  int month = now_time.mon;
  int day = now_time.day;
  int hour = now_time.hour;
  int minute = now_time.min;
  int sec = now_time.sec;
  int week = now_time.week;

  //hr + min
  char str_big_time[20] = "";
  my_strcat(str_big_time, hour);
  strcat(str_big_time, ":");
  my_strcat(str_big_time, minute);
  u8g2.setFont(u8g2_font_logisoso24_tf);
  u8g2.drawStr(0, 30, str_big_time);

  //sec
  char str_small_sec[20] = "";
  my_strcat(str_small_sec, sec);
  u8g2.setFont(u8g2_font_wqy14_t_gb2312);
  u8g2.drawStr(73, 30, str_small_sec);

  //date
  char str_date[20] = "";
  char str_temp[6];
  itoa(year, str_temp, 10);
  strcat(str_date, str_temp);
  strcat(str_date, "-");
  my_strcat(str_date, month);
  strcat(str_date, "-");
  my_strcat(str_date, day);
  u8g2.drawStr(0, 47, str_date);

  u8g2.setCursor(0, 63);
  u8g2.print("星期");
  switch (week) {
    case 1: u8g2.print("日"); break;
    case 2: u8g2.print("一"); break;
    case 3: u8g2.print("二"); break;
    case 4: u8g2.print("三"); break;
    case 5: u8g2.print("四"); break;
    case 6: u8g2.print("五"); break;
    case 7: u8g2.print("六"); break;
    default: break;
  }
  u8g2.setCursor(60, 63);
  u8g2.print(reqLocation);     //show your location

  u8g2.drawLine(90, 0, 90, 63);

  //weather
  if (!(weather_info.iconIdx < 0 || weather_info.iconIdx > 3)) {
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);
    u8g2.drawStr(96, 34, icon_index[weather_info.iconIdx]);
  }
  char temperature_tmp[25];
  itoa(weather_info.temp, temperature_tmp, 10);
  strcat(temperature_tmp, "℃");
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  u8g2.setCursor(96, 55);
  u8g2.print(temperature_tmp);

  u8g2.sendBuffer();

  Serial.println(now_time.mon);
}

void updateShowTime() {
  ntp_time now_time;
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  now_time.year = ptm->tm_year+1900;
  now_time.mon = ptm->tm_mon+1;
  now_time.day = ptm->tm_mday;
  now_time.hour = timeClient.getHours();
  now_time.min = timeClient.getMinutes();
  now_time.sec = timeClient.getSeconds();
  now_time.week = 1 + timeClient.getDay();

  xinzhiWeather weather_info;
  weather_info.weather = text_String;
  weather_info.iconIdx = code_int;
  weather_info.temp = temperature_int;

  Serial.println(now_time.year);

  testShowTimeAndWeather(now_time, weather_info);
}

bool weatherDelay() {
  unsigned long countsec = millis();
  if(countsec > 900000) {
    countsec = countsec - 900000;
    delayFlag = !delayFlag;
  }
  return delayFlag;
}

void audioPlay(filename) {
  audio.setPinout(###################GPIO_BCLK, GPIO_LRC, GPIO_DOUT);
  audio.setVolume(#########);
  audio.connecttoFS(SD, "filename");
  audio.
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  initOLED();
  i2sInit();
  connectWifi(WIFINAME, WIFIPASSWORD, 10);
  timeClient.begin();
  timeClient.setTimeOffset(28800);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI disconnected, reconnecting...");
    connectWifi(WIFINAME, WIFIPASSWORD, 10);
  }

  getWeather();
  //Chatgpt(reqChatgpt);
  updateShowTime();
  delay(3000);
}