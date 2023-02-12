# ESP32-e-Paper-Weather-Display
![alt text width="600"](/pictures/amaWeatherStationComplete.jpg)
An ESP32 and an ePaper Display reads [Open Weather Map](https://openweathermap.org/) and displays the weather

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
- Fixed the positioning of weekday labels to be moved pixelperfect
- Added vertical lines in all historys to get a better differntiation between days

Additional Stuff:
- 3D Case
- Custom made PCB, now with CH9102F fix (cheap CP2104 alternativ UART Chip), you're able to use both chips now
- Contains all needed stuff for battery usage (battery sensor, charge-unit, buck-boost converter)


Download the software to your Arduino's library directory.

1. From the examples, choose depending on your module either
   - Waveshare_7_5_T7 (newer 800x480 version of the older 640x384)
   
   Code requires [GxEPD2 library](https://github.com/ZinggJM/GxEPD2)
   - which needs [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library
   - an also requires U8g2_for_Adafruit_GFX

2. Obtain your [OWM API key](https://openweathermap.org/appid) - it's free

3. Edit the owm_credentials.h file in the IDE (TAB at top of IDE) and change your Language, Country, choose your units Metric or Imperial and be sure to find a valid weather station location on OpenWeatherMap, if your display has all blank values your location does not exist!

4. Save your files.

Compile and upload the code - Enjoy!

7.5" V2 Marani 800x480 E-Paper Layout

![alt text width="600"](/pictures/Waveshare_7_5_Opti_1.jpg)

![alt text width="600"](/pictures/Waveshare_7_5_V2_marani.jpg)

PCB
![alt text width="600"](/pictures/amaWeatherStationEpaper_CH9102F_fix.png)

Schematics
![alt text width="600"](/pictures/amaWeatherStationSchematics1.1.png)

Cabling
![alt text width="600"](/pictures/amaWeatherStationPCB_Cabling.jpg)

3D Casing
![alt text width="600"](/pictures/amaWeatherStationComplete.jpg)

7.5" Old 800x480 E-Paper Layout

![alt text width="600"](/pictures/Waveshare_7_5_new.jpg)

7.5" Old 640x384 E-Paper Layout

![alt text width="600"](/pictures/Waveshare_7_5.jpg)
