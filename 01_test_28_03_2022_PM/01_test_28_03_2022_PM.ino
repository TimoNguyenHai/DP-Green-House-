#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h" 
#include <WiFi.h>
#include <HTTPClient.h>
#include "ThingSpeak.h"
#include <ESP_Mail_Client.h>

//const char* ssid = "TP-Link_8A70";
//const char* password = "42592051";
//const char* ssid = "WIAM GUEST";
//const char* password = "yK64!8VGTYiF";
const char* ssid = "HangTrieu 2.4GHz";
const char* password = "hoilamgi";


//Reconnect to Wifi parameters
unsigned long previousMillis = 0;
unsigned long interval = 20000;
//Alarm parameter
unsigned long intervalAlarm = 900000;
//constants for moisture sensor calibration
const int airValue = 3300;   //you need to replace this value with Value_1
const int waterValue = 1450;  //you need to replace this value with Value_2

//Thingspeak
const char* myWriteAPIKey = "SYNHQPWXULTVK24T"; //ThingSpeak API Key
unsigned long myChannelNumber = 1686802; //ThingSpeak channel number


/*Defining the pin on ESP32*/
#define SENSOR_PIN 34
#define ONE_WIRE_BUS 33
#define LIGHT_SENSOR_PIN 32
#define RELAY_PIN_PUMP 26 
#define BUTTON_PIN 27
/*Defining the port and host for smtp.gmail.com*/
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define AUTHOR_EMAIL "mailsendergreenhouse@gmail.com"
#define AUTHOR_PASSWORD "mutjbxonrrsczoro"
/* Sending Emails to: */
#define RECIPIENT_EMAIL "timomailnahry@gmail.com"

/*Create names for objects.*/
WiFiClient client;
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
Adafruit_BME680 bme;

// define variables
int getValue;
int soilMoistureValue;
int soilMoisturePercent = 0;
bool pumpState;

float temperatureBME680;
//Sending Flag
bool sendingFlag = true;
//Email Error Flag
bool emailFlag = true; //Spravene 2 timere ale ako to funguje?
//test valuate
float SensorValue[5];
//int count = 0;
//float temperatureBME680;

//Timer
volatile int interruptCounter;
int totalInterruptCounter;
 
hw_timer_t * timer0 = NULL;
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t * timer1 = NULL;
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

void sendSMTP(String stringAlarm)
{
//Sending EMAILS
/** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1 
   */
smtp.debug(0);

/* Declare the session config data */
 ESP_Mail_Session session;

/* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

/* Declare the message class */
   SMTP_Message message;

/* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Test Email";
  message.addRecipient("Timo", RECIPIENT_EMAIL);

  String stringOne = "Hi, Timo!";
  String stringTwo = ("I think you should check the GreenHouse! " + stringAlarm + " This Value is abnormal!"+ "\n");
  String stringThree = "- Sent from ESP board - TEST VERSION";
  String stringFour= "Here are the parameters of the GreenHouse!:";
  String stringFinal = (stringOne + "\n"  + stringTwo + "\n" + stringThree + "\n" + stringFour + "\n" + "\n" + "Temperature BME680:" + " " + (bme.temperature) + "C" + "\n" + "Pressure BME680:" + " " + (bme.pressure / 100.0) + "hPa" + "\n" + "Humidity BME680:" + " " + (bme.humidity) + "%" + "\n" + "Gas BME680:" + " " + (bme.gas_resistance / 1000.0) + "kOhm" + "\n" + "Soil Moisture:" + " " + soilMoisturePercent + "%"+ "\n" + "Pump State" + " " + pumpState + " ");
  String textMsg = stringFinal;
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
    
  /*Send HTML message
   String htmlMsg =("<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p> Sent from ESP board<br>"soilMoisturePercent"<br>City, State 00000</p></div>");
   message.html.content = htmlMsg.c_str();
   message.text.charSet = "us-ascii";
   message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
   */
/* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;
/* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
   Serial.println("Error sending Email, " + smtp.errorReason());
}

void evaluateError()
 {
  //SensorValue[count] = temperatureBME680;
  //count = count+1; 
 }

void IRAM_ATTR onTimer0(){
  // Critical Code here
  portENTER_CRITICAL_ISR(&timerMux0);
  emailFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux0);
}


void IRAM_ATTR onTimer1(){
  // Critical Code here
  portENTER_CRITICAL_ISR(&timerMux1);
  sendingFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux1);
}

bool getValueJson(String URL) 
{  
// Get manual flag value from Thingspeak   
   HTTPClient http;
   http.begin(URL); //Specify the URL
   
   int httpCode = http.GET();
   if (httpCode > 0) 
    { //Check for the returning code
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        //Serial.println(payload);
        //JsonObject root = doc.as<JsonObject>();
      //channel
        //JsonObject root = doc["channel"];
        /*key of the Object
          int id = root["id"];
          Serial.println(id);
        */
     //feeds
        JsonArray arrayValue = doc["feeds"];
        JsonObject objectValue = arrayValue[0];
        int getValue = objectValue["field1"];
        return getValue;
     }
   else 
    {  Serial.println("Error on HTTP request");
        return false;
    }
   http.end(); //Free the resources
}

void bmeStart()
{
 // BME680 starting
      unsigned long endTime = bme.beginReading();
      if (endTime == 0) {
         Serial.println(F("Failed to begin reading!"));
         return;
      }
      if (!bme.endReading()) {
         Serial.println(F("Failed to complete reading!"));
         return;
      }
}      

int moistureSensor()
{
  //Moisture sensor
    soilMoistureValue = analogRead(SENSOR_PIN);
    soilMoisturePercent = map(soilMoistureValue, airValue, waterValue, 0, 100);
    if(soilMoisturePercent > 100)
      {
      soilMoisturePercent = 100;
      }
    else if(soilMoisturePercent < 0)
      {
      soilMoisturePercent = 0;
      }
   return soilMoisturePercent;
}


bool alarmEmailSender(int a, bool b)
   // Email if error
{ 
     if (a > 90 || a < 20 || b == 0)
      { 
         return true;
      }
      else 
      {  
        return false;
      }
}

bool tankLevelSwitch()
{
  // Switch to show position of the tank
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  int lastState = digitalRead(BUTTON_PIN); //Tank level High/Low
  if (lastState == HIGH) 
  {
    return true;
  }
  else
  {
    return false;
  }
}

void pumpOn()
{
  // Pump Switch Off
  digitalWrite(RELAY_PIN_PUMP, LOW);
}
void pumpOff()
{
  // Pump Switch On
  digitalWrite(RELAY_PIN_PUMP, HIGH);
}

bool pumpControl(int x,bool y, bool z, bool j)
{ 
  
  String param;
  String stringAlarm;
  // Decided if pump is on or off. Parameters: x = soilMoisturePercent;y = getValue; z = tankLevelSwitch ; j = ON/OFF manual pump
  if (x > 90 || z == 0) //Alarm active, Pump off bcs of safety. Either moisture is high or level in the tank is low
       {  
          // x = soilMoisturePercent;y = getValue y =  z = tankLevel;
          if (x > 90)
          {
            if (x > 90 && y == 1)
            {
             param = "Soil Moisture Percent: ";
             stringAlarm = (param + x + "%." + "Careful! The pump is on manual, you should turn it off!");
             //sendSMTP(stringAlarm);
             return false;
            }
            else
            {
             param = "Soil Moisture Percent: ";
             stringAlarm = (param + x + "%.");
             //sendSMTP(stringAlarm);
             return false;
            }
          }
          else
          {
          param = "Water Level Tank is LOW =  ";
          stringAlarm = (param + z);
          //sendSMTP(stringAlarm);
          return false;
          }
       }
    else if (x <= 90 && y == 1 && x > 15)
       {  
          if (x <= 90 && y == 1 && x > 15 && j > 0) //turn on manually
          {
          return true;
          }
          else if (x <= 90 && y == 1 && x > 15 && j == 0) //turn off manually
          {
          return false;
          }
       }
   else
    {   
      if  (x <= 15 && y == 1)
        {
          if  (x <= 15 && y == 1 && j == 0)
          { 
            param = "Soil Moisture Percent: ";
            stringAlarm = (param + x + "%." + "Careful! The pump is on manual and it is OFF, you should turn it on!");
            //sendSMTP(stringAlarm);
            return false;
          }
          else if (x <= 15 && y == 1 && j > 0)
          {
            param = "Soil Moisture Percent: ";
            stringAlarm = (param + x + "%." + "Careful! The pump is on manual and it is ON, not working!");
            //sendSMTP(stringAlarm);
            return true;
          }
        } 
      else
          {
             if (x <= 30 && y == 0) 
             {
               if (x <= 15 && y == 0)
                {
                  param = "Soil Moisture Percent: ";
                  stringAlarm = (param + x + "%.");
                  //sendSMTP(stringAlarm);
                  return true;
                }
              }
              else if (x >= 55 && y == 0)
              {
                return false;
              }
          }
     }
}


int lightSensor()
{
  float volts = analogRead(LIGHT_SENSOR_PIN) * 3.3 / 4096.0;
  float amps = volts / 10000.0;
  float microamps = amps * 1000000;
  float lux = microamps * 2.0;
  return lux;
}



void setup(void)
{
  Serial.begin(115200);

  timer0 = timerBegin(0, 80, true); 
  timerAttachInterrupt(timer0, &onTimer0, true); 
  timerAlarmWrite(timer0, 17000000, true);
  timerAlarmEnable(timer0); // enable;

  timer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer1, &onTimer1, true);  
  timerAlarmWrite(timer1, 16000000, true);
  timerAlarmEnable(timer1);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // init ThingSpeak
  ThingSpeak.begin(client);
  //Relay  
  pinMode(RELAY_PIN_PUMP, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Start up the library
  while (!Serial);
  Serial.println(F("BME680 async test"));

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }
  
  bme.setTemperatureOversampling(BME680_OS_8X); //reads temperature in Celsius
  bme.setHumidityOversampling(BME680_OS_2X);   //reads absolute humidity
  bme.setPressureOversampling(BME680_OS_4X);   //reads pressure in hPa (hectoPascal = millibar)
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3); //The BME680 sensor integrates an internal IIR filter to reduce short-term changes in sensor output values caused by external disturbances.
  bme.setGasHeater(320, 150); // 320*C for 150 ms. The BME680 contains a MOX (Metal-oxide) sensor that detects Volatile Organic Compounds (VOC) like ethanol and carbon monoxide. in the air.
  /*
   Sensor: Accuracy
   Pressure: +/- 1 hPa
   Humidity: +/- 3 %
   Temperature: +/- 1.0 ºC
   */
}

void loop(void)
{     
  unsigned long currentMillis = millis();
  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED)
  {
   //Start the BME680
    bmeStart();

   //Flag Manual Value
    String url_PumpFlag = ("https://api.thingspeak.com/channels/1686803/feeds.json?api_key=COJG4GDTZ0GB5QXC&results=1");
    bool pumpFlag = getValueJson(url_PumpFlag);

   //Flag Manual Value
    String url_ManualPump = ("https://api.thingspeak.com/channels/1686806/feeds.json?api_key=INMEFND2RBOJZFWD&results=1");
    bool manualPump = getValueJson(url_ManualPump);
        
   //Moisture sensor
    soilMoisturePercent = moistureSensor();

   // Tank Level Switch
    bool tankLevel = tankLevelSwitch();
    
   // LIGHT SENSOR
    float lightValue = lightSensor(); //Lowest 659  
    
   //Pump Control
    bool pumpState = pumpControl(soilMoisturePercent,pumpFlag,tankLevel,manualPump);
    if (pumpState == true)
    {
      pumpOn();
    }
    else
    {
      pumpOff();
    }
   
   //Email if error
   //alarmEmailSender();
   /*
   bool sendingEmail =  alarmEmailSender(soilMoisturePercent,tankLevel);
   Serial.println("sendingEmail:");
   Serial.println(sendingEmail);

   Serial.println("currentMillis:");
   Serial.println(currentMillis);
   
   Serial.println("previousMillis:");
   Serial.println(previousMillis);
   
   if ((sendingEmail == true) && (currentMillis - previousMillis >= intervalAlarm));
   {
      sendSMTP();
      currentMillis = previousMillis;
   }
   */
   
   // Timer ended - prepare the values for thingspeak
   if(sendingFlag == true)
   {
   //Print JsonObject in Serial Monitor
    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();     
         root["temperatureBME680"] = bme.temperature;
         root["pressureBME680"] = bme.pressure / 100.0;
         root["humidityBME680"] = bme.humidity;
         root["gasBME680"] = bme.gas_resistance / 1000.0;  
         root["soilMoisturePercent"]= soilMoisturePercent;
         root["pumpState"] = pumpState;
         root["Pump Flag Value"] = pumpFlag;
         root["Manual Pump Value"] = manualPump;
         root["Light Value"] = lightValue;
    serializeJsonPretty(doc, Serial);
    Serial.println();

   //Calculate the parameters of BME680
    float temperatureBME680 = bme.temperature;
    float pressureBME680 = bme.pressure / 100.0;
    float humidityBME680 = bme.humidity;
    float gasBME680 = bme.gas_resistance / 1000.0;

  //Evaluating the error data
     // evaluateError();
     //Serial.println("sensor value array:");
     //Serial.println(soilMoistureValue);
          
  // Fields in ThingSpeak
         ThingSpeak.setField(1, temperatureBME680);
         ThingSpeak.setField(2, gasBME680);
         ThingSpeak.setField(3, humidityBME680);
         ThingSpeak.setField(4, pressureBME680);
         ThingSpeak.setField(5, soilMoisturePercent);
         ThingSpeak.setField(6, pumpState);
         ThingSpeak.setField(7, lightValue);
         
         String myStatus = ("Updating data");
         
 // set the status
         ThingSpeak.setStatus(myStatus);
 
 // write to the ThingSpeak channel
         int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
         if(x == 200)
         {
            Serial.println("Channel updated successfuly.");
         }
         else
         {
            Serial.println("Problem updating channel. HTTP error code " + String(x));
         } 
    sendingFlag = false;
   }
  }
  else
  {
 // Reconnect to Wifi  
 //if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval))
    {
            Serial.println(millis());
            Serial.println("Reconnecting to WiFi...");
            WiFi.disconnect();
            WiFi.reconnect();
            previousMillis = currentMillis;
    }
  }
}
 
