#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Adafruit_BME680.h" 
#include <WiFi.h>
#include <HTTPClient.h>
#include "ThingSpeak.h"
#include <ESP_Mail_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "ENTER_YOUR_WIFI_NAME"; 
const char* password = "ENTER_YOUR_WIFI_PASSWORD";

//Reconnect parameters
unsigned long previousMillis = 0;
unsigned long interval = 20000;
//Moisture sensor
const int airValueLeft = 2032;   //You need to replace this value with value of the sensor in the air
const int waterValueLeft = 1489;  //You need to replace this value with value of the sensor in the water
const int airValueRight = 2032;  //You need to replace this value with value of the sensor in the air
const int waterValueRight = 1489;  //You need to replace this value with value of the sensor in the water
//Thingspeak
   //Write the sensor's parameters of the GreenHouse to the Thingspeak
const char* myWriteAPIKeyChannel1 = "ENTER_ThingSpeak_API_Key"; //ThingSpeak API Key. Example: "70TO1SL3LFUPK53E"
unsigned long myChannelNumberChannel1 = 12345678; //"ENTER_ThingSpeak_Channel_Number"; Example: "1234567"
    //Write the actuator's parameters of the GreenHouse to the Thingspeak
const char* myWriteAPIKeyChannel2 = "ENTER_ThingSpeak_API_Key"; //ThingSpeak API Key. Example: "70TO1SL3LFUPK53E"
unsigned long myChannelNumberChannel2 = 12345678; //"ENTER_ThingSpeak_Channel_Number"; Example: "1234567"

/*Defining the pin on ESP32*/
#define SENSOR_MOIST_LEFT 34
#define SENSOR_MOIST_RIGHT 35
#define ONE_WIRE_BUS 33
#define LIGHT_SENSOR_PIN 32
#define RELAY_PIN_PUMP  26
#define RELAY_PIN_VALVE_SOIL  25
#define RELAY_PIN_VENT  16
#define RELAY_PIN_LIGHT  17
#define RELAY_PIN_HEATER  18
#define LEVEL_PIN 27

/*Defining the port and host for smtp.gmail.com*/
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define AUTHOR_EMAIL "ENTER_SENDER_MAIL_ADRESS" //Example: name.lastname@email.com
#define AUTHOR_PASSWORD "ENTER_SENDER_MAIL_PASSWORD"

/* Sending Emails to: */
#define RECIPIENT_EMAIL "ENTER_RECIEVER_MAIL_ADRESS" //Example: name.lastname@email.com

/*Create names for objects.*/
WiFiClient client;
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Adafruit_BME680 bmeOne;
Adafruit_BME680 bmeTwo;

// Define variables
int soilMoisturePercentLeft = 0;
int soilMoisturePercentRight = 0;
int averageMoistureSoilPercent;
int lightValue;

// Variables to save date and time for light timer
String formattedDate;
String dayStamp;
String timeStamp;
String hourStringStamp;
int hourStamp;

  //States of the Actuators
bool pumpState;
bool lightState;
bool ventState;
bool heaterState;
bool valveSoilState;
bool valveAquaState;

  //BME680 values
float temperatureBME680;
float pressureBME680;
float humidityBME680;
float gasBME680;

 //Last Values for Validate
float lastTemperatureWater;
float lastTemperatureSoil;
float lastGasBME680;
float lastHumidityBME680;
float lastPressureBME680;
float lastTemperatureBME680; 

//Sending Flag
bool sendingFlag = true;
//Email Error Flag
bool alarmFlag = true;
String param;
String stringAlarm;

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
  message.addRecipient("User", RECIPIENT_EMAIL);

  String stringOne = "Hi, User!";
  String stringTwo = ("I think you should check the GreenHouse! " + stringAlarm + " This Value is abnormal!"+ "\n");
  String stringThree = "- Sent from ESP board";
  String stringFour= "Here are the parameters of the GreenHouse!:";
  String stringFinal = (stringOne + "\n"  + stringTwo + "\n" + stringThree + "\n" + stringFour + "\n" + "\n" + 
                        "Soil Moisture Left Side:" + " " + soilMoisturePercentLeft + " %" + "\n" + 
                        "Soil Moisture Right Side:" + " " + soilMoisturePercentRight + " %" + "\n" + 
                        "Soil Moisture Avarage:" + " " + ((soilMoisturePercentRight+soilMoisturePercentLeft)/2) + " %" + "\n" + 
                        "Temperature BME680:" + " " + (((bmeOne.temperature)+(bmeTwo.temperature))/2) + " °C" + "\n" + 
                        "Pressure BME680:" + " " + (((bmeOne.pressure / 100.0)+(bmeTwo.pressure / 100.0))/2) + " hPa" + "\n" + 
                        "Humidity BME680:" + " " + (((bmeOne.humidity)+(bmeTwo.humidity))/2) + "%" + "\n" + 
                        "Gas BME680:" + " " + (((bmeOne.gas_resistance / 1000.0)+(bmeTwo.gas_resistance / 1000.0))/2) + " kOhm" + "\n" + 
                        "Temperature Soil:" + " " + sensors.getTempCByIndex(1) + " °C" + "\n" + 
                        "Temperature Water:" + " " + sensors.getTempCByIndex(0) + " °C" + "\n" + 
                        "Light Value:" + " " + lightValue + "%");
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
  alarmFlag = true;
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

void errorSafety()
{
   digitalWrite(RELAY_PIN_PUMP, HIGH);
   digitalWrite(RELAY_PIN_VENT, HIGH);
   digitalWrite(RELAY_PIN_HEATER, HIGH);
   digitalWrite(RELAY_PIN_LIGHT, HIGH);
   digitalWrite(RELAY_PIN_VALVE_SOIL, HIGH);
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

void pumpAlarmControl(int x,bool y, bool z, bool j)
{ 
  // Decided if pump is on or off. Parameters: x = soilMoisturePercent;y = getValue; z = tankLevelSwitch ; j = ON/OFF manual pump
  if (x > 70 || z == 0) //Alarm active, Pump off bcs of safety. Either moisture is high or level in the tank is low
       {  
          if (x > 70)
          {
            if (x > 70 && y == 1)
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
  //Light sensor
   int lightSensorValue = analogRead(LIGHT_SENSOR_PIN);
   int x = map(lightSensorValue, 4095, 0, 0, 100);
   if(x > 100)
   {
      x = 100;
   }
    else if(x < 0)
   {
      x = 0;
   }
   return x;
}

void lightAlarmControl(int x,bool y, bool z,  bool j)
{ 
  // Decided if light is on or off. 
  // Parameters: x = lightLuxValue; y = lightManualFlag ; z = State of the light relay; j = ON/OFF Manual Light
  if (x > 30 && y == 1 && j > 0)
       { 
           param = "Light Value: ";
           stringAlarm = (param + x + " %. " + "Careful! The light is on manual and it is ON. Should turn it off to save energy!");
           sendSMTP(stringAlarm);
        }
  else if (x <= 10 && y == 1 && j == 0)
       {  
          param = "Light Value: ";
          stringAlarm = (param + x + " %. " + "Careful! The light is on manual and it is OFF. Should turn it on for the GreenHouse!");
          sendSMTP(stringAlarm);
       }
 return; 
}

// VENTILATION PART

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

  //Alarm Timer = 10 min
  timer0 = timerBegin(0, 80, true); 
  timerAttachInterrupt(timer0, &onTimer0, true); 
  timerAlarmWrite(timer0, 600000000, true);
  timerAlarmEnable(timer0); // enable;
  //Sending Timer = 1 min
  timer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer1, &onTimer1, true);  
  timerAlarmWrite(timer1, 60000000, true);
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
  //Relays  
  pinMode(SENSOR_MOIST_LEFT, OUTPUT);
  pinMode(SENSOR_MOIST_RIGHT, OUTPUT);
  pinMode(RELAY_PIN_PUMP, OUTPUT);
  pinMode(RELAY_PIN_VALVE_SOIL, OUTPUT);
  pinMode(RELAY_PIN_VENT, OUTPUT);
  pinMode(RELAY_PIN_LIGHT, OUTPUT);
  pinMode(RELAY_PIN_HEATER, OUTPUT);
  pinMode(LEVEL_PIN, INPUT_PULLUP);

  // The relays always turns on when starting, need to call them to turn off for safety.
  errorSafety();

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  timeClient.setTimeOffset(7200);
  
  // Start up the library
  sensors.begin();
  while (!Serial);
  Serial.println(F("BME680 async test"));

   if (!bmeOne.begin(0x77)) {
    Serial.println(F("Could not find a valid BME680 One sensor, check wiring!"));
    Serial.println("Restarting in 5 seconds"); //BME680 not working always properly, if ESP cant read the parameters, reset ESP will helps to boost BME680 again.
    delay(5000);
    ESP.restart();
    while (1);
  }
  
  bmeOne.setTemperatureOversampling(BME680_OS_8X); //reads temperature in Celsius
  bmeOne.setHumidityOversampling(BME680_OS_2X);   //reads absolute humidity
  bmeOne.setPressureOversampling(BME680_OS_4X);   //reads pressure in hPa (hectoPascal = millibar)
  bmeOne.setIIRFilterSize(BME680_FILTER_SIZE_3); //The BME680 sensor integrates an internal IIR filter to reduce short-term changes in sensor output values caused by external disturbances.
  bmeOne.setGasHeater(320, 150); // 320*C for 150 ms. The BME680 contains a MOX (Metal-oxide) sensor that detects Volatile Organic Compounds (VOC) like ethanol and carbon monoxide. in the air.
  /*
   Sensor: Accuracy
   Pressure: +/- 1 hPa
   Humidity: +/- 3 %
   Temperature: +/- 1.0 ºC
   */

  if (!bmeTwo.begin(0x76)) {
    Serial.println(F("Could not find a valid BME680Two sensor, check wiring!"));
    Serial.println("Restarting in 5 seconds");
    delay(5000);
    ESP.restart();
    while (1);
  }
  
  bmeTwo.setTemperatureOversampling(BME680_OS_8X); //reads temperature in Celsius
  bmeTwo.setHumidityOversampling(BME680_OS_2X);   //reads absolute humidity
  bmeTwo.setPressureOversampling(BME680_OS_4X);   //reads pressure in hPa (hectoPascal = millibar)
  bmeTwo.setIIRFilterSize(BME680_FILTER_SIZE_3); //The BME680 sensor integrates an internal IIR filter to reduce short-term changes in sensor output values caused by external disturbances.
  bmeTwo.setGasHeater(320, 150);
}

void loop(void)
{ 
  unsigned long currentMillis = millis();
  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED)
  {
       //Start the BME680
   unsigned long endTimeOne = bmeOne.beginReading();
      if (endTimeOne == 0) {
         Serial.println(F("Failed to begin reading!"));
         return;
      }
      if (!bmeOne.endReading()) {
         Serial.println(F("Failed to complete reading!"));
         return;
      }

    unsigned long endTimeTwo = bmeTwo.beginReading();
      if (endTimeTwo == 0) {
         Serial.println(F("Failed to begin reading!"));
         return;
      }
      if (!bmeTwo.endReading()) {
         Serial.println(F("Failed to complete reading!"));
         return;
      }

      // Get real time - Slovakia  
        while(!timeClient.update()) {
        timeClient.forceUpdate();
        }
        formattedDate = timeClient.getFormattedDate();
        int splitT = formattedDate.indexOf("T");
        dayStamp = formattedDate.substring(0, splitT);
        // Extract time
        timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
        hourStringStamp = formattedDate.substring(splitT+1, formattedDate.length()-7);
        hourStamp = hourStringStamp.toInt();
        Serial.print("RealTime: ");
        Serial.println(formattedDate);

  //SENSORS      
      //TEMPERATURE SENZOR
       sensors.requestTemperatures();
       
      //Moisture sensor
      soilMoisturePercentLeft = moistureSensorLeft();
      soilMoisturePercentRight = moistureSensorRight();
    
      // Tank Level Switch
      bool tankLevel = tankLevelSwitch();  

      //Light sensor
      int lightValue = lightSensor();
         
      //Calculate the parameters
      float temperatureBME680One = bmeOne.temperature;
      float pressureBME680One = bmeOne.pressure / 100.0;
      float humidityBME680One = bmeOne.humidity;
      float gasBME680One = bmeOne.gas_resistance / 1000.0;

      float temperatureBME680Two = bmeTwo.temperature;
      float pressureBME680Two = bmeTwo.pressure / 100.0;
      float humidityBME680Two = bmeTwo.humidity;
      float gasBME680Two = bmeTwo.gas_resistance / 1000.0;
     
      float temperatureWater = sensors.getTempCByIndex(0);
      float temperatureSoil = sensors.getTempCByIndex(1);

      // Averaging sensors parameters
      temperatureBME680 = (temperatureBME680One + temperatureBME680Two)/2;
      pressureBME680 = (pressureBME680One + pressureBME680Two)/2;
      humidityBME680 = (humidityBME680One + humidityBME680Two)/2;
      gasBME680 = (gasBME680One + gasBME680One)/2;
      averageMoistureSoilPercent = (soilMoisturePercentLeft+soilMoisturePercentRight)/2;
      
      //Soft Validate the range of each sensor - provided by manufacturer. If lower or higher, it is seem to be fail measured data.
       if ((temperatureWater < -10)||(temperatureWater > 85))
       {
        temperatureWater = lastTemperatureWater;
       }
       if ((temperatureSoil < -10)||(temperatureSoil > 85))
       {
        temperatureSoil = lastTemperatureSoil;
        }
       if ((temperatureBME680 > 85)||(temperatureBME680 < -15))
       //not real to have temperature of the air lower than -15 celsius, 
       //the BME can measure the temperature up to -40 but if we set the border -15, can eliminate some wrong data from sensor. 
       {
        temperatureBME680 = lastTemperatureBME680;
       }
       if ((pressureBME680 < 300)||(pressureBME680 > 1013))
       {
        pressureBME680 = lastPressureBME680;
       }
       if ((humidityBME680 < 1)||(humidityBME680 > 99)) // Not real to have 100% humidity in the room
       {
        humidityBME680 = lastHumidityBME680;
       }
       if ((gasBME680 < 1)||(gasBME680 > 500))
       {
        gasBME680 = lastGasBME680;
       }

       // Remember the last values

       lastTemperatureBME680 = temperatureBME680;
       lastPressureBME680 = pressureBME680;
       lastHumidityBME680 = humidityBME680;
       lastGasBME680 = gasBME680;
       lastTemperatureWater = temperatureWater;
       lastTemperatureSoil = temperatureSoil;
       
        //Flag Pump Manual Value
        bool pumpFlag = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL"); //Example https://api.thingspeak.com/channels/xxxxxx/feeds.json?results=1
       //Pump Manual Value
        bool manualPump = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       //Flag Valve Soil Value
        bool valveSoilFlag = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       //Valve Soil Manual Value
        bool manualSoilValve = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       //Light Manual Value
        bool lightFlag = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       //Light Manual Value
        bool manualLight = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
        //Flag Ventilation Manual Value
        bool ventFlag = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       //Flag Ventilation Manual Value
        bool manualVent = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
        //Flag Heater Manual Value
        bool heaterFlag = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       //Flag Heater Manual Value
        bool manualHeater = getValueJson("ENTER_URL_FEED_API_FROM_THINGSPEAK_CHANNEL");
       
      //PUMP CONTROL
          // Decided if pump is on or off. 
          // Parameters: x = soilMoisturePercent; y = pumpManualFlag ; z = tankLevelSwitch ; j = ON/OFF Manual Pump
          if (tankLevel == 0) //Pump is off because of safety. Either moisture is high or level in the tank is low
              { 
              digitalWrite(RELAY_PIN_PUMP, HIGH);
              pumpState = false;   
              }
          else if (tankLevel == 1) //Pump is off because of safety. Either moisture is high or level in the tank is low
              { 
              if (tankLevel == 1 && pumpFlag == 1 && manualPump == 1)
              {
              digitalWrite(RELAY_PIN_PUMP, LOW);
              pumpState = true;   
              }
              else if (tankLevel == 1 && pumpFlag == 1 && manualPump == 0)
              {
              digitalWrite(RELAY_PIN_PUMP, HIGH);
              pumpState = false; 
              }
              else
              {
              digitalWrite(RELAY_PIN_PUMP, LOW);
              pumpState = true;  
              }
             }
               
     //Manual control Soil
          // If put the valve on manual and soil is more than 90%, for the safety the valve will not turns on
          if (averageMoistureSoilPercent > 90)
              {  
                 if (averageMoistureSoilPercent > 90 && valveSoilFlag == 1 && tankLevel == 1)
                 {
                  digitalWrite(RELAY_PIN_VALVE_SOIL, HIGH);
                  valveSoilState = false;
                 }
                  else if (averageMoistureSoilPercent > 90 && valveSoilFlag == 0 && tankLevel == 1)
                  {
                  digitalWrite(RELAY_PIN_VALVE_SOIL, HIGH);
                  valveSoilState = false;
                  }
                }
                     
           if  (averageMoistureSoilPercent <= 90 && valveSoilFlag == 1 && manualSoilValve == 0 && tankLevel == 1)
                 { 
                  digitalWrite(RELAY_PIN_VALVE_SOIL, HIGH);
                  valveSoilState = false;
                  }
               else if (averageMoistureSoilPercent <= 90 && valveSoilFlag == 1 && manualSoilValve > 0 && tankLevel == 1)
                  {
                  digitalWrite(RELAY_PIN_VALVE_SOIL, LOW);
                  valveSoilState = true;
                  } 
        //Automation Control of the pump
        //If the moisture of the soil drops under 30 percents,
        //turns the pump on till the moisture get higher than 55 percent.           
           if (averageMoistureSoilPercent < 20 && valveSoilFlag == 0 && tankLevel == 1)
               {
               digitalWrite(RELAY_PIN_VALVE_SOIL, LOW);
               valveSoilState = true;
               }
           if (averageMoistureSoilPercent > 60 && valveSoilFlag == 0 && tankLevel == 1)
              {
              digitalWrite(RELAY_PIN_VALVE_SOIL, HIGH);
              valveSoilState = false;
              } 
              
  // HEATER CONTROL
        // Decided if heater is on or off. 
         if (temperatureBME680 <= 85 && heaterFlag == 1)
            {
             if (temperatureBME680 <= 85 && heaterFlag == 1 && manualHeater > 0) //turn on manually
                 {
                 digitalWrite(RELAY_PIN_HEATER, LOW);
                 heaterState = true; 
                 }
             else if (temperatureBME680 <= 85 && heaterFlag == 1 && manualHeater == 0) //turn off manually
                 {
                 digitalWrite(RELAY_PIN_HEATER, HIGH);
                 heaterState = false;
                 }
             }
      
        //Automation Control of the heater
        //If the temperature drops under 16 degree Celsius,
        //turns the heater on till the temperature get higher than 18.
         if (temperatureBME680 <= 16  && heaterFlag == 0)
              {
               digitalWrite(RELAY_PIN_HEATER, LOW);
               heaterState = true; 
              }
         if (temperatureBME680 >= 20 && heaterFlag == 0)
              {
               digitalWrite(RELAY_PIN_HEATER, HIGH);
               heaterState = false;  
              }


  // VENTILATION CONTROL
          // Decided if heater is on or off. 
           if (temperatureBME680 <= 85 && ventFlag == 1)
              {
               if (temperatureBME680 <= 85 && ventFlag == 1 && manualVent > 0) //turn on manually
                   {
                   digitalWrite(RELAY_PIN_VENT, LOW);
                   ventState = true; 
                   }
               else if (temperatureBME680 <= 85 && ventFlag == 1 && manualVent == 0) //turn off manually
                   {
                   digitalWrite(RELAY_PIN_VENT, HIGH);
                   ventState = false;
                   }
               }
        
          //Automation Control of the ventilation
          //If the temperature get higher then 25 degree Celsius,
          //turns the vent on till the temperature not drop until 22.
           if (temperatureBME680 >= 25  && ventFlag == 0)
                {
                 digitalWrite(RELAY_PIN_VENT, LOW);
                 ventState = true; 
                }
           if (temperatureBME680 <= 22 && ventFlag == 0)
                {
                 digitalWrite(RELAY_PIN_VENT, HIGH);
                 ventState = false;  
                }
    

  // LIGHT CONTROL
  // Decided if light is on or off. 
          // Decided if heater is on or off. 
           if (lightValue <= 100 && lightFlag == 1)
              {
               if (lightValue <= 100 && lightFlag == 1 && manualLight > 0) //turn on manually
                   {
                   digitalWrite(RELAY_PIN_LIGHT, LOW);
                   lightState = true; 
                   }
               else if (lightValue <= 100 && lightFlag == 1 && manualLight == 0) //turn off manually
                   {
                   digitalWrite(RELAY_PIN_LIGHT, HIGH);
                   lightState = false;
                   }
               }
        
             //Automation Control of the light
          //If the light percent get lower then 15 percent,
          //turns the light on till the light is not higher 40.
           if (lightValue <= 15 && lightFlag == 0 && hourStamp >= 6) 
                {
                digitalWrite(RELAY_PIN_LIGHT, LOW);
                lightState = true; 
                }
           else if (lightValue <= 15 && lightFlag == 0 && hourStamp < 6)
                   {
                    digitalWrite(RELAY_PIN_LIGHT, HIGH);
                    lightState = false;
                   }
                   
           if (lightValue >= 40 && lightFlag == 0)
                {
                 digitalWrite(RELAY_PIN_LIGHT, HIGH);
                 lightState = false;  
                }
    
      DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();

       //Print JsonObject of sensor's values in Serial Monitor
         root["ventFlag"] = ventFlag;
         root["lightFlag"] = lightFlag;
         root["pumpFlag"] = pumpFlag;
         root["heaterFlag"]= heaterFlag;
         root["manualVent"] = manualVent;
         root["manualLight"] = manualLight;
         root["manualPump"] = manualPump;
         root["manualHeater"]= manualHeater;
         root["valveSoilState"]= valveSoilState;
         root["valveAquaState"]= valveAquaState;
         serializeJsonPretty(doc, Serial);
         Serial.println();
     
         
    // Alarm Trigger Timer
      if (alarmFlag == true)
      {
          pumpAlarmControl(averageMoistureSoilPercent,pumpFlag,tankLevel,manualPump);
          lightAlarmControl(lightValue,lightFlag,lightState,manualLight);
          ventAlarmControl(temperatureBME680,ventFlag,ventState,manualVent);
          heaterAlarmControl(temperatureBME680,heaterFlag,heaterState,manualHeater);
  
       alarmFlag = false;
       }

    // Sending data to database Trigger Timer
       // Timer ended - prepare the values for thingspeak
       if(sendingFlag == true)
       {
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();

       //Print JsonObject of sensor's values in Serial Monitor
         root["Temperature Soil"] = temperatureSoil;
         root["Temperature Water"] = temperatureWater;
         root["Light Percent"] = lightValue;
         root["Soil Moisture Percent Left"]= soilMoisturePercentLeft;
         root["Soil Moisture Percent Right"]= soilMoisturePercentRight;
         root["Soil Moisture Percent Average"]= averageMoistureSoilPercent;
         root["GasBME 680"] = gasBME680;

        // Fields for sensors in ThingSpeak
            ThingSpeak.setField(1, temperatureSoil);
            ThingSpeak.setField(2, temperatureWater);      
            ThingSpeak.setField(3, lightValue);
            ThingSpeak.setField(4, soilMoisturePercentLeft);
            ThingSpeak.setField(5, soilMoisturePercentRight);
            ThingSpeak.setField(6, averageMoistureSoilPercent);
            ThingSpeak.setField(7, gasBME680);
            String myStatus = ("Updating data from GreenHouse");
        
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
           root["Temperature BME680"] = temperatureBME680;
           root["Pressure BME680"] = pressureBME680;
           root["HumidityBME680"] = humidityBME680;
           root["Valve Soil State"] = valveSoilState;
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
            ThingSpeak.setField(8, valveSoilState);
            myStatus = ("Updating data from GreenHouse");
         
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
            // Turning all actuators off for safety.
            errorSafety();
            Serial.println("Reconnecting to WiFi...");
            WiFi.disconnect();
            WiFi.reconnect();
            previousMillis = currentMillis;
    }
  }
}
