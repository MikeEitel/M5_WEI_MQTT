// Used pins 
// GND            GND
// SCK            Supply 3.3V power to the sensors
// DT             Connect data of the sensors

int HXmynumsen = 0;
int HXcounted = 0;
long raw = 0;
String numh;

#include <HX711.h>
HX711 HXread[HXcount];                                       // Array for HX71x chips 

// HXread Arrays
const uint8_t DAT[] = {DT1, DT2, DT3, DT4};
const uint8_t CLK_PIN[] = {SCK, SCK, SCK, SCK};       // Same pin for all
long HXLastsend[HXcount];                             // This variable remembers the last send measurement

void startHX() {
  HXmynumsen = numweight; 
  for (int i = 0; i < HXcount; i++) {
    HXread[i].begin(DAT[i], CLK_PIN[i]);
    delay(500);
    HXread[i].power_up();

    if (HXread[i].wait_ready_timeout(1000)) {
      HXread[i].set_gain(128, false);                  // (128, 64 or 32; Overwrite when not already set)
      HXread[i].set_offset(HX71x_OFFSET[i]);  
      HXread[i].set_scale(HX71x_DIVIDER[i]);
      HXread[i].set_medavg_mode(); // HXread[i].set_runavg_mode(); //HXread[i].set_raw_mode(); HXread[i].set_average_mode()// ;
      raw = HXread[i].get_units(1);
      Serial.printf("Activated %i.HX71x with: %f\n", i + 1, raw);
      if(HX71x_Typ[i]=='W')   
        { numweight++; }
      else                    
        { numpress++;  }
      HXcounted++;
    } else {
      #if defined(TEST)
        Serial.printf("Failed to start %i.HX71x\n", i + 1);
        char errMsg[20];
        snprintf(errMsg, sizeof(errMsg), "%i.HX71x ERROR", i + 1);
        mqttclient.publish(out_status, errMsg, false);
        MySensors += "Error " + String(i + 1) + ".HX71x";
      #endif
    }
  }
  if ( HXcounted > 0){
    MySensors += String(HXcounted) + "xHX71x  ";
  }
}

void sendHX2mqtt(char HXtyp){
  if(HXtyp=='W'){
    String topic = String(mqtt_out_sen) + "/weight/" + numh;
    mqttclient.publish(topic.c_str(), String(raw).c_str(), false);
    }
  else {
    String topic = String(mqtt_out_sen) + "/pressure/" + numh;
    mqttclient.publish(topic.c_str(), String(raw).c_str(), false);
  }
}
 
//      if(HX71x_Typ[i]=='W'){ numweight++; }
//      else { numpress++; }


void readHX(){
  if ( HXcounted > 0){
    MySensors += "HX<";
  }
  for (int i = 0; i < HXcounted; i++) { 
    if (HXread[i].wait_ready_timeout(1000)){
      raw = HXread[i].get_units(3);
      // Publish readings with unique MQTT topics per sensor
      numh = String(HXmynumsen + i);                // Starting mqtt at /0
      if(HX71x_DELTA[i] == 0){                      // Send every time based cycle
        sendHX2mqtt(HX71x_Typ[i]);
        HXLastsend[i] = raw;
      }
      else  {                                       // Send in cycle but only when bigger as delta
        Serial.printf("\tL:%ld W:%ld D:%ld\n", abs(HXLastsend[i]), abs(raw), HX71x_DELTA[i]);
        if ((abs(HXLastsend[i] - raw)) > HX71x_DELTA[i]) {
          HXLastsend[i] = raw;
          sendHX2mqtt(HX71x_Typ[i]);
        }
      }
      
      MySensors += numh;
      #if defined(TEST)
        Serial.printf("%i.HX71x: %ld measured\n",i+1 ,raw);
      #endif 
      }
    else {
      #if defined(TEST)
        mqttclient.publish(out_status, "-5" ,false);
        Serial.printf("%i HX71x unavailable\n",i);
      #else
        Serial.printf("%i HX71x unavailable\n",i);
        mqttclient.publish(out_status, "HX71x unavailable" ,false);
        led.setPixelColor(0, led.Color(LEDhigh,LEDoff,LEDoff));
        led.show();
      #endif
    }

    #if defined(TEST)
      //Serial.printf("DHT T: %f C\tF: %f %%\n",temp , hum);
    #endif

    delay(10);                                          // Give cpu time to update watchdog
  } 
  MySensors += "> ";  
}

