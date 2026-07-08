/*Made by Mike Eitel to store device credenitals on one place
  No rights reserved when private use. Otherwise contact author.

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
  and associated documentation files. The above copyright notice and this permission notice shall 
  be included in all copies or substantial portions of the software.
*/

// Wifi acess definitions of network to connect to  
#define wifi_ssid "xxx"
#define wifi_password "yyy"

#define myIP "100"                                    // Device ID

// Wifi definitions of this device  
//IPAddress staticIP(192,168,x,atoi(myIP));           // IOT device IP
IPAddress staticIP(1,1,1,atoi(myIP));      // REPLACE !!!!
IPAddress subnet(255,255,255,0);                      // Network subnet size
//IPAddress gateway(192,168,x,y);                     // Network router IP
IPAddress gateway(1,1,1,1);                // REPLACE !!!!

// Mosquitto MQTT Broker definitions
#define mqtt_server    "192.168.x.z"                  // IOT MQTT server IP
#define mqtt_user      "admin"
#define mqtt_password  "admin"
#define mqtt_port      1883
#define WiFi_timeout    101                           // How many times to try before give up
#define mqtt_timeout     11                           // How many times to try before try Wifi reconnect
// MQTT Topics
#define mytype          "esp/M5-WEI-"                   // Client Typ
#define iamclient       mytype MyIP                     // Client name 
#define in_topic        iamclient "/command"            // This common input is received from MQTT
#define out_param       iamclient "/signal"             // Wifi signal strength is send to MQTT
#define out_status      iamclient "/status"             // This is a general message send to MQTT
#define out_watchdog    iamclient "/watchdog"           // A watchdog bit send to MQTT
#define out_sensors     iamclient "/sensors"            // This is a list of usable external sensors send to MQTT
#define out_qual        iamclient "/qual"               // Measurement quality is send to MQTT
#define out_loop        iamclient "/loop"               // This helper debug variable can be send to MQTT
#define mqtt_out_sen    iamclient ""                   // This is sensor data placeholder to send to MQTT

const char *mqtt_out_weight[4] = {
    iamclient "/ext_weight/0", // This is send to MQTT
    iamclient "/ext_weight/1", // This is send to MQTT
    iamclient "/ext_weight/2", // This is send to MQTT
    iamclient "/ext_weight/3"  // This is send to MQTT
};

// Errors send as values in test mode
// error =  -7
// error =  -6 
// error =  -5
// error =  -4    No DS18B20 sensors
// error =  -3    DHT Humidity sensor not read
// error =  -2    DHT Temperatur sensor not read
// error =  -1    Wrong command received
// error =   0    Normal status
// error =   1    MQTT first time connected
// error =   2    MQTT Reconnect succesfull
// error =   3    Command received

int LEDbrightness =          96;                        // Brightness of the modules buildin led

// Allowed are:  DHT 11  or   DHT 12  or   DHT 22 (AM2302 / AM2321)   or   DHT 21 (AM2301)
#define DHTtyp   DHT22  

// See above
const long HX711_OFFSET[]  =    {0,0,0,0};         // Tara compensation
const long HX711_DIVIDER[] =    {1,1,1,1};              // Scale sensitivity
long HX711_DELTA[] =            {0,0,0,0};              // Basic sensitivity to send new mqtt value

#define HXcount   4                                     // How many HX711 are active. Max 4 forseen 
#define INcount   1                                     // How many Inputs are active. Max 5 forseen (Automatic restricted = 5 - HXcount)
#define OUTcount  1                                     // How many Inputs are active. Max 5 forseen (Automatic restricted = 5 - HXcount -INcount)

#if (HXcount + INcount + OUTcount) > 6 
  #error "Not Enough pins!"
#endif

// Enable the prepart sensors
#define enableDHT                                       // Enable DHT T&H sensor      
#define enableDS                                        // Enable multiple DS18 T sensors      
#define enableHX                                        // Enable HX711 weight sensors      
#define enableIN                                        // Enable inputs     
#define enableOUT                                       // Enable outputs    

// Constant how often the mqtt message is send
#if defined(TEST)
  long readinterval =  1500;                            // Interval at which device does MQTT reading
  long sendinterval =  3000;                            // Interval at which sensor data is send via mqtt
  long INinterval =     300;                            // Interval at which IO's are treated
#else
  long readinterval =  2000;   // 60000;               // Interval at which device does MQTT reading
  long sendinterval =  5000;                           // Interval at which sensor data is send via mqtt
  long INinterval =     500;                            // Interval at which IO's are treated
#endif
