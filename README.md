# Planto-Manager

## Description: 
This is an embedded project aimed at monitoring and caring for a potted plant. Device is able to monitor room temperature and humidity using the [DHT11](https://www.adafruit.com/product/386). It can also measure the soil moisture by utilizing a personally designed soil moisture sensor, influenced by [Grove's Capacitor Soil Moisture Sensor](https://wiki.seeedstudio.com/Grove-Capacitive_Moisture_Sensor-Corrosion-Resistant/). Additionally, the device is able to track time and set alarms using the [DS3231](https://components101.com/modules/ds3231-rtc-module-pinout-circuit-datasheet) RTC module. A [16x2 Character LCD](https://www.sparkfun.com/products/709)  was used to display relevant information. Lastly, given the simplicity of the tasks and the low cost, the firmware was developed on an Atmega328P.

## Libraries:
  [Atmel Start Configuration Files](https://start.atmel.com/#dashboard)

## Images:

### Planto Manager System 
![Planto Manager System](https://raw.githubusercontent.com/pleteaud/Planto-Manager/master/Pictures/Planto%20Manager%20Modules%20Connected.jpg)

### Planto Manager System Wiring
![Planto Manager System Wiring](https://raw.githubusercontent.com/pleteaud/Planto-Manager/master/Pictures/Planto%20Manager%20Schematic.png)

### Soil Moisture Sensor PCB Layout
![Soil Moisture Sensor PCB Layout](https://raw.githubusercontent.com/pleteaud/Planto-Manager/master/Pictures/Soil%20Moisture%20PCB%20Layout.png)

### Soil Moisture Sensor Reading (Dry Case)
![Soil Moisture Sensor Reading (Dry Case)](https://raw.githubusercontent.com/pleteaud/Planto-Manager/master/Pictures/Soil%20Moisture%20Sensor%20Reading%20(Dry%20Case).jpg)

### Soil Moisture Sensor Reading (Moist Case)
![Soil Moisture Sensor Reading (Moist Case)](https://raw.githubusercontent.com/pleteaud/Planto-Manager/master/Pictures/Soil%20Moisture%20Sensor%20Reading%20(Moist%20Case).jpg)

## Future Plan
- [ ] Switch to Adafruit BME280 sensor since it supports I2C. This will free up pins and reduce the timing issues faced with the DHT11
- [ ] Implement a Keypad module to allow user to inter inferace with device out of debug mode.
- [ ] Fix Soil moisture sensor code bugs.
- [ ] Implement I2C I/O Expander (MCP23017-E/SP-ND) to reduce number of pins used by the LCD and Keypad
- [ ] Implement reworks and design upgrades to Soil Moisture Sensor
- [ ] Design and build container for device 
