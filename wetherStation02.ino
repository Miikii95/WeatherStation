#include "DHT.h"
#include <Arduino.h>
#include <BMx280I2C.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <WiFiClient.h> 
#include <ESP32_FTPClient.h>

#define DHTPIN 4 
#define DHTTYPE    DHT22   
#define I2C_ADDRESS 0x76
BMx280I2C bmx280(I2C_ADDRESS);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(4);


#define WIFI_SSID "Asus"
#define WIFI_PASS "krzysiek16rt"

char ftp_server[] = "hosting2341454.online.pro";
char ftp_user[]   = "climateesp@miki-dev.pl";
char ftp_pass[]   = "esp32climate";

ESP32_FTPClient ftp (ftp_server,ftp_user,ftp_pass, 5000, 2);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

String sTime;


String smartInt2String(int input)
{
  if (input > 9)
    return String(input);
  else
    return "0"+String(input);
}

void dispData(float temp1,float temp2, float hum, float presure, int hour, int minute, int second)
{
  display.clearDisplay();
  display.setCursor(1,1);
  display.setTextSize(1);
  display.print("Temp: "); display.print(temp1); display.print(" / "); display.print(temp2); display.println(" C");
  display.print("Humi: "); display.print(hum); display.println(" %");
  display.print("Pres: "); display.print(presure); display.println(" hPa");

  if (hour<10) display.print(0);
  display.print(hour); 
  display.print(" : "); 
  if (minute<10) display.print(0);
  display.print(minute); 
  display.print(" : "); 
  if (second<10) display.print(0);
  display.println(second);

  display.display();
}

void setup() {

  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  Serial.begin(9600);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE); 
  display.setCursor(1,1);
  display.println("System");
  display.println("starting");
  display.println("...");
  display.display();

   pinMode(5, OUTPUT);
   delay(10);
/////////////////////////////////////////////////////////////////////////////////////
Wire.begin();
if (!bmx280.begin())
	{
		Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
		while (1);
	}
  	bmx280.resetToDefaults();
    bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
	  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     dht.begin();
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 WiFi.begin( WIFI_SSID, WIFI_PASS );
  Serial.println("Connecting Wifi...");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


////////////////////////////////////////////////////////////////////////////////////////


} //setup

float fPresure;
float h;
float t,t2,t1;
int minuteOfLastUpload = -6;

void loop() {

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  sTime = String(timeinfo.tm_year + 1900)+"-"+smartInt2String(timeinfo.tm_mon+1)+"-"+smartInt2String(timeinfo.tm_mday)+"T"+smartInt2String(timeinfo.tm_hour)+":"+smartInt2String(timeinfo.tm_min)+":"+smartInt2String(timeinfo.tm_sec)+"+01:00";
  Serial.println(sTime);

  if (!bmx280.measure())
	{
		Serial.println("could not start measurement, is a measurement already running?");
    return;
	}
do
	{
		delay(100);
	} while (!bmx280.hasValue());
  fPresure = bmx280.getPressure();
  	
	t2= bmx280.getTemperature();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 delay(100);

  h = dht.readHumidity();
  t1 = dht.readTemperature();
 
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
///////////////////////////
  t=t1+t2;
  t=t/2;
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("Preassure: "));
  Serial.print(fPresure);
  dispData(t1,t2,h,fPresure/100,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
 delay(100);
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////

      if(timeinfo.tm_min - minuteOfLastUpload>5 || timeinfo.tm_min - minuteOfLastUpload<0)
    {
        ftp.OpenConnection();  
          ftp.InitFile("Type A");
          ftp.NewFile("index.html");

          String sHtmlContent=" <!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><title>Online Weather Station</title><link rel=\"stylesheet\" href=\"style.css\" /></head><body><div class=\"table-container\"> <table><tr><td>" + smartInt2String(timeinfo.tm_hour) + " : " + smartInt2String(timeinfo.tm_min) + "</td><td>" + String(timeinfo.tm_mday) + "." + String(timeinfo.tm_mon+1) + "." + String(timeinfo.tm_year+1900) + "</td> </tr><tr><td>Temp1</td><td>" + String(t1) + "</td></tr><tr><td>Temp2</td><td>" + String(t2) + "</td></tr><tr><td>Hum:</td><td>" + String(h) + "</td></tr><tr><td>Press:</td><td>" + String(fPresure/100) + "</td></tr></table></div></body></html>";
         
          int l1 = sHtmlContent.length()+1;
          char cHtmlContent[l1];
          sHtmlContent.toCharArray( cHtmlContent, l1 );
          ftp.Write(cHtmlContent);
          ftp.Write("\n");
          ftp.CloseFile();
          ftp.CloseConnection();
          minuteOfLastUpload=timeinfo.tm_min;

            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(WHITE); 
            display.setCursor(1,1);
            display.println("FTP");
            display.println("UPLOADED");
            display.display();
            delay(3000);
          
    }
}//loop
