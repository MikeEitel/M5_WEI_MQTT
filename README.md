# M5_WEI_MQTT
M5stack based device to especialy read up to 4 HX71x chips and other sensors....

It is possible to measure temperature and humidity with 1xDHT22, xDS18B20.
Additional you can use all unused GPIO pins as either in or output

This version is based on the M5stack-3CU device but can be easy transfer to any other ESP-C3 etc..

The results are transfered via mqtt.
Best to findout what can be done with mqtt is to use the MQTT-Explorer app.

The commands that you mostly use are ? to get multiple info's  and X for soft restart of the device.

What is received as command is defined in: 
void callback(char* topic, uint8_t* payload, unsigned int length)  {....}
    case '?' : // Request parameters
    case 'D' : // Send mqtt on value change
    case 'I' : // Read input
    case 'O' : // Serialprint I2C device and IO pin direction list
    case 'Q' : // Write output
    case 'U' : // Set new update rate 
    case 'X' : // Remote RESTART


In the credentials.h  you define what sensoric to use plus address and network / mqtt parameters.. 
This device can read all sensors in the same device at the same time. 
The number of DS18B20 is not restricted but not tested with more as 5.
When used with 4xHX71x 1xDHT22 nxDS18B20 two pins can be used for IN- or OUT-put

As these devices are used by me in multiple networks the .ino also defines whitch credential file for a network is to use.

I dislike DHCP in static IOT networks and the last IP digit is therefore used to define the mqtt name. Easier for debugging. 
So in a correct credential file only the one number in   #define myIP "100"   is enough to compile another device.
