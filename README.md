# ESP32-e-Paper-Weather-Display
An ESP32 and an ePaper Display reads [Open Weather Map](https://openweathermap.org/) and displays the weather

----
ORIGINAL DONE BY G6EJD

----

So please visit his page for the original software: https://github.com/G6EJD/ESP32-e-Paper-Weather-Display

----
ORIGINAL DONE BY G6EJD

----

Take a look at the V2 Marani Version. If you like it, download it, if not, take the original one from G6EJD

I just enhanced his version, like:
- Better alignment of Text and Graphics
- Better alignment of Forecast Multiline-Text
- Wind and Gust speeds
- Multi-Graphs to show more than one value (ex. Rain and Snow)
- Ajust-Graphs with moving x-axis descriptions for better readout of forecasts
- Added Moon-Set and Rise times, Distance, Zodiac, Age, Longitude and Latitude and Illumination
- Changed Battery-Display when no battery used
- Moved some Status-Infos to better suitable places
- Stronger lines in Weather Symbols and Wind graphics



----
ORIGINAL DONE BY G6EJD

----



----
ORIGINAL DONE BY G6EJD

----



----
ORIGINAL DONE BY G6EJD

----



----
ORIGINAL DONE BY G6EJD

----


Download the software to your Arduino's library directory.

1. From the examples, choose depending on your module either
   - Waveshare_7_5_T7 (newer 800x480 version of the older 640x384)
   
   Code requires [GxEPD2 library](https://github.com/ZinggJM/GxEPD2)
   - which needs [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library
   - an also requires U8g2_for_Adafruit_GFX

2. Obtain your [OWM API key](https://openweathermap.org/appid) - it's free

3. Edit the owm_credentials.h file in the IDE (TAB at top of IDE) and change your Language, Country, choose your units Metric or Imperial and be sure to find a valid weather station location on OpenWeatherMap, if your display has all blank values your location does not exist!

4. If your are using the older style Waveshare HAT then you need to use:
  
  **display.init(); //for older Waveshare HAT's 
  
  In the InitialiseDisplay() function, comment out the variant as required 

5. Save your files.

NOTE: See schematic for the wiring diagram, all displays are wired the same, so wire a 7.5" the same as a 4.2", 2.9" or 1.54" display! Both 2.13" TTGO T5 and 2.7" T5S boards come pre-wired.

The Battery monitor assumes the use of a Lolin D32 board which uses GPIO-35 as an ADC input, also it has an on-board 100K+100K voltage divider directly connected to the Battery terminals. On other boards, you will need to change the analogRead(35) statement to your board e.g. (39) and attach a voltage divider to the battery terminals. The TTGO T5 and T5S boards already contain the resistor divider on the correct pin.

Compile and upload the code - Enjoy!

7.5" V2 Marani 800x480 E-Paper Layout

![alt text width="600"](/Waveshare_7_5_V2_marani.jpg)

7.5" 800x480 E-Paper Layout

![alt text width="600"](/Waveshare_7_5_new.jpg)

7.5" 640x384 E-Paper Layout

![alt text width="600"](/Waveshare_7_5.jpg)


**** NOTE change needed for latest Waveshare HAT versions ****

Ensure you have the latest GxEPD2 library

See here: https://github.com/ZinggJM/GxEPD2/releases/

Modify this line in the code:

display.init(115200, true, 2); // init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration, bool pulldown_rst_mode)

Wiring Schematic for ALL Waveshare E-Paper Displays
![alt_text, width="300"](/Schematic.JPG)
