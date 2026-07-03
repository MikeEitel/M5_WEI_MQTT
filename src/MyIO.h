// Used pins on the MCU board are defined in the config file
// Definition as input has always priority agains output to avoid short circuit mode.
// GND            GND
// 3V3            Supply max 3.3V power to inputs 
// IN0            Input max 3.3V
// IN1            Input max 3.3V
// OUT0           Output max allowed 3.3V
// OUT1           Output max allowed 3.3V


int IOmynumin = 0;                                      // Actual number of input pins
int IOmynumout = 0;                                     // Actual number of output pins
int IOnum = 0;                                          // Actual number of IO's that define the used pins
unsigned long prevIOMillis = 2764472319;                // Stores last time IO scan value ->  2764472319->FASTER START
unsigned long nowIOMillis;         

int myio[5]=  {99, 99, 99, 99, 99};                     // Preset hw gpio number generell
int myin[5]=  {99, 99, 99, 99, 99};                     // Preset hw gpio number inputs
int myout[5]= {99, 99, 99, 99, 99};                     // Preset hw gpio number outputs
bool lastin[5]= {false, false, false, false, false};    // Storage for last read input value


// Helper function to identify used IO pins
void showIO() {
#if defined(enableIN) || defined(enableOUT)
  auto printRow = [](const char* label, const int* array, int size) {
    Serial.printf("\n%s:\t", label);
    for (int i = 0; i < size; i++) {
     Serial.printf("%i\t", array[i]);
    }
  };
  printRow("IO", myio, 5);
  printRow("In", myin, 5);
  printRow("OUT", myout, 5);
  Serial.println();
#else
  Serial.print("\nNo IO devices:\n");
#endif
}


void startIO(){
  int IOoff = 0;                    // Offset if Inputs are defined
  #if defined(enableIN)             // If on the same pin input is defined it has priority
    for (int i = 0; i < INcount; i++) {
      pinMode(IO[i], INPUT);
      myio[i] = IO[i];
      myin[i] = IO[i];
      IOnum++;
      IOmynumin++;
    }
  #endif
  #if defined(enableOUT)            // If on the same pin input is defined it has priority
    #if defined(enableIN)           // In case there is no input
      IOoff = IOnum;                // Make sure outputs start AFTER inputs
    #endif
    for (int i = 0; i < OUTcount; i++) {
      pinMode(IO[IOoff + i], OUTPUT);
      myio[IOoff + i] = IO[IOoff + i];
      myout[i] = IO[IOoff + i];
      IOnum++;
      IOmynumout++;
    }
  #endif

  MySensors = MySensors + IOmynumin + "xIN " + IOmynumout + "xOUT ";
  Serial.printf("Found %s\n", MySensors.c_str());
  numin = IOmynumin;
  numout = IOmynumout;
  #if defined(TEST)
    showIO();
  #endif
}

void readIO(){
  MySensors += "I<";
  for (int i = 0; i < IOmynumin; i++){
      MySensors += i;
    }
  MySensors += "> O<"; 
  for (int i = 0; i < IOmynumout; i++){
    MySensors += i;
  }
  MySensors += "> + ";

  //digitalWrite(IO[myout[0]], !digitalRead(IO[myout[0]]));
}

void fastIN(){
  #if defined(enableIN)
    bool in = false;
    nowIOMillis = millis();
    if (nowIOMillis - prevIOMillis <= INinterval) {
      return;
    }
    prevIOMillis = nowIOMillis;
    #if defined(TEST)
      for (int i = 0; i < IOmynumin; i++){
        Serial.printf("IO%i= %x\t", IO[i], digitalRead(IO[i]));
      }
      Serial.println();
    #endif   
    for (int i = 0; i < IOmynumin; i++){
      in = digitalRead(IO[i]);
      if (lastin[i] != in){
        String numh = String(i);
        String topic = String(mqtt_out_sen) + "/IN/" + numh;
        mqttclient.publish(topic.c_str(), String(in).c_str(), false);
      }
      lastin[i] = in;
    }
  #endif
}

void sendIN(){
  #if defined(TEST)
    Serial.print("Status ");
    for (int i = 0; i < IOmynumin; i++)
    {
      Serial.printf("In%i(%i)= %i\t", i, myin[i], digitalRead(myin[i]));
    }
      Serial.println();
  #endif   
  for (int i = 0; i < IOmynumin; i++){
    String numh = String(i);
    String topic = String(mqtt_out_sen) + "/IN/" + numh;
    mqttclient.publish(topic.c_str(), String(digitalRead(myin[i])).c_str(), false);
  }
}

void sendOUT(){
  #if defined(TEST)
    Serial.print("Status ");
    for (int i = 0; i < IOmynumout; i++){
      Serial.printf("Out%i(%i)= %i\t", i, myout[i], digitalRead(myout[i]));
    }
    Serial.println();
  #endif   
  for (int i = 0; i < IOmynumout; i++) {
    String numh = String(i);
    String topic = String(mqtt_out_sen) + "/OUT/" + numh;
    mqttclient.publish(topic.c_str(), String(digitalRead(myout[i])).c_str(), false);
    Serial.printf("Out%i(%i)= %i\t", i, myout[i], digitalRead(myout[i]));
  }
}
void setIO(int out, int outval ){
  //#if defined(TEST)
    Serial.printf("Commanded Pin: %i  Val: %i\n", out, outval);
  //#endif
  if (out <= IOmynumout-1){
    switch (outval){
      case 0 : {
        digitalWrite(myout[out], false);
        break; }   
      case 1 : {
        digitalWrite(myout[out], true);
        break; }    
      default : {
        Sendme = "False value: " + String(outval) + " for commanded Pin:" + String(out);
        Serial.println(Sendme);
        mqttclient.publish(out_status, (String(Sendme).c_str()), false); 
      }
    }
  } 
  else {
    Sendme = "False commanded Pin" + String(out);
    Serial.println(Sendme);
    mqttclient.publish(out_status, (String(Sendme).c_str()), false); 
  }
}

