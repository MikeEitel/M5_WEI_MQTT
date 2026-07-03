# M5_WEI_MQTT

M5Stack-based device to especially read up to 4 HX71x chips and other sensors.

It is possible to measure temperature and humidity with 1xDHT22 and xDS18B20.  
Additionally, you can use all unused GPIO pins as either input or output.

This version is based on the M5Stack-3CU device but can be easily transferred to any other ESP-C3 etc.

The results are transferred via MQTT.  
The best way to find out what can be done with MQTT is to use the **MQTT-Explorer** app.

The commands that you mostly use are `?` to get multiple infos and `X` for a soft restart of the device.

### Supported Commands

What is received as a command is defined in:  
`void callback(char* topic, uint8_t* payload, unsigned int length) {...}`

```cpp
case '?': // Request parameters
case 'D': // Send mqtt on value change
case 'I': // Read input
case 'O': // Serialprint I2C device and IO pin direction list
case 'Q': // Write output
case 'U': // Set new update rate 
case 'X': // Remote RESTART
```

### Configuration

In the `credentials.h` you define what sensors to use plus the address and network / MQTT parameters.  
This device can read all sensors in the same device at the same time.  
The number of DS18B20 is not restricted, but it has not been tested with more than 5.  
When used with 4xHX71x, 1xDHT22, and nxDS18B20, two pins can still be used for INPUT or OUTPUT.

As these devices are used by me in multiple networks, the `.ino` file also defines which credential file to use for a specific network.

I dislike DHCP in static IoT networks, so the last IP digit is used to define the MQTT name. This makes debugging much easier.  
Therefore, in a correct credential file, modifying just the single number in `#define myIP "100"` is enough to compile another device.
