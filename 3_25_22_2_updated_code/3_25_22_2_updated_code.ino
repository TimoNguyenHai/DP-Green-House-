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
#include <stdio.h>

const char* ssid = "TP-Link_8A70";
const char* password = "42592051";

//const char* ssid = "WIAM GUEST";
//const char* password = "yK64!8VGTYiF";
//const char* ssid = "HangTrieu 2.4GHz";
//const char* password = "hoilamgi";

//Reconnect parameters
unsigned long previousMillis = 0;
unsigned long interval = 20000;
//Moisture sensor
const int airValueLeft = 3000;   //you need to replace this value with value of the sensor in the air
const int waterValueLeft = 800;  //you need to replace this value with value of the sensor in the water
const int airValueRight = 3006;  
const int waterValueRight = 800;
//Thingspeak
   //Write the sensor's parameters of the GreenHouse to the Thingspeak
const char* myWriteAPIKeyChannel1 = "70TO1SL3LFUPK53E"; //ThingSpeak API Key
unsigned long myChannelNumberChannel1 = 1560931; //ThingSpeak channel number
    //Write the actuator's parameters of the GreenHouse to the Thingspeak
const char* myWriteAPIKeyChannel2 = "DJFY3B2VFWGRIKCY"; //ThingSpeak API Key
unsigned long myChannelNumberChannel2 = 1657494; //ThingSpeak channel number

/*Defining the pin on ESP32*/
#define SENSOR_MOIST_LEFT 34
#define SENSOR_MOIST_RIGHT 25
#define ONE_WIRE_BUS 33
#define LIGHT_SENSOR_PIN 32
#define RELAY_PIN_PUMP  16
#define RELAY_PIN_VENT  26 
#define RELAY_PIN_LIGHT  17
#define RELAY_PIN_HEATER  18
#define LEVEL_PIN 27

//17
//18
//16
//26
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

// Define variables
int soilMoisturePercentLeft = 0;
int soilMoisturePercentRight = 0;
int lightLuxValue = 0;
  //States of the Actuators
bool pumpState;
bool lightState;
bool ventState;
bool heaterState;
  //BME680 values
float temperatureBME680;
float pressureBME680;
float humidityBME680;
float gasBME680;

int oldAverageMoisturePercentLeft;

//Sending Flag
bool sendingFlag = true;
//Email Error Flag
bool alarmFlag = true;
String param;
String stringAlarm;
//Valuate Error Flag
bool valuateFlag = true;
//test valuate
int values[5] = {};
int i,sum = 0;
int averageMoisturePercentLeft = 0;
// Mode in GreenHouse
int modeOne[7] = {30,55,200,23,25,16,18};
int modeEnvironment[7];


//Timer
volatile int interruptCounter;
int totalInterruptCounter;

hw_timer_t * timer0 = NULL;
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t * timer1 = NULL;
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t * timer2 = NULL;
portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED;

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
  String stringThree = "- Sent from ESP board - Miloslavov";
  String stringFour= "Here are the parameters of the GreenHouse!:";
  String stringFinal = (stringOne + "\n"  + stringTwo + "\n" + stringThree + "\n" + stringFour + "\n" + "\n" + "Soil Moisture Left Side:" + " " + soilMoisturePercentLeft + " %" + "\n" + "Soil Moisture Right Side:" + " " + soilMoisturePercentRight + " %" + "\n" + "Temperature BME680:" + " " + (bme.temperature) + " °C" + "\n" + "Pressure BME680:" + " " + (bme.pressure / 100.0) + " hPa" + "\n" + "Humidity BME680:" + " " + (bme.humidity) + "%" + "\n" + "Gas BME680:" + " " + (bme.gas_resistance / 1000.0) + " kOhm" + "\n" + "Temperature Soil:" + " " + sensors.getTempCByIndex(1) + " °C" + "\n" + "Temperature Water:" + " " + sensors.getTempCByIndex(0) + " °C" + "\n" + "Light Value:" + " " + lightLuxValue + " lux");
  String textMsg = stringFinal;
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

/* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;
/* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
   Serial.println("Error sending Email, " + smtp.errorReason());
}


void IRAM_ATTR onTimer0(){
  // Critical Code here
  portENTER_CRITICAL_ISR(&timerMux0);
  valuateFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux0);
}

void IRAM_ATTR onTimer1(){
  // Critical Code here
  portENTER_CRITICAL_ISR(&timerMux1);
  sendingFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux1);
}

void IRAM_ATTR onTimer2(){
  // Critical Code here
  portENTER_CRITICAL_ISR(&timerMux2);
  alarmFlag = true;
  portEXIT_CRITICAL_ISR(&timerMux2);
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

// MOISTURE PART

int moistureSensorLeft()
{
  //Moisture sensor
    int soilMoistureValue = analogRead(SENSOR_MOIST_LEFT);
    int soilMoisturePercent = map(soilMoistureValue, airValueLeft, waterValueLeft, 0, 100);
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

int moistureSensorRight()
{
  //Moisture sensor
    int soilMoistureValue = analogRead(SENSOR_MOIST_RIGHT);
    int soilMoisturePercent = map(soilMoistureValue, airValueRight, waterValueRight, 0, 100);
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



bool tankLevelSwitch()
{
  // Switch to show position of the tank
  pinMode(LEVEL_PIN, INPUT_PULLUP);
  int lastState = digitalRead(LEVEL_PIN); //Tank level High/Low
  if (lastState == HIGH) 
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool pumpControl(int x,bool y, bool z, bool j)
{ 
  // Decided if pump is on or off. 
  // Parameters: x = soilMoisturePercent; y = pumpManualFlag ; z = tankLevelSwitch ; j = ON/OFF Manual Pump
  if (x > 90 || z == 0) //Pump is off because of safety. Either moisture is high or level in the tank is low
       {  
          if (x > 90)
          {
            if (x > 90 && y == 1)
            {
             return false;
            }
            else if (x > 90 && y == 0)
            {
             return false;
            }
          }
          else //Level in tank is Low
          {
          return false;
          }
       }
  else if (x <= 90 && y == 1 && z == 1)
    {
       if  (x <= 90 && y == 1 && j == 0)
          { 
            return false;
          }
        else if (x <= 90 && y == 1 && j > 0)
          {
            return true;
          }
       } 
   else
      {
         if (x <= 30 && y == 0 && z == 1)
            {
              return true;
            }
          else if (x >= 55 && y == 0 && z == 1)
            {
              return false;
            }
       }
}

void pumpAlarmControl(int x,bool y, bool z, bool j)
{ 
  // Decided if pump is on or off. Parameters: x = soilMoisturePercent;y = getValue; z = tankLevelSwitch ; j = ON/OFF manual pump
  if (x > 90 || z == 0) //Alarm active, Pump off bcs of safety. Either moisture is high or level in the tank is low
       {  
          if (x > 90)
          {
            if (x > 90 && y == 1)
            {
             param = "Soil Moisture Percent: ";
             stringAlarm = (param + x + "%. " + "Careful! The pump is on manual, you should turn it off!");
             sendSMTP(stringAlarm);
            }
            else
            {
             param = "Soil Moisture Percent: ";
             stringAlarm = (param + x + "%.");
             sendSMTP(stringAlarm);
            }
          }
          else
          {
          param = "Water Level Tank is LOW =  ";
          stringAlarm = (param + z);
          sendSMTP(stringAlarm);
          }
       }
   else if  (x <= 20 && y == 1)
    {   
       if  (x <= 20 && y == 1 && j == 0)
          { 
            param = "Soil Moisture Percent: ";
            stringAlarm = (param + x + "%. " + "Careful! The pump is on manual and it is OFF, you should turn it on!");
            sendSMTP(stringAlarm);
          }
        else if (x <= 20 && y == 1 && j > 0)
          {
            param = "Soil Moisture Percent: ";
            stringAlarm = (param + x + "%. " + "Careful! The pump is on manual and it is ON, not working!");
            sendSMTP(stringAlarm);
          }
      } 
   else
     {
        if (x <= 20 && y == 0)
           {
             param = "Soil Moisture Percent: ";
             stringAlarm = (param + x + "%.");
             sendSMTP(stringAlarm);
            }
     }
 return;
}

// LIGHT CONTROL

int lightSensor()
{
  float volts = analogRead(LIGHT_SENSOR_PIN) * 3.3 / 4096.0;
  float amps = volts / 10000.0;
  float microamps = amps * 1000000;
  float lux = microamps * 2.0;
  return lux;
}

bool lightControl(int x,bool y, bool j)
{ 
  // Decided if light is on or off. 
  // Parameters: x = lightLuxValue; y = lightManualFlag ; j = ON/OFF Manual Light
  if (x > 100) //Light is off automaticly because of energy comsumtion.
       {  
            if (x > 100 && y == 1)
            {
             if (x > 100 && y == 1 && j > 0) //turn on manually
               {
                return true;
               }
             else if (x > 100 && y == 1 && j == 0) //turn off manually
                {
                 return false;
                }
            }
            else if (x > 100 && y == 0)
            {
             return false;
            }
       }
    else if (x <= 100 && y == 1)
       {  
          if (x <= 100 && y == 1 && j > 0) //turn on manually
          {
          return true;
          }
          else if (x <= 100 && y == 1 && j == 0) //turn off manually
          {
          return false;
          }
       }
    else
          {
             if (x <= 100 && y == 0) 
             {
                  return true;
             }
          }
}


void lightAlarmControl(int x,bool y, bool z,  bool j)
{ 
  // Decided if light is on or off. 
  // Parameters: x = lightLuxValue; y = lightManualFlag ; z = State of the light relay; j = ON/OFF Manual Light
  if (x > 100 && y == 1 && j > 0)
       { 
           param = "Light Value: ";
           stringAlarm = (param + x + "lux. " + "Careful! The light is on manual and it is ON. Should turn it off to save energy!");
           sendSMTP(stringAlarm);
        }
  else if (x <= 100 && y == 1 && j == 0)
       {  
          param = "Light Value: ";
          stringAlarm = (param + x + "lux. " + "Careful! The light is on manual and it is OFF. Should turn it on for the GreenHouse!");
          sendSMTP(stringAlarm);
       }
  else
          {
             if (x <= 100 && y == 0 && z == 1) 
             {
               param = "Light Value: ";
               stringAlarm = (param + x + "lux. " + "Careful! The light should be ON. But the light value is LOW. Should check the light for the GreenHouse!");
               sendSMTP(stringAlarm);
             }
          }
 return; 
}

// VENTILATION PART

  // VENT CONTROL

bool ventControl(int x,bool y, bool j)
{ 
  // Decided if vent is on or off. 
  // Parameters: x = temperatureBME680; y = ventManualFlag ; j = ON/OFF Manual Ventilation
  if (x > 25) //Ventilation is on to regulatate the temperature in the Greenhouse.
       {  
            if (x > 25 && y == 1)
            {
             if (x > 25 && y == 1 && j > 0) //turn on manually
               {
                return true;
               }
             else if (x > 25 && y == 1 && j == 0) //turn off manually
                {
                 return false;
                }
            }
            else if (x > 25 && y == 0)
            {
             return true;
            }
       }
 else if (x <= 25 && y == 1)
       {  
          if (x <= 25 && y == 1 && j > 0) //turn on manually
          {
          return true;
          }
          else if (x <= 25 && y == 1 && j == 0) //turn off manually
          {
          return false;
          }
       }
 else
       {
          if (x <= 23 && y == 0) 
          {
           return false;
          }
       }
}

  //Vent Alarm Control
void ventAlarmControl(int x,bool y, bool z,  bool j)
{ 
  // Parameters: x = temperatureBME680; y = ventManualFlag ; z = State of the ventilation relay; j = ON/OFF Manual Vent
  if (x > 25 && y == 1 && j == 0)
       { 
           param = "Temperature Air Value: ";
           stringAlarm = (param + x + "°C. " + "Careful! The ventilation is on manual and it is OFF. Should turn it on to regulate the temperature of the GreenHouse!");
           sendSMTP(stringAlarm);
        }
  else if (x <= 25 && y == 1 && j > 0 )
       {  
          param = "Temperature Air Value: ";
          stringAlarm = (param + x + "°C. " + "Careful! The ventilation is on manual and it is ON. Should turn it off to save energy!");
          sendSMTP(stringAlarm);
       }
  else
       {
        if (x > 40 && y == 0 && z == 1) 
         {
          param = "Temperature Air Value: ";
          stringAlarm = (param + x + "°C. " + "Careful! The ventilation should be ON. But the temperature value is HIGH. Should check the ventilation for the GreenHouse!");
          sendSMTP(stringAlarm);
         }
      }
 return;     
}

// HEATER PART

  // HEATER CONTROL

bool heaterControl(int x,bool y, bool j)
{ 
  // Decided if heater is on or off. 
  // Parameters: x = temperatureBME680; y = heaterManualFlag ; j = ON/OFF Manual Heater
  if (x <= 16) //Heater is on to regulatate the temperature in the Greenhouse.
       {  
            if (x <= 16 && y == 1)
            {
             if (x <= 16 && y == 1 && j > 0) //turn on manually
               {
                return true;
               }
             else if (x <= 16 && y == 1 && j == 0) //turn off manually
                {
                 return false;
                }
            }
            else if (x <= 16 && y == 0)
            {
             return true;
            }
       }
 else if (x > 16 && y == 1)
       {  
          if (x <= 16 && y == 1 && j > 0) //turn on manually
          {
          return true;
          }
          else if (x > 16 && y == 1 && j == 0) //turn off manually
          {
          return false;
          }
       }
 else
       {
          if (x >= 18 && y == 0) 
          {
           return false;
          }
       }
}

void heaterAlarmControl(int x,bool y, bool z,  bool j)
{ 
  // Parameters: x = temperatureBME680; y = heaterManualFlag ; z = State of the heater relay; j = ON/OFF Manual Heater
  if (x <= 16 && y == 1 && j == 0)
       { 
           param = "Temperature Air Value: ";
           stringAlarm = (param + x + "°C. " + "Careful! The heater is on manual and it is OFF. Should turn it on to regulate the temperature of the GreenHouse!");
           sendSMTP(stringAlarm);
        }
  else if (x >= 20 && y == 1 && j > 0 )
       {  
          param = "Temperature Air Value: ";
          stringAlarm = (param + x + "°C. " + "Careful! The heater is on manual and it is ON. Should turn it off to save energy!");
          sendSMTP(stringAlarm);
       }
  else
       {
        if (x < 5 && y == 0 && z == 1) 
         {
          param = "Temperature Air Value: ";
          stringAlarm = (param + x + "°C. " + "Careful! The heater should be ON. But the temperature value is LOW. Should check the heater for the GreenHouse!");
          sendSMTP(stringAlarm);
         }
      }
 return;     
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
  timerAlarmWrite(timer1, 15000000, true);
  timerAlarmEnable(timer1);

  
  timer2 = timerBegin(2, 80, true);
  timerAttachInterrupt(timer2, &onTimer2, true);  
  timerAlarmWrite(timer2, 480000000, true);
  timerAlarmEnable(timer2);
  
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
  //Relays  
  pinMode(SENSOR_MOIST_LEFT, OUTPUT);
  pinMode(SENSOR_MOIST_RIGHT, OUTPUT);
  pinMode(RELAY_PIN_PUMP, OUTPUT);
  pinMode(RELAY_PIN_VENT, OUTPUT);
  pinMode(RELAY_PIN_LIGHT, OUTPUT);
  pinMode(RELAY_PIN_HEATER, OUTPUT);
  pinMode(LEVEL_PIN, INPUT_PULLUP);

  
  // Start up the library
  sensors.begin();
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
        unsigned long endTime = bme.beginReading();
          if (endTime == 0) {
             Serial.println(F("Failed to begin reading!"));
             return;
          }
          if (!bme.endReading()) {
             Serial.println(F("Failed to complete reading!"));
             return;
          }
      //TEMPERATURE SENZOR
         sensors.requestTemperatures();
         
      //Calculate the parameters
       float temperatureBME680 = bme.temperature;
       float pressureBME680 = bme.pressure / 100.0;
       float humidityBME680 = bme.humidity;
       float gasBME680 = bme.gas_resistance / 1000.0;
       float temperatureWater = sensors.getTempCByIndex(0);
       float temperatureSoil = sensors.getTempCByIndex(1);
    
       //Flag Pump Manual Value
        bool pumpFlag = getValueJson("https://api.thingspeak.com/channels/1680507/feeds.json?results=1");
       //Flag Pump Manual Value
        bool manualPump = getValueJson("https://api.thingspeak.com/channels/1681543/feeds.json?results=1");
       //Light Manual Value
        bool lightFlag = getValueJson("https://api.thingspeak.com/channels/1690046/feeds.json?results=1");
       //Light Manual Value
        bool manualLight = getValueJson("https://api.thingspeak.com/channels/1690049/feeds.json?results=1");
        //Flag Ventilation Manual Value
        bool ventFlag = getValueJson("https://api.thingspeak.com/channels/1690055/feeds.json?results=1");
       //Flag Ventilation Manual Value
        bool manualVent = getValueJson("https://api.thingspeak.com/channels/1690058/feeds.json?results=1");
        //Flag Heater Manual Value
        bool heaterFlag = getValueJson("https://api.thingspeak.com/channels/1690004/feeds.json?results=1");
       //Flag Heater Manual Value
        bool manualHeater = getValueJson("https://api.thingspeak.com/channels/1690028/feeds.json?results=1");
       
       /*
       //Mode Value
        bool modeGreenHouse = 1; 
       //Mode Selecting
        if (modeGreenHouse == 1)
        {
          int modeEnvironment[7] = {modeOne[7]};
        }
        int moistAutomaticLow = modeEnvironment[1];
        int moistAutomaticHigh = modeEnvironment[2];
        int lightAutomatic = modeEnvironment[3];
        int ventAutomaticLow = modeEnvironment[4];
        int ventAutomaticHigh = modeEnvironment[5];
        int heaterAutomaticLow = modeEnvironment[6];
        int heaterAutomaticHigh = modeEnvironment[7];

        Serial.println("moistAutomaticLow");
        Serial.println(moistAutomaticLow);
        Serial.println("lightAutomatic");
        Serial.println(lightAutomatic);
        */


         
       //Moisture sensor
        soilMoisturePercentLeft = moistureSensorLeft();
        soilMoisturePercentRight = moistureSensorRight();
    
       // Tank Level Switch
        bool tankLevel = tankLevelSwitch();  
        
       //Pump Control
        bool pumpState = pumpControl(soilMoisturePercentLeft,pumpFlag,tankLevel,manualPump);
        if (pumpState == true)
        {
          // Pump Switch On
          digitalWrite(RELAY_PIN_PUMP, LOW);
        }
        else
        {
          // Pump Switch Off
         digitalWrite(RELAY_PIN_PUMP, HIGH);
        }
    
       //Light sensor
        lightLuxValue = lightSensor();
        
       // Light Control
        bool lightState = lightControl(lightLuxValue,lightFlag,manualLight);
        if (lightState == true)
        {
            // Light Switch On
            digitalWrite(RELAY_PIN_LIGHT, LOW);
        }
        else
        {
            // Light Switch Off
            digitalWrite(RELAY_PIN_LIGHT, HIGH);
        }
    
       // Ventilation Control
        bool ventState = ventControl(temperatureBME680,ventFlag,manualVent);
        Serial.println("temperatureBME680");
        Serial.println(temperatureBME680);
        if (ventState == true)
        {
            // Vent Switch On
            digitalWrite(RELAY_PIN_VENT, LOW);
        }
        else
        {
            // Vent Switch Off
            digitalWrite(RELAY_PIN_VENT, HIGH);
        }
    
      // Heater Control
    
        bool heaterState = heaterControl(temperatureBME680,heaterFlag,manualHeater);
        if (heaterState == true)
        {
            // Vent Switch On
            digitalWrite(RELAY_PIN_HEATER, LOW);
        }
        else
        {
            // Vent Switch Off
            digitalWrite(RELAY_PIN_HEATER, HIGH);
        }

       
    // Alarm Trigger Timer
      if (alarmFlag == true)
      {
          pumpAlarmControl(soilMoisturePercentLeft,pumpFlag,tankLevel,manualPump);
          //lightAlarmControl(lightLuxValue,lightFlag,lightState,manualLight);
          ventAlarmControl(temperatureBME680,ventFlag,ventState,manualVent);
          heaterAlarmControl(temperatureBME680,heaterFlag,heaterState,manualHeater);
  
          alarmFlag = false;
       }

       // Timer ended - prepare the values for thingspeak
       if(sendingFlag == true)
       {
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();

       //Print JsonObject of sensor's values in Serial Monitor
         root["Temperature Soil"] = sensors.getTempCByIndex(0);
         root["Temperature Water"] = sensors.getTempCByIndex(1);
         root["Light Lux Value"] = lightLuxValue;
         root["Soil Moisture Percent Left"]= soilMoisturePercentLeft;
         root["Soil Moisture Percent Right"]= soilMoisturePercentRight;
         serializeJsonPretty(doc, Serial);
         Serial.println();

        // Fields for sensors in ThingSpeak
            ThingSpeak.setField(1, temperatureSoil);
            ThingSpeak.setField(2, temperatureWater);
            ThingSpeak.setField(3, lightLuxValue);
            ThingSpeak.setField(4, soilMoisturePercentLeft);
            ThingSpeak.setField(5, soilMoisturePercentRight);
            
            String myStatus = ("Updating data for channel 1");
        
        // set the status
            ThingSpeak.setStatus(myStatus);
  
        // write to the ThingSpeak channel for sensors
         int x = ThingSpeak.writeFields(myChannelNumberChannel1, myWriteAPIKeyChannel1);
         if(x == 200)
         {
            Serial.println("Channel 1 updated successfuly.");
         }
         else
         {
            Serial.println("Problem updating channel. HTTP error code " + String(x));
         } 

        //START FOR THE SECOND THINGSPEAK CHANNEL

         //Print JsonObject of relay's values in Serial Monitor
            
           root["Pump State"] = pumpState;
           root["Vent State"] = ventState;
           root["Light State"] = lightState;
           root["Heater State"] = heaterState;
           root["Temperature BME680"] = bme.temperature;
           root["Pressure BME680"] = bme.pressure / 100.0;
           root["HumidityBME680"] = bme.humidity;
           root["Gas BME680"] = bme.gas_resistance / 1000.0;
           serializeJsonPretty(doc, Serial);
           Serial.println();


         // Fields for actuators in ThingSpeak
            ThingSpeak.setField(1, pumpState);
            ThingSpeak.setField(2, ventState);
            ThingSpeak.setField(3, lightState);
            ThingSpeak.setField(4, heaterState);
            ThingSpeak.setField(5, temperatureBME680);
            ThingSpeak.setField(6, pressureBME680);
            ThingSpeak.setField(7, humidityBME680);
            ThingSpeak.setField(8, gasBME680);
            myStatus = ("Updating data for channel 2");
         
         // set the status
            ThingSpeak.setStatus(myStatus);
         // Write to the ThingSpeak channel     
            int y = ThingSpeak.writeFields(myChannelNumberChannel2, myWriteAPIKeyChannel2);
            if(y == 200)
            {
               Serial.println("Channel 2 updated successfuly.");
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
