# w_Display_CYD
A calendar on a CYD (ESP32-Cheap-Yellow-Display)

[Deutsche Version](README.md)

## Calendar on an ESP32-Cheap-Yellow-Display:
This project is based on Arduino and runs on an ESP32 microcontroller. <br>
It is very easy to build as the project uses ready-made components <br>
that can be implemented without soldering.

## Lizenz:
This project is based on the MIT License (Non-Commercial): <br>
Copyright (c) 2025 7soft <br>
 <br>
Permission is hereby granted, free of charge, to any person obtaining a copy <br>
of this software and associated documentation files (the "Software"), to deal <br>
in the Software without restriction, including without limitation the rights <br>
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell <br>
copies of the Software, subject to the following conditions: <br>
 <br>
The above copyright notice and this permission notice shall be included in all <br>
copies or substantial portions of the Software. <br>
 <br>
**Non-Commercial Clause:** <br>
The Software may not be used for commercial purposes. <br> 
Commercial purposes include, but are not limited to, selling or licensing the Software or any derivative works thereof for a fee, or using the Software to provide services for a fee. <br>
 <br>
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR <br>
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, <br>
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE <br>
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER <br>
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, <br>
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE <br>
SOFTWARE.

## Features:
 - Display of public holidays.
 - Display of birthdays or other anniversaries.
 - Display of school holidays.
 - Configuration via config.txt file on a micro SD card.
 - Display of a clock with sunrise and sunset times.
 - Freely configurable for other time zones and locations (SD card).

![finished device](IMG_20250106_183430.jpg_compressed.JPEG) <br>
![finished device](IMG_20250106_183649.jpg_compressed.JPEG)

## Hardware Tutorial:
- https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/
- https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/
- https://github.com/paelzer/ESP32-Cheap-Yellow-Display-Documentation-DE
- https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

![finished device](IMG_20250106_201539.jpg_compressed.JPEG) <br> 
![finished device](IMG_20250106_201550.jpg_compressed.JPEG) 

## Arduino Library:
- SPI.h <br>
Necessary for communication with the SPI interface to address the display.
- XPT2046_Touchscreen.h <br>
Library to communicate with the touchscreen.
- TFT_eSPI.h <br>
Graphics library to control the LCD display.

## Configuration of the TFT_eSPI.h Library:
The library can be used for different types of displays. <br>
The Cheap-Yellow-Display comes in two different variants with slightly different displays. <br>
Therefore, it is necessary to adjust the library before compiling.

### Variant with one USB interface (USB-2.0-Micro socket)
In this case, the User_Setup_USB1.h must be renamed to User_Setup.h and then copied to the libraries\TFT_eSPI folder.

### Variant with two USB interfaces (USB-2.0-Micro socket + USB-3.1-Type-C socket)
In this case, the User_Setup_USB2.h must be renamed to User_Setup.h and then copied to the libraries\TFT_eSPI folder.

## User Configuration of the Calendar/Clock:
To make the configuration of the calendar/clock as easy as possible, it is stored on an SD card. <br>
The config.txt file must be saved on the SD card. <br>
This file contains the following settings:

### ssid=
The SSID of the WiFi network.
### password=
The password of the WiFi network.
### tzinfo=
The time zone and daylight saving time information to determine the local time of the location. <br>
[Examples](tzinfo.txt)
### ntpserver=
The NTP time server from which the exact current time is obtained.
### tformat=
Display of the time in 12 or 24-hour format. (12/24)
### brightness=
The initial brightness of the display backlight. (0-255)
### function=
Which display the software should start with. (0: Calendar, 1: Clock)
### latitude=
Latitude of the location to determine sunrise and sunset times.
### longitude=
Longitude of the location to determine sunrise and sunset times.
### WeekDays=
List of weekdays to adapt them to another language.
### MonthName=
List of month names to adapt them to another language.
### Translate=
Another list of displays to adapt them to another language. 
### birthday=
The birthday list. (Date-Name,...)
### Year=
The year lists stored in the configuration. There must always be 3 consecutive years. (Year0, Year1, Year2)
### holiday0=
List of public holidays for Year0. (Date-Description,...) <br>
[List of federal states of Germany 2025](holiday_2025.txt)
### holiday1=
List of public holidays for Year1. (Date-Description,...) <br>
[List of federal states of Germany 2026](holiday_2026.txt)
### holiday2=
List of public holidays for Year2. (Date-Description,...) <br>
[List of federal states of Germany 2027](holiday_2027.txt)
### schoolhol0=
List of school holidays for Year0. (Date,...) 
### schoolhol1=
List of school holidays for Year1. (Date,...)
### schoolhol2=
List of school holidays for Year2. (Date,...)

## Touch Operation:
![Operation](Touch_Kal.jpg)  <br>
### GREEN
Switches to the calendar view for the current month.
### BLUE
Switches to the next calendar month step by step up or down.
### YELLOW
Switches to the clock view.
### RED
Adjusts the display backlight brightness.

## YouTube Link:
https://www.youtube.com/watch?v=C9uFmEZfyEc

## ESP Web Tools:
With this web tool, you can flash the CYD with the latest firmware.  <br>
Please select USB1 for the hardware variant with one USB interface and USB2 for the variant with two USB interfaces. <br>
[Flash mit ESP Web Tools](https://7soft.de/esp_w_display_cyd/flash_x.html)

