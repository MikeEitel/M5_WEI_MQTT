// Modified by Mike Eitel to read temperatur and humidity by M5-C3U based board
// All special modified libraries are stored in the via platformio.ini defined libraries directory !!!
// Took some time to get the right versions etc. working flawless, so this unusual approach is choosen deliberately.
// No rights reserved when used non commercial, otherwise contact author.

// M5 pinout
// static const uint8_t G0 /  =  0;    Port A  Used for IO                      I2_SCL
// static const uint8_t G1 /  =  1;    Port A  Used for IO                      I2_SDA
// static const uint8_t G2 /  =  2;    Internal WS2812 typ LED =    SK6812
// static const uint8_t G3 /  =  3;    CLK Clock for            all HX71x
// static const uint8_t G4 /  =  4;    CS1 Chip select for        1.HX71x
// static const uint8_t G5 /  =  5;    CS2 Chip select for        2.HX71x
// static const uint8_t G6 /  =  6;    CS3 Chip select for        3.HX71x
// static const uint8_t G7 /  =  7;    CS4 Chip select for        4.HX71x     
// static const uint8_t G8 /  =  8;    NC as interal pullup high =  Flash mode
// static const uint8_t G9 /  =  9;    Internal button pullup hw    Restart     BTN_A 
// static const uint8_t G10 / = 10;    Used for                     DS18B20 x
// static const uint8_t G18 / = 18;    USB D- Used for powering     DHT to make it resetable
// static const uint8_t G19 / = 19;    USB D+ Used for              DHT data
// static const uint8_t G20 / = 20;    Rx not used for programming
// static const uint8_t G21 / = 21;    Tx not used for programming

#define Me      // If defined ( Me .. MeIOT .. Rhy ) use private network for testing, otherwise use IOT standard
//#define TEST    // Testmodus
#define OTA     // If defined use OTA programming posibility

#include <cstdlib>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>

#include <My_ESP-C3_Tools.h>

// Setup the background classes  
WiFiClient   espClient;                       // Get Wifi access
PubSubClient mqttclient;                      // MQTT protokol handler

// This files contain the device definitions for different networks
// In the credential files the fixed IP has to be defined
#if defined(Me)                       
    #include <Me_credentials.h>      
#elif defined(MeIOT)
    #include <MeIOT_credentials.h>     
#elif defined(MeTEST)
    #include <MeTEST_credentials.h>
#elif defined(Rhy)
    #include <Rhy_credentials.h> 
#else
    #include <credentials.h>         
#endif

// Define used pins
#define LED_PIN         2             // Used for ON BOARD LED
#define RESTART_PIN     9             // BOOT pin = Press on top-switch to restart

#define DS_PIN          1             // Digital pin G1  on M5-C3U  read data from the DS18B20 sensors
#define DHT_POWER      19             // Digital pin D+  on M5-C3U  to supply power to the DHT sensor
#define DHT_PIN        18             // Digital pin D-  on M5-C3U  read data from the DHT sensor

#define SCK             3             // Digital pin G3  on M5-C3U  connect clock for all HX71x
#define DT1             4             // Digital pin G4  on M5-C3U  connect data for 1. HX71x
#define DT2             5             // Digital pin G5  on M5-C3U  connect data for 2. HX71x
#define DT3             6             // Digital pin G6  on M5-C3U  connect data for 3. HX71x
#define DT4             7             // Digital pin G7  on M5-C3U  connect data for 4. HX71x
#define DT5            10             // Digital pin G10 on M5-C3U  connect data for 5. HX71x

const uint8_t DT[] = {4, 5, 6, 7,10}; 
const uint8_t IO[] = {0, 1, 7, 6, 5}; 

//#define IN0             0             // Port A pin G0   on M5-C3U  connect as 1.input 
//#define IN1             1             // Port A pin G1   on M5-C3U  connect as 2.input  
//#define OUT0            0             // Port A pin G0   on M5-C3U  connect as 1.ouput   
//#define OUT1            1             // Port A pin G1   on M5-C3U  connect as 2.ouput


//#define SDA             1             // Port A pin G1   on M5-C3U  connect data of I2C chips
//#define SCL             0             // Port A pin G0   on M5-C3U  connect clock of I2C chips
//#define maxSCK          8             // Digital pin G8  on M5-C3U  ATTENTION Bootstrap-Pin must NOT be GND on startup
//#define maxMISO         7             // Digital pin G7  on M5-C3U  
//No MOSI                                
//#define CS            10              // Digital pin G6  on M5-C3U  c

// Define onboard led
#define LEDS_COUNT     1

Adafruit_NeoPixel led = Adafruit_NeoPixel(LEDS_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int LEDhigh =       255;
int LEDmid  =       128;
int LEDlow  =        32;
int LEDoff  =         0;

// Declare global variables and constants
unsigned long currentMillis;                  // Actual timer 
unsigned long prevRMQTTMillis = 2764472319;   // Stores last MQTT time value was published 2764472319->FASTER START
unsigned long prevSMQTTMillis = 2764472319;   // Stores last MQTT time value was published 2764472319->FASTER START
unsigned long prevMinMillis = 2764472319;     // Stores last minutes time value ->  2764472319->FASTER START

//const int readSensorsinterval  = 1000;        // How often the sensors are read

String Sendme="";                            // Used for clear text messages in MQTT
String MySensors ="";                        // A list of usable sensors
char debug_buf[96];                           // Placeholder for MQTT text messages
int receivedlenght;                           // How long is mqtt message
char lastreceived;                            // Stores the last received status
char receivedChar[10];                        //  = "";
bool received;                                // Actual received status
int watchdogW = 1;                            // Counter if there is no wifi connection
int watchdogM = 1;                            // Counter if there is no MQTT connection
int mqttstatus;                               // Helper to see whats going on
bool watchdog = true;                         // Signal via mqtt that device is still ok
bool statusreset = false;                     // Used to minimize error 0 sendouts
int looped = 1;                               // Loop counter as debug helper
long deltasend = 0;                           // Value <> 0 switch between cyclic and delta-triggert mqtt sending
float hum;                                    // Measured humidity
float temp;                                   // Measured temperature
long rawp;                                    // Measured raw pressure
long raww;                                    // Measured raw weight
int numsensors = 0;                           // Number of existing sensors
int numhum = 0;                               // Counter for existing humidity sensors
int numtemp = 0;                              // Counter for existing temperatur sensors
int numpress = 0;                             // Counter for existing presure sensors
int numweight = 0;                            // Counter for existing weight sensors
int numin = 0;                                // Counter for configured inputs
int numout = 0;                               // Counter for configured outputs
//#include <MySensors.h>

///////////////////////////////////////////////////////// Sensors ///////////////////////////////////
#if defined(enableDHT)                        // One wire based DHT temperatur and humidity sensors
  #include "MyDHTxx.h"
#endif
#if defined(enableDS)                         // DS18B20 temperatur sensors
  #include "MyDSx.h"
#endif
#if defined(enableHX)                         // Weight  sensors
  #include "MyHX71x.h"
#endif
#if defined(enableIN) || defined(enableOUT)  // IO's
  #include "MyIO.h"
#endif

void setup_wifi() {
  WiFi.disconnect(false);                     // Stack reset ohne SSID zu löschen
  delay(100);
  WiFi.hostname(iamclient);
  WiFi.config(staticIP, gateway, subnet);
  
  Serial.println("");
  Serial.print("Try connect to: ");
  Serial.println(wifi_ssid);
  Serial.print("With IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  while ((WiFi.status()!= WL_CONNECTED) && (watchdogW <= WiFi_timeout)) {
    led.setPixelColor(0, led.Color(LEDoff,LEDoff,LEDmid));
    led.show();    
    delay(250);
    led.setPixelColor(0, led.Color(LEDmid,LEDoff,LEDhigh));
    led.show();
    Serial.print(".");
    Serial.print(watchdogW);
    watchdogW++;
    }
    if (WiFi.status()!= WL_CONNECTED){
      Serial.println("No connection -> Restart");
      led.setPixelColor(0, led.Color(LEDhigh,LEDoff,LEDoff));
      led.show();
      WiFi.disconnect(true);
      RestartDevice();
    }
  else {
    Serial.println(""); Serial.println("");
    Serial.print("Successfull connected Wifi with RSSI: "); 
    Serial.println(WiFi.RSSI());
    led.setPixelColor(0, led.Color(LEDoff,LEDoff,LEDhigh));
    led.show();
  }
}

#if defined(OTA)                                    // Augment by OTA posibility
  #include <ArduinoOTA.h>
  bool otaInProgress = false;

void setupOTA(){                                    
    ArduinoOTA.end();                               // Bestehenden OTA-Server beenden falls aktiv
    delay(100);
    ArduinoOTA.setPort(3232);                       // Port defaults to 3232
    // Hostname defaults to esp32-[ChipID] Set this BEFORE calling begin()
    ArduinoOTA.setHostname(iamclient);              // Choose a unique name
    // No authentication by default Password can be set with MD5 hash as well MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    ArduinoOTA.setPassword("admin");

    // OTA event handlers
    ArduinoOTA.onStart([]() {
      otaInProgress = true;
      mqttclient.disconnect();                      // ← MQTT Socket freigeben vor OTA
      espClient.stop();                             // ← WiFiClient Socket explizit schliessen
      delay(100);
      Serial.println("OTA Started");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\n", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf(" IP:%s Port:%u\n", ArduinoOTA.getHostname().c_str(), 3232);
        Serial.printf("OTA Error[%u]: ", error);
        if      (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)     Serial.println("End Failed");
       }
      );
    ArduinoOTA.onEnd([]() {
        otaInProgress = false;
        Serial.println("OTA End");
    });
    ArduinoOTA.begin();
    Serial.println("OTA Initialized");
  }

void checkOTA(){
    ArduinoOTA.handle();
  }
#endif

void reconnect() {
  delay(100);
  // Loop until we're reconnected to MQTT server
  while (!mqttclient.connected() && (watchdogM <= mqtt_timeout)) {
    mqttclient.clearWriteError();                   // Cleaning MQTT write buffer
    mqttclient.flush();                             // Cleaning MQTT data buffer
    mqttstatus = mqttclient.state();                // Decoding of MQTT status 
    Serial.println("");
    Serial.print("Attempting MQTT connection try Nr. ");
    led.setPixelColor(0, led.Color(LEDoff,LEDmid,LEDmid));
    led.show();
    Serial.println(watchdogM);
  
    const char *reason;
    switch (mqttstatus) {
      case -4 : {reason = "MQTT server didn't respond within the keepalive time"; break;}
      case -3 : {reason = "MQTT network connection was broken"; break;}
      case -2 : {reason = "MQTT network connection failed"; break;}
      case -1 : {reason = "MQTT client is disconnected cleanly"; break;}
      case  0 : {reason = "MQTT client is connected"; break;}
      case  1 : {reason = "MQTT server doesn't support the requested version of MQTT"; break;}
      case  2 : {reason = "MQTT server rejected the client identifier"; break;}
      case  3 : {reason = "MQTT server was unable to accept the connection"; break;}
      case  4 : {reason = "MQTT username/password were rejected"; break;}
      case  5 : {reason = "MQTT client was not authorized to connect"; break;}
      default: {   }      // Wrong detection              
    }
    Serial.println(reason);
    if (mqttclient.connect(iamclient, mqtt_user, mqtt_password)) {
      Serial.print("Connected as: ");
      Serial.println(iamclient);
      Serial.println("");
      watchdogM = 1;
      led.setPixelColor(0, led.Color(LEDoff,LEDhigh,LEDoff));
      led.show();
      // Send status after start
      #if defined(TEST)                               // Send status after start
        mqttclient.publish(out_status, "1" ,false);
      #else
        mqttclient.publish(out_status, "MQTT Connected" ,false);
        delay(500);
      #endif 
    }
    else {
      Serial.printf("With RSSI: %i Retry %i. MQTT in 4 seconds ...\n", WiFi.RSSI(), watchdogM);
      // Wait 4 seconds before retrying with OTA-frindlye wait loop
      for (int i = 0; i < 14; i++) {
        led.setPixelColor(0, led.Color(LEDoff,LEDhigh,LEDhigh));
        led.show();
        delay(250);
        #if defined(OTA)
          checkOTA();
        #endif
        led.setPixelColor(0, led.Color(LEDoff,LEDmid,LEDmid));
        led.show();
      }
      watchdogM++;

      if (watchdogM >= mqtt_timeout) {
        Serial.println("NO MQTT available -> RESTART");
        led.setPixelColor(0, led.Color(LEDhigh,LEDoff,LEDoff));
        led.show();
        delay(5000);
        RestartDevice();                                  // REBOOT of the system !!!!!
      }
    }
  }
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  receivedlenght = length;
  Serial.println("");
  Serial.print("Message for [");
  Serial.print(topic);
  Serial.print("] arrived = L:(");
  Serial.print(length);
  Serial.print(")->");
  for (unsigned int i = 0; i < length; i++) {
    receivedChar[i] = payload[i];    
    Serial.print(receivedChar[i]);                          //     Received from mqtt text
  }
  receivedChar[length] = '\0';                      // Add end of string
  Serial.println();
  #if defined(TEST)
      mqttclient.publish(out_status, "3" ,false);
    #else
      Sendme = "Command  ";
      Sendme = Sendme + (receivedChar[0]);
      Sendme = Sendme + " received";
      mqttclient.publish(out_status, (String(Sendme).c_str()) ,false);
      statusreset = true;
      delay(1000);
  #endif
  switch (receivedChar[0]) {   // Detecition what command is detected 
    case '?' :  {                                                               // Request parameters
      Sendme = "RSSI: ";
      Sendme += (WiFi.RSSI());
      mqttclient.publish(out_param,   (String(Sendme).c_str()), false);         // Send the wifi signal strenght
      mqttclient.publish(out_sensors, (String(MySensors).c_str()), false);      // Send list of the usable sensors
      readvalues();                                                             // Send actual sensor measurements
      #if defined(enableIN)
        sendIN();                                                               // Send input stati  
      #endif
      #if defined(enableOUT)
        sendOUT();                                                              // Send output stati
      #endif
      scanI2Cbus();                                                             // Show I2C bus devices on Serial if exist
      #if defined(enableIN) || defined(enableOUT)                              
        showIO();                                                               // Show used IO's and type on Serial if exist
      #endif
      #if defined(enableHX)                                                     
        showdeltasends();                                                       // Show Delta values of value driven mqtt send
      #endif
      break;   }
    case 'D' : {                                                                // Send mqtt on value change
      int Va = 0;
      long Va2 = 0 ;
      #if defined(enableHX)
        Va = x2i(receivedChar,1,1);
        Va2 = x2l(receivedChar,2,9);
        if (Va <= (HXcounted - 1) && length > 1) {
          HX71x_DELTA[Va] = Va2; 
          Sendme = "DeltaMeasure_" + String(Va) + ": " + String(Va2);
          mqttclient.publish(out_status, (String(Sendme).c_str()), false);      // Send the commandet delta trigger values
        }
        showdeltasends();                                                       // Show trigger delta on Serial if exist
      #endif
      break;    }
    case 'O' : {                                                                // Serialprint I2C device list
      mqttclient.publish(out_status, "Serialprint I2C requested", false);
      scanI2Cbus();                                                             // Show I2C devices on Serial
      readIOdir();                                                              // Show used IO pins and type on Serial
      break;    }
    case 'Q' : {                                                                // Set output
      int Va = 0;
      int Va2 = 0 ;
      Va = x2i(receivedChar,1,1);
      Va2 = x2i(receivedChar,2,3);
      #if defined(enableOUT)                                                    // Show bus devices on Serial if exist
        setIO(Va, Va2);
        sendOUT();
      #endif
      break;    }
    case 'T' : {                                                                // Set tara
      int Va = 0;
      Va = x2i(receivedChar,1,1);
      #if defined(enableHX)                                                    // Set HX71x tara values
        settara(Va);
        sendHXtara();
      #endif
      break;    }
    case 'U' :  {                                                               // Received from mqtt new update rate 
      int Va = x2i(receivedChar,1,2);                                             
      if (Va <= 3){ Va = 3; };
      Sendme = "Update :" + String(Va);
      mqttclient.publish(out_status, (String(Sendme).c_str()), false);          // Send the sensor update rate
      readinterval = Va *  500;
      sendinterval = Va * 1000;
      break;    }
    case 'X' :  {                                                               // Restart device
      mqttclient.publish(out_status, "Restart requested", false);
      RestartDevice();
      break;    }
    default:    {                                                               // Wrong command
      #if defined(TEST)
        mqttclient.publish(out_status, "-1" ,false);
      #else
        mqttclient.publish(out_status, "No valid command" ,false);
        statusreset = true;
        //delay(1000);
      #endif
      break;
    } 
      // DIRTY TRICK to read all mqtt's fast if they are stacked
  // This overwrites the normal mqtt request timing by simulating an "older" timestamp
  prevRMQTTMillis = currentMillis - (readinterval - 250); // But at least 250ms before retrigger
     
  }
}

void RestartDevice(){
  mqttclient.publish(out_status, "Restart Device", false);
  Serial.println("Restart Device");
  delay(1000);
  mqttclient.disconnect();
  Serial.println("MQTT disconnect");
  delay(1000);
  WiFi.disconnect(true);                                                  // true = auch gespeicherte SSID löschen
  Serial.println("Wifi disconnect");
  delay(1000);
  ESP.restart();                                                          // REMOTE RESTART
}

void readvalues() {
  led.setPixelColor(0, led.Color(LEDoff,LEDhigh,LEDoff));
  led.show();
  #if defined(enableIN) || defined(enableOUT)
    MySensors = "";
    readIO();
  #endif
  MySensors += String(numsensors) + "xSen-> ";

#if defined(enableDHT)          // Sensor reading DHT
    readDHT();   
  #endif
  #if defined(enableDS)           // Sensor reading DS
    readDS();   
  #endif                      
  #if defined(enableHX)           // Sensors reading HX71x
    readHX();   
  #endif                 
  led.setPixelColor(0, led.Color(LEDoff,LEDlow,LEDoff));
  led.show();
}

 // Helper function for substring to integer conversion 
int x2i(const char *s, int from_a, int to_b) {
  char temp[33];                                // Enough for 32-bit hex + null terminator
  int len = to_b - from_a + 1;
  strncpy(temp, s + from_a, len);
  temp[len] = '\0';
  return (int)strtol(temp, nullptr, 16);
}

// Helper function for substring to long conversion
long x2l(const char *s, int from_a, int to_b) {
  char temp[33];                                // Enough for 32-bit hex + null terminator
  int len = to_b - from_a + 1;
  strncpy(temp, s + from_a, len);
  temp[len] = '\0';
  return (int32_t)strtoul(temp, nullptr, 16);   // unsigned parse → signed reinterpret
}

// Helper function to identify I2C devices
void scanI2Cbus(){
  #if defined(enablewire)
    Serial.print("\nI2C devices: \t");
    for (uint8_t addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0)
        Serial.printf(" 0x%02X\t", addr);
    }
    Serial.print("\nNo more I2C devices:\n");
  #else
    Serial.print("\nNo I2C devices:\n");
  #endif
}


// Helper to propagte delta sending for measurements
void showdeltasends(){
  #if defined(enableHX)
    Sendme = "HX-Delta: ";
    for (int i = 0; i < HXcounted; i++) {
      Sendme += String(i) + ".=" + String(HX71x_DELTA[i]) + "   ";
    }
    mqttclient.publish(out_qual, (String(Sendme).c_str()), false);            // Send list of the usable sensors
    #if defined(TEST)
      for (int i = 0; i < HXcounted; i++) {
        Serial.printf("HX71x_%i Delta: %ld\t",i, HX71x_DELTA[i]);
      }
    Serial.println();
    #endif
  #endif
}
// XXXXXXXXXXXXXXXXXXXXXXXX PROGRAM  START XXXXXXXXXXXXXXXXXXXXXXX

void setup() {
  // Initialize onboard LED and restart button
  led.begin();
  led.setBrightness(LEDbrightness);
  led.setPixelColor(0, led.Color(LEDmid,LEDmid,LEDmid));
  led.show();
  pinMode(RESTART_PIN, INPUT);

  // Initialize debug output and wifi and preset mqtt
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  Serial.println("");
  Serial.print("I am: ");
  Serial.print(iamclient);
  Serial.println("");

  setup_wifi(); // Start the wifi connection
  delay(100);
  #if defined(OTA)                                  
    setupOTA();                                     // Start OTA posibility
  #endif

  mqttclient.setClient(espClient);
  mqttclient.setServer(mqtt_server, mqtt_port);
  mqttclient.setCallback(callback);
  mqttclient.setKeepAlive(61);      // MQTT_KEEPALIVE : keepAlive interval in seconds. Override setKeepAlive()
  mqttclient.setSocketTimeout(63);  // MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override setSocketTimeout()
  reconnect();                      // Start the mqtt connection would read first meassures with 0
  mqttclient.subscribe(in_topic);   // Listen to the mqtt inputs
  
  #if defined(enableWire)
    Wire.begin(SDA, SCL);
    Wire.setTimeOut(200); 
    delay(500);
    Serial.println("Wire started");
	  #if defined(TEST)
      scanI2Cbus();
    #endif
  #endif

  ///////////////////// Sensors startup //////////////////////////////////////////////////////
  led.setPixelColor(0, led.Color(LEDlow,LEDlow,LEDoff));
  led.show();
  #if defined(enableIN) || defined(enableOUT)
    startIO();
  #endif
  #if defined(enableDHT)        // Initialize the DHT sensor
    startDHT();
  #endif
  #if defined(enableDS)        // Initialize the DS sensors
    startDS();
  #endif
  #if defined(enableHX)         // Initialize the DS sensors
    startHX();
  #endif
  led.setPixelColor(0, led.Color(LEDhigh,LEDhigh,LEDoff));
  led.show();
  numsensors = numtemp + numweight + numpress;
  Serial.printf("\n%ixIn %ixOut & %ixSensors for Temp: %i, Weight: %i, Pressure: %i\n", numin, numout, numsensors, numtemp, numweight, numpress);
  mqttclient.publish(out_sensors, (String(MySensors).c_str()), false); // Send list of the usable sensors.
}


//  This is the Main loop
void loop() {                             
  #if defined(OTA)                                  // Augment  OTA posibility
    checkOTA();
  #endif
  if (otaInProgress) return;                        // ← Während OTA alles andere stoppen  
  
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = x milliseconds) it reads a new MQTT message
  if (currentMillis - prevRMQTTMillis >= readinterval) {
    prevRMQTTMillis = currentMillis;

    if (WiFi.status() == WL_CONNECTED) { Serial.print("+");}
    else {
      WiFi.begin(wifi_ssid, wifi_password);
      Serial.println("Try Wifi reconnect "); 
    }
    
    if (!mqttclient.connected()) {
      reconnect();                                  // In case no mqtt it will reconnect
      mqttclient.subscribe(in_topic);
      Serial.print("Go in loop with MQTT topic: " );
      Serial.println(in_topic);
      #if defined(TEST)
        mqttclient.publish(out_status, "2" ,false);
      #else
        mqttclient.publish(out_status, "Reconnected" ,false);
      #endif
      statusreset = true;
    }
    mqttclient.loop();                              // Request if there is a message
    watchdog = !watchdog;                           // Create toggeling watchdog signal
    mqttclient.publish(out_watchdog, String(watchdog).c_str() ,false); 

    if (statusreset){
      statusreset = false;
      delay(1000);
      #if defined(TEST)
        mqttclient.publish(out_status, "0" ,false);
      #else
        mqttclient.publish(out_status, "Normal" ,false);
      #endif
    }
    // Here are the sensordata send via mqtt when exist
    // It is intentional that a send is only done within receive interval. (To avoid unchecked connection add with buffer problems)
    if (currentMillis - prevSMQTTMillis >= sendinterval) {
      prevSMQTTMillis = currentMillis;
      #if defined(TEST)
        Serial.printf("Free Heap: %d\n\n", ESP.getFreeHeap());  
      #endif
      readvalues();
    }
  }
  #if defined(enableIN)                                                             // Show bus devices on Serial if exist
    fastIN();                                           // Faster reaction of Input IO's
  #endif

  #if defined(TEST)
    // mqttclient.publish(out_loop, String(looped).c_str() ,false);     // Only use that when in doubts of loop speed
    // Serial.println(MySensors);
    looped++;
  #endif

  if (!digitalRead(RESTART_PIN)){ESP.restart();}      // Press top button to rrstart 
  
  delay(50);
}