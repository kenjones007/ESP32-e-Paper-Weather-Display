/* ESP32 Weather Display using an EPD 7.5" 800x480 Display, obtains data from Open Weather Map, decodes and then displays it.
  ####################################################################################################################################
  This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.

  Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
  1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
  2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
  3. You may not, except with my express written permission, distribute or commercially exploit the content.
  4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

  The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
  software use is visible to an end-user.

  THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
  OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  See more at http://www.dsbird.org.uk

   Modifications done by Alessandro Marani
  - Better alignment of Text and Graphics
  - Better alignment of Forecast Multiline-Text
  - Wind and Gust speeds
  - Multi-Graphs to show more than one value (ex. Rain and Snow)
  - Ajust-Graphs with moving x-axis descriptions for better readout of forecasts
  - Added Moon-Set and Rise times, Distance, Zodiac, Age, Longitude and Latitude and Illumination
  - Changed Battery-Display when no battery used
  - Moved some Status-Infos to better suitable places
  - Stronger lines in Weather Symbols and Wind graphics
  - Fixed Moon drawing routine to avoid drawing strayed pixels
*/

#include <u8g2_fonts.h>
#define BOX_HEADER 20

#include "owm_credentials.h"          // See 'owm_credentials' tab and enter your OWM API key and set the Wifi SSID and PASSWORD
#include <ArduinoJson.h>              // https://github.com/bblanchon/ArduinoJson needs version v6 or above
#include <WiFi.h>                     // Built-in
#include <TimeLib.h>                  // Built-in
#include <SPI.h>                      // Built-in 
#define  ENABLE_GxEPD2_display 1
#include <GxEPD2_BW.h>
//#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "src/epaper_fonts.h"
#include "src/forecast_record.h"
#include "src/lang.h" 

#include "MoonRise.h"
#include "MoonPhase.h"

// Localisation (English)
//#include "lang_cz.h"                  // Localisation (Czech)
//#include "lang_fr.h"                  // Localisation (French)
//#include "lang_gr.h"                  // Localisation (German)
//#include "lang_it.h"                  // Localisation (Italian)
//#include "lang_nl.h"
//#include "lang_pl.h"                  // Localisation (Polish)

#define SCREEN_WIDTH  800             // Set for landscape mode
#define SCREEN_HEIGHT 480

enum alignment { LEFT, RIGHT, CENTER };

// Connections for e.g. LOLIN D32
static const uint8_t EPD_BUSY = 4;  // to EPD BUSY
static const uint8_t EPD_CS = 5;  // to EPD CS
static const uint8_t EPD_RST = 16; // to EPD RST
static const uint8_t EPD_DC = 17; // to EPD DC
static const uint8_t EPD_SCK = 18; // to EPD CLK
static const uint8_t EPD_MISO = 19; // Master-In Slave-Out not used, as no data from display
static const uint8_t EPD_MOSI = 23; // to EPD DIN

// Connections for e.g. Waveshare ESP32 e-Paper Driver Board
//static const uint8_t EPD_BUSY = 25;
//static const uint8_t EPD_CS   = 15;
//static const uint8_t EPD_RST  = 26; 
//static const uint8_t EPD_DC   = 27; 
//static const uint8_t EPD_SCK  = 13;
//static const uint8_t EPD_MISO = 12; // Master-In Slave-Out not used, as no data from display
//static const uint8_t EPD_MOSI = 14;

GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY));   // B/W display
//GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT> display(GxEPD2_750(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY)); // 3-colour displays
// use GxEPD_BLACK or GxEPD_WHITE or GxEPD_RED or GxEPD_YELLOW depending on display type

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
// Using fonts:
// u8g2_font_helvB08_tf
// u8g2_font_helvB10_tf
// u8g2_font_helvB12_tf
// u8g2_font_helvB14_tf
// u8g2_font_helvB18_tf
// u8g2_font_helvB24_tf

//################  VERSION  ###########################################
String version = "16.11";    // Programme version, see change log at end
//################ VARIABLES ###########################################

boolean LargeIcon = true, SmallIcon = false;
#define Large  17           // For icon drawing, needs to be odd number for best effect
#define Small  6            // For icon drawing, needs to be odd number for best effect
String  Time_str, Date_str; // strings to hold time and received weather data
int     wifi_signal, CurrentDay = 0,  CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
long    StartTime = 0;

//################ PROGRAM VARIABLES and OBJECTS ################

MoonRise mr;
_MoonPhase m;


#define max_readings 24

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];

#include "src/common.h"
#include "ESP32-e-Paper-Weather-Display.h"

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false
#define drawgraph_on   true
#define drawgraph_off  false

float pressure_readings[max_readings] = { 0 };
float temperature_readings[max_readings] = { 0 };
float temperature_feel_readings[max_readings] = { 0 };
float humidity_readings[max_readings] = { 0 };
float rain_readings[max_readings] = { 0 };
float snow_readings[max_readings] = { 0 };
float wind_readings[max_readings] = { 0 };
float gust_readings[max_readings] = { 0 };

long SleepDuration = 30; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupTime = 7;  // Don't wakeup until after 07:00 to save battery power
int  SleepTime = 23; // Sleep after (23+1) 00:00 to save battery power

//#########################################################################################
void setup() {
    StartTime = millis();
    Serial.begin(115200);
    if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
        if ((CurrentHour >= WakeupTime && CurrentHour <= SleepTime) || DebugDisplayUpdate) {
            InitialiseDisplay(); // Give screen time to initialise by getting weather data!
            byte Attempts = 1;
            bool RxWeather = false, RxForecast = false;
            WiFiClient client;   // wifi client object
            while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { // Try up-to 2 time for Weather and Forecast data
                if (RxWeather == false) RxWeather = obtain_wx_data(client, "weather");
                if (RxForecast == false) RxForecast = obtain_wx_data(client, "forecast");
                Attempts++;
            }
            if (RxWeather && RxForecast) { // Only if received both Weather or Forecast proceed
                StopWiFi(); // Reduces power consumption
                DisplayWeather();
                display.display(false); // Full screen update mode
            }
        }
    }
    BeginSleep();
}
//#########################################################################################
void loop() { // this will never run!
}
//#########################################################################################
void BeginSleep() {
    display.powerOff();
    long SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)); //Some ESP32 are too fast to maintain accurate time
    esp_sleep_enable_timer_wakeup((SleepTimer + 20) * 1000000LL); // Added extra 20-secs of sleep to allow for slow ESP32 RTC timers
#ifdef BUILTIN_LED
    pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
    digitalWrite(BUILTIN_LED, HIGH);
#endif
    Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
    Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
    Serial.println("Starting deep-sleep period...");
    esp_deep_sleep_start();      // Sleep for e.g. 30 minutes
}
//#########################################################################################
void DisplayWeather() {                        // 7.5" e-paper display is 800x480 resolution
    DisplayGeneralInfoSection();                 // Top line of the display
    DisplayDisplayWindSection(0, 18, 216, 242 + 66, WxConditions[0].Winddir, WxConditions[0].Windspeed, WxConditions[0].Gust, 81);

    DisplayConditionsSection(216 + 1, 18, 173, 226, WxConditions[0].Icon, LargeIcon);
    DisplayTemperatureSection(216 + 173 + 1 + 1, 18, 137, 100);
    DisplayPressureSection(216 + 173 + 137 + 1 + 1 + 1, 18, 137, 100, WxConditions[0].Pressure, WxConditions[0].Trend);
    DisplayPrecipitationSection(216 + 173 + 137 + 137 + 1 + 1 + 1 + 1, 18, 133, 100);
    DisplayForecastTextSection(216 + 173 + 1 + 1, 18 + 100 + 1, 409, 65);

    DisplayForecastSection(216 + 1, 245);            // 3hr forecast boxes
    DisplayAstronomySection(216 + 173 + 1 + 1, 100 + 69); // Astronomy section Sun rise/set, Moon phase and Moon icon
    DisplayStatusSection(690, 215, wifi_signal); // Wi-Fi signal strength and Battery voltage
}
//#########################################################################################
void DisplayGeneralInfoSection() {
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    drawString(SCREEN_WIDTH / 2, 0, City, CENTER);
    drawString(0, 0, Date_str + " (" + Time_str + ")", LEFT);
}
//#########################################################################################
void drawBox(int x, int y, int w, int h, int cx, String text) {
    display.drawRect(x, y, w, h, GxEPD_BLACK);
    display.drawLine(x, y + BOX_HEADER, x + w - 1, y + BOX_HEADER, GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    drawString(cx, y + 4, text, CENTER);
}

//#########################################################################################
void DisplayDisplayWindSection(int x, int y, int w, int h, float angle, float windspeed, float gust, int Cradius) {
    int cx = x + (w / 2);
    int cy = y + BOX_HEADER + Cradius + 20;

    drawBox(x, y, w, h, cx, TXT_WIND_SPEED_DIRECTION);

    display.fillCircle(cx, cy, Cradius + 1, GxEPD_BLACK); // Draw compass circle
    display.fillCircle(cx, cy, Cradius - 1, GxEPD_WHITE); // Draw compass circle
    display.drawCircle(cx, cy, Cradius * 0.7, GxEPD_BLACK); // Draw compass inner circle
    arrow(cx, cy, Cradius - 22, angle, 18, 33); // Show wind direction on outer circle of width and length

    int dxo, dyo, dxi, dyi;
    for (float a = 0; a < 360; a = a + 22.5) {
        dxo = Cradius * cos((a - 90) * PI / 180);
        dyo = Cradius * sin((a - 90) * PI / 180);
        if (a == 45)  drawString(dxo + cx + 12, dyo + cy - 12, TXT_NE, CENTER);
        if (a == 135) drawString(dxo + cx + 7, dyo + cy + 6, TXT_SE, CENTER);
        if (a == 225) drawString(dxo + cx - 18, dyo + cy, TXT_SW, CENTER);
        if (a == 315) drawString(dxo + cx - 18, dyo + cy - 12, TXT_NW, CENTER);
        dxi = dxo * 0.9;
        dyi = dyo * 0.9;
        display.drawLine(dxo + cx, dyo + cy, dxi + cx, dyi + cy, GxEPD_BLACK);
        dxo = dxo * 0.7;
        dyo = dyo * 0.7;
        dxi = dxo * 0.9;
        dyi = dyo * 0.9;
        display.drawLine(dxo + cx, dyo + cy, dxi + cx, dyi + cy, GxEPD_BLACK);
    }

    drawString(cx, cy - Cradius - 15, TXT_N, CENTER);
    drawString(cx, cy + Cradius + 3, TXT_S, CENTER);
    drawString(cx - Cradius - 12, cy - 3, TXT_W, CENTER);
    drawString(cx + Cradius + 10, cy - 3, TXT_E, CENTER);
    drawString(cx, cy - 43, WindDegToDirection(angle), CENTER);
    drawString(cx, cy + 30, String(angle, 0) + "°", CENTER);
    if (Units == "M") { windspeed *= 3.6; gust *= 3.6; } // m/s to km/h which is more common
    drawValue(cx, cy - 20, String(windspeed, 1), Units == "M" ? "km/h" : "mph", u8g2_font_helvB18_tf, u8g2_font_helvB08_tf, CENTER);
    drawValue(cx, cy + 5, "("+String(gust, 1)+")", Units == "M" ? "km/h" : "mph", u8g2_font_helvB12_tf, u8g2_font_helvB08_tf, CENTER);

    // draw wind forecast
    for (uint8_t r = 0; r < max_readings; r++) {
        if (Units == "M") wind_readings[r] = WxForecast[r].Windspeed * 3.6; else wind_readings[r] = WxForecast[r].Windspeed;
        if (Units == "M") gust_readings[r] = WxForecast[r].Gust * 3.6; else gust_readings[r] = WxForecast[r].Gust;
    }

    float y1min, y1max, y2min, y2max;
    calcMinMaxFromArray(max_readings, wind_readings, y1min, y1max); // Calc min max of 2 Array to show them both correctly in one graph
    calcMinMaxFromArray(max_readings, gust_readings, y2min, y2max);
    DrawGraph(34, y+h-70, 164, 55, _min(y1min, y2min), _max(y1max, y2max), Units == "M" ? TXT_WINDGUST_M : TXT_WINDGUST_I, wind_readings, max_readings, autoscale_off, barchart_off, drawgraph_on);
    DrawGraph(34, y+h-70, 164, 55, _min(y1min, y2min), _max(y1max, y2max), Units == "M" ? TXT_WINDGUST_M : TXT_WINDGUST_I, gust_readings, max_readings, autoscale_off, barchart_off, drawgraph_off);
}
//#########################################################################################
String WindDegToDirection(float winddirection) {
    if (winddirection >= 348.75 || winddirection < 11.25)  return TXT_N;
    if (winddirection >= 11.25 && winddirection < 33.75)  return TXT_NNE;
    if (winddirection >= 33.75 && winddirection < 56.25)  return TXT_NE;
    if (winddirection >= 56.25 && winddirection < 78.75)  return TXT_ENE;
    if (winddirection >= 78.75 && winddirection < 101.25) return TXT_E;
    if (winddirection >= 101.25 && winddirection < 123.75) return TXT_ESE;
    if (winddirection >= 123.75 && winddirection < 146.25) return TXT_SE;
    if (winddirection >= 146.25 && winddirection < 168.75) return TXT_SSE;
    if (winddirection >= 168.75 && winddirection < 191.25) return TXT_S;
    if (winddirection >= 191.25 && winddirection < 213.75) return TXT_SSW;
    if (winddirection >= 213.75 && winddirection < 236.25) return TXT_SW;
    if (winddirection >= 236.25 && winddirection < 258.75) return TXT_WSW;
    if (winddirection >= 258.75 && winddirection < 281.25) return TXT_W;
    if (winddirection >= 281.25 && winddirection < 303.75) return TXT_WNW;
    if (winddirection >= 303.75 && winddirection < 326.25) return TXT_NW;
    if (winddirection >= 326.25 && winddirection < 348.75) return TXT_NNW;
    return "?";
}
//#########################################################################################
void DisplayTemperatureSection(int x, int y, int w, int h) {
    int cx = x + (w / 2);
    int cy = y + BOX_HEADER + ((h - BOX_HEADER) / 2);
    drawBox(x, y, w, h, cx, TXT_TEMPERATURES);

    u8g2Fonts.setFont(u8g2_font_helvB10_tf);
    drawString(cx, y + h - 20, "Min:" + String(WxConditions[0].Low, 0) + "° | Max:" + String(WxConditions[0].High, 0) + "°", CENTER); // Show forecast high and Low
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    drawString(cx, cy, "Feels like " + String(WxConditions[0].Feelslike, 0) + "°", CENTER); // Show feels like

    drawValue(cx, cy - 34, String(WxConditions[0].Temperature, 1) + "°", Units == "M" ? "C" : "F", u8g2_font_helvB24_tf, u8g2_font_helvB10_tf, CENTER);
}
//#########################################################################################
void DisplayForecastTextSection(int x, int y, int w, int h) {
    int cx = x + (w / 2);

    display.drawRect(x, y, w, h, GxEPD_BLACK); // forecast text outline
    u8g2Fonts.setFont(u8g2_font_helvB14_tf);
    String Wx_Description = WxConditions[0].Main0;
    if (WxConditions[0].Forecast0 != "") Wx_Description += " (" + WxConditions[0].Forecast0;
    if (WxConditions[0].Forecast1 != "") Wx_Description += ", " + WxConditions[0].Forecast1;
    if (WxConditions[0].Forecast2 != "") Wx_Description += ", " + WxConditions[0].Forecast2;
    if (Wx_Description.indexOf("(") > 0) Wx_Description += ")";

    int MsgWidth = 43; // Using proportional fonts, so be aware of making it too wide!
    if (Language == "DE") drawStringMaxWidth(cx, y + 20, MsgWidth, Wx_Description, CENTER); // Leave German text in original format, 28 character screen width at this font size
    else                  drawStringMaxWidth(cx, y + 20, MsgWidth, TitleCase(Wx_Description), CENTER); // 28 character screen width at this font size
}
//#########################################################################################
void DisplayForecastWeather(int x, int y, int index) {
    int fwidth = 73;
    x = x + fwidth * index;
    display.drawRect(x, y, fwidth - 1, 81, GxEPD_BLACK);
    display.drawLine(x, y + 16, x + fwidth - 3, y + 16, GxEPD_BLACK);
    DisplayConditionsSection(x, y - 5, fwidth, 43, WxForecast[index].Icon, SmallIcon);
    drawString(x + fwidth / 2, y + 2, String(ConvertUnixTime(WxForecast[index].Dt + WxConditions[0].Timezone).substring(0, 5)), CENTER);
    drawString(x + fwidth / 2, y + 66, String(WxForecast[index].High, 0) + "°/" + String(WxForecast[index].Low, 0) + "°", CENTER);
}
//#########################################################################################
void DisplayPressureSection(int x, int y, int w, int h, float pressure, String slope) {
    int cx = x + (w / 2);
    int cy = y + BOX_HEADER + ((h - BOX_HEADER) / 2);
    drawBox(x, y, w, h, cx, TXT_PRESSURE);

    String slope_direction = TXT_PRESSURE_STEADY;
    if (slope == "+") slope_direction = TXT_PRESSURE_RISING;
    if (slope == "-") slope_direction = TXT_PRESSURE_FALLING;
    u8g2Fonts.setFont(u8g2_font_helvB10_tf);
    drawString(cx, y + h - 20, slope_direction, CENTER);

    drawValue(cx, cy - 25, Units == "I" ? String(pressure, 2) : String(pressure, 0), Units == "M" ? "hPa" : "in", u8g2_font_helvB24_tf, u8g2_font_helvB10_tf, CENTER);
}
//#########################################################################################
void DisplayPrecipitationSection(int x, int y, int w, int h) {
    int cx = x + (w / 2);
    drawBox(x, y, w, h, cx, TXT_PRECIPITATION_SOON);

    u8g2Fonts.setFont(u8g2_font_helvB12_tf);
    if (WxForecast[1].Rainfall >= 0.005) { // Ignore small amounts
        drawString(cx, y + 45, String(WxForecast[1].Rainfall, 2) + (Units == "M" ? "mm" : "in"), CENTER); // Only display rainfall total today if > 0
        addraindrop(x + 58, y + 40, 7);
    }
    if (WxForecast[1].Snowfall >= 0.005)  // Ignore small amounts
        drawString(cx, y + h - 35, String(WxForecast[1].Snowfall, 2) + (Units == "M" ? "mm" : "in") + " **", CENTER); // Only display snowfall total today if > 0
    if (WxForecast[1].Pop >= 0.005)       // Ignore small amounts
        drawString(cx, y + h - 20, String(WxForecast[1].Pop * 100, 0) + "%", CENTER); // Only display pop if > 0
}
//#########################################################################################
void DisplayAstronomySection(int x, int y) {
    display.drawRect(x, y + 16, 409, 59, GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    drawString(x + 8, y + 17, ConvertUnixTime(WxConditions[0].Sunrise + WxConditions[0].Timezone).substring(0, 5) + " " + TXT_SUNRISE, LEFT);
    drawString(x + 8, y + 28, ConvertUnixTime(WxConditions[0].Sunset + WxConditions[0].Timezone).substring(0, 5) + " " + TXT_SUNSET, LEFT);
    time_t _now = time(NULL);
    struct tm* now_utc = gmtime(&_now);
    const int day_utc = now_utc->tm_mday;
    const int month_utc = now_utc->tm_mon + 1;
    const int year_utc = now_utc->tm_year + 1900;
    drawString(x + 170, y + 50, MoonPhase(day_utc, month_utc, year_utc, Hemisphere), LEFT);
    DrawMoon(x + 320, y - 2, day_utc, month_utc, year_utc, Hemisphere);

    time_t utcOffset = mktime(now_utc) - _now;
    m.calculate(_now + utcOffset + WxConditions[0].Timezone);
    mr.calculate(WxConditions[0].lat, WxConditions[0].lon, _now + utcOffset);
    time_t moonRiseTime = mr.riseTime - utcOffset;
    time_t moonSetTime = mr.setTime - utcOffset;
    char LCDTime[] = "HH:MM";
    int xt = 0;
    sprintf(LCDTime, "%02d:%02d", hour(moonRiseTime), minute(moonRiseTime));
    xt = drawString(x + 170, y + 17, LCDTime, LEFT);
    drawString(xt + 5, y + 17, "Moonrise", LEFT);
    sprintf(LCDTime, "%02d:%02d", hour(moonSetTime), minute(moonSetTime));
    xt = drawString(x + 170, y + 28, LCDTime, LEFT);
    drawString(xt + 5, y + 28, "Moonset", LEFT);
    drawString(x + 8, y + 39, String(m.fraction*100,1)+"% Illuminated", LEFT);
    xt = drawString(x + 8, y + 50, "Zodiac", LEFT);
    drawString(xt + 5, y + 50, m.zodiacName, LEFT);
    drawString(x + 170, y + 39, String(m.distance * 6371)+"km Distance", LEFT);
    drawString(x + 8, y + 61, String(m.age)+" Days Moonage", LEFT);
    drawString(x + 170, y + 61, String(m.longitude,2) + "/" + String(m.latitude, 2) + " Lon/Lat", LEFT);
    
}
//#########################################################################################
double DrawMoon(int x, int y, int dd, int mm, int yy, String hemisphere) {
    const int diameter = 47;
    double Phase = NormalizedMoonPhase(dd, mm, yy);
    hemisphere.toLowerCase();
    if (hemisphere == "south") Phase = 1 - Phase;

    // Draw dark part of moon
    display.fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, GxEPD_BLACK);
    const int number_of_lines = 90;
    for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
        double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
        // Determine the edges of the lighted part of the moon
        double Rpos = 2 * Xpos;
        double Xpos1, Xpos2;
        if (Phase < 0.5) {
            Xpos1 = -Xpos;
            Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
        }
        else {
            Xpos1 = Xpos;
            Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
        }
        // Draw light part of moon
        // Marani: Fixed the calculation to draw the moon phases smoothly without straying pixels
        double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
        double pW1y = ceil((number_of_lines - Ypos) / number_of_lines * diameter + y);
        double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
        double pW2y = floor((Ypos + number_of_lines) / number_of_lines * diameter + y);
        bool draw_fill = false;
        if (Phase < 0.48) {
            pW1x = ceil(pW1x);
            pW2x = floor(pW2x-1);
            draw_fill = true;
        }
        else if (Phase > 0.52 && Phase <= 1.0) {
            pW1x = floor(pW1x-1);
            pW2x = ceil(pW2x);
            draw_fill = true;
        }
        if (draw_fill) {
            display.drawLine(pW1x, pW1y, pW2x, pW1y, GxEPD_WHITE);
            display.drawLine(pW1x, pW2y, pW2x, pW2y, GxEPD_WHITE);
        }
    }
    display.drawCircle(x + diameter - 1, y + diameter, diameter / 2, GxEPD_BLACK);
    return Phase;
}
//#########################################################################################
String MoonPhase(int d, int m, int y, String hemisphere) {
    int c, e;
    double jd;
    int b;
    if (m < 3) {
        y--;
        m += 12;
    }
    ++m;
    c = 365.25 * y;
    e = 30.6 * m;
    jd = c + e + d - 694039.09;     /* jd is total days elapsed */
    jd /= 29.53059;                        /* divide by the moon cycle (29.53 days) */
    b = jd;                              /* int(jd) -> b, take integer part of jd */
    jd -= b;                               /* subtract integer part to leave fractional part of original jd */
    b = jd * 8 + 0.5;                /* scale fraction from 0-8 and round by adding 0.5 */
    b = b & 7;                           /* 0 and 8 are the same phase so modulo 8 for 0 */
    if (hemisphere == "south") b = 7 - b;
    if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
    if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
    if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
    if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
    if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
    if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
    if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
    if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
    return "";
}
//#########################################################################################
String MoonAge(int d, int m, int y, String hemisphere) {
    int c, e;
    double jd;
    int b;
    if (m < 3) {
        y--;
        m += 12;
    }
    ++m;
    c = 365.25 * y;
    e = 30.6 * m;
    jd = c + e + d - 694039.09;     /* jd is total days elapsed */
    jd /= 29.53059;                        /* divide by the moon cycle (29.53 days) */
    b = jd;                              /* int(jd) -> b, take integer part of jd */
    jd -= b;                               /* subtract integer part to leave fractional part of original jd */
    jd = abs(jd - 0.5);                   /* 0 = new - 50 = full - 100 again new  --> 0 = 0% ; 0.5 = 100% */
    b = 100 - jd * 200;
    if (hemisphere == "south") b = 100 - b;
    return String(b) + "% Illumination";
}
//#########################################################################################
void DisplayForecastSection(int x, int y) {
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    for (uint8_t f = 0; f <= 7; f++) {
        DisplayForecastWeather(x, y, f);
    };
    // Pre-load temporary arrays with with data - because C parses by reference
    for (uint8_t r = 0; r < max_readings; r++) {
        if (Units == "I") pressure_readings[r] = WxForecast[r].Pressure * 0.02953;   else pressure_readings[r] = WxForecast[r].Pressure;
        if (Units == "I") rain_readings[r] = WxForecast[r].Rainfall * 0.0393701; else rain_readings[r] = WxForecast[r].Rainfall;
        if (Units == "I") snow_readings[r] = WxForecast[r].Snowfall * 0.0393701; else snow_readings[r] = WxForecast[r].Snowfall;
        temperature_readings[r] = WxForecast[r].Temperature;
        temperature_feel_readings[r] = WxForecast[r].Feelslike;
        humidity_readings[r] = WxForecast[r].Humidity;
    };
    int gwidth = 165, gheight = 122;
    int gx = (SCREEN_WIDTH - gwidth * 4) / 5 + 5;
    int gy = 345;
    int gap = gwidth + gx;
    // (x,y,width,height,MinValue, MaxValue, Title, Data Array, AutoScale, ChartMode)
    DrawGraph(gx + 0 * gap, gy, gwidth, gheight, 900, 1050, Units == "M" ? TXT_PRESSURE_HPA : TXT_PRESSURE_IN, pressure_readings, max_readings, autoscale_on, barchart_off, drawgraph_on);
    float y1min, y1max, y2min, y2max;
    calcMinMaxFromArray(max_readings, temperature_readings, y1min, y1max); // Calc min max of 2 Array to show them both correctly in one graph
    calcMinMaxFromArray(max_readings, temperature_feel_readings, y2min, y2max);
    DrawGraph(gx + 1 * gap, gy, gwidth, gheight, _min(y1min, y2min), _max(y1max, y2max), Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, max_readings, autoscale_off, barchart_off, drawgraph_on);
    DrawGraph(gx + 1 * gap, gy, gwidth, gheight, _min(y1min, y2min), _max(y1max, y2max), Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_feel_readings, max_readings, autoscale_off, barchart_off, drawgraph_off);
    DrawGraph(gx + 2 * gap, gy, gwidth, gheight, 0, 100, TXT_HUMIDITY_PERCENT, humidity_readings, max_readings, autoscale_off, barchart_off, drawgraph_on);
    calcMinMaxFromArray(max_readings, rain_readings, y1min, y1max); // Calc min max of 2 Array to show them both correctly in one graph
    calcMinMaxFromArray(max_readings, snow_readings, y2min, y2max);
    DrawGraph(gx + 3 * gap + 5, gy, gwidth, gheight, _min(y1min, y2min), _max(y1max, y2max), Units == "M" ? TXT_RAINSNOWFALL_MM : TXT_RAINSNOWFALL_IN, rain_readings, max_readings, autoscale_off, barchart_on, drawgraph_on);
    DrawGraph(gx + 3 * gap + 5, gy, gwidth, gheight, _min(y1min, y2min), _max(y1max, y2max), Units == "M" ? TXT_RAINSNOWFALL_MM : TXT_RAINSNOWFALL_IN, snow_readings, max_readings, autoscale_off, barchart_on, drawgraph_off);
}
//#########################################################################################
void DisplayConditionsSection(int x, int y, int w, int h, String IconName, bool IconSize) {
    int cx = x + (w / 2);
    int cy = y + BOX_HEADER + ((h - BOX_HEADER) / 2);

    if (IconSize == LargeIcon) {
        drawBox(x, y, w, h, cx, TXT_CONDITIONS);
        drawValue(cx, y + h - 25, String(WxConditions[0].Humidity, 0) + "%", "RH", u8g2_font_helvB14_tf, u8g2_font_helvB10_tf, CENTER);
        Visibility(x + 25, y + 40, String(WxConditions[0].Visibility) + "M");
        CloudCover(x + w - 60, y + 40, WxConditions[0].Cloudcover);
        if (IconName.endsWith("n")) cy += 20;
    }
    else cy += 20;

    Serial.println("Icon name: " + IconName);
    if (IconName == "01d" || IconName == "01n")       Sunny(cx, cy, IconSize, IconName);
    else if (IconName == "02d" || IconName == "02n")  MostlySunny(cx, cy, IconSize, IconName);
    else if (IconName == "03d" || IconName == "03n")  Cloudy(cx, cy, IconSize, IconName);
    else if (IconName == "04d" || IconName == "04n")  MostlySunny(cx, cy, IconSize, IconName);
    else if (IconName == "09d" || IconName == "09n")  ChanceRain(cx, cy, IconSize, IconName);
    else if (IconName == "10d" || IconName == "10n")  Rain(cx, cy, IconSize, IconName);
    else if (IconName == "11d" || IconName == "11n")  Tstorms(cx, cy, IconSize, IconName);
    else if (IconName == "13d" || IconName == "13n")  Snow(cx, cy, IconSize, IconName);
    else if (IconName == "50d")                       Haze(cx, cy, IconSize, IconName);
    else if (IconName == "50n")                       Fog(cx, cy, IconSize, IconName);
    else                                              Nodata(cx, cy, IconSize, IconName);
}
//#########################################################################################
void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
    float dx = (asize + 28) * cos((aangle - 90) * PI / 180) + x; // calculate X position
    float dy = (asize + 28) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
    float x1 = 0;         float y1 = plength;
    float x2 = pwidth / 2;  float y2 = pwidth / 2;
    float x3 = -pwidth / 2; float y3 = pwidth / 2;
    float angle = aangle * PI / 180;
    float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
    float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
    float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
    float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
    float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
    float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
    display.fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, GxEPD_BLACK);
}
//#########################################################################################
uint8_t StartWiFi() {
    Serial.print("\r\nConnecting to: "); Serial.println(String(ssid));
    IPAddress dns(8, 8, 8, 8); // Google DNS
    WiFi.disconnect();
    WiFi.mode(WIFI_STA); // switch off AP
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    uint8_t connectionStatus;
    bool AttemptConnection = true;
    while (AttemptConnection) {
        connectionStatus = WiFi.status();
        if (millis() > start + 15000) { // Wait 15-secs maximum
            AttemptConnection = false;
        }
        if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) {
            AttemptConnection = false;
        }
        delay(50);
    }
    if (connectionStatus == WL_CONNECTED) {
        wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
        Serial.println("WiFi connected at: " + WiFi.localIP().toString());
    }
    else Serial.println("WiFi connection *** FAILED ***");
    return connectionStatus;
}
//#########################################################################################
void StopWiFi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}
//#########################################################################################
void DisplayStatusSection(int x, int y, int rssi) {
//    display.drawRect(x - 35, y - 32, 145, 61, GxEPD_BLACK);
//    display.drawLine(x - 35, y - 17, x - 35 + 145, y - 17, GxEPD_BLACK);
//    display.drawLine(x - 35 + 146 / 2, y - 18, x - 35 + 146 / 2, y - 32, GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
//    drawString(x + 3, y - 29, TXT_WIFI, CENTER);
//    drawString(x + 75, y - 30, TXT_POWER, CENTER);
    DrawRSSI(SCREEN_WIDTH, 0, rssi);
    DrawBattery(SCREEN_WIDTH - 150, 0);;
}
//#########################################################################################
void DrawRSSI(int x, int y, int rssi) {
    int WIFIsignal = 0;
    int xpos = 1;
    for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
        if (_rssi <= -20)  WIFIsignal = 20; //            <-20dbm displays 5-bars
        if (_rssi <= -40)  WIFIsignal = 16; //  -40dbm to  -21dbm displays 4-bars
        if (_rssi <= -60)  WIFIsignal = 12; //  -60dbm to  -41dbm displays 3-bars
        if (_rssi <= -80)  WIFIsignal = 8;  //  -80dbm to  -61dbm displays 2-bars
        if (_rssi <= -100) WIFIsignal = 4;  // -100dbm to  -81dbm displays 1-bar
        display.fillRect(x - 29 + xpos * 6, y + 17 - WIFIsignal, 5, WIFIsignal, GxEPD_BLACK);
        xpos++;
    }
    display.fillRect(x - 29, y + 16, 5, 1, GxEPD_BLACK);
    drawString(x - 32, y, String(rssi) + "dBm", RIGHT);
}
//#########################################################################################
boolean SetupTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
    setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
    tzset(); // Set the TZ environment variable
    delay(100);
    bool TimeStatus = UpdateLocalTime();
    return TimeStatus;
}
//#########################################################################################
boolean UpdateLocalTime() {
    struct tm timeinfo;
    char   time_output[30], day_output[30], update_time[30];
    while (!getLocalTime(&timeinfo, 10000)) { // Wait for 10-sec for time to synchronise
        Serial.println("Failed to obtain time");
        return false;
    }
    CurrentDay = timeinfo.tm_wday;
    CurrentHour = timeinfo.tm_hour;
    CurrentMin = timeinfo.tm_min;
    CurrentSec = timeinfo.tm_sec;
    //See http://www.cplusplus.com/reference/ctime/strftime/
    //Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
    if (Units == "M") {
        if ((Language == "CZ") || (Language == "DE") || (Language == "PL") || (Language == "NL")) {
            sprintf(day_output, "%s, %02u. %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900); // day_output >> So., 23. Juni 2019 <<
        }
        else
        {
            sprintf(day_output, "%s %02u-%s-%04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
        }
        strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '14:05:49'
        sprintf(time_output, "%s %s", TXT_UPDATED, update_time);
    }
    else
    {
        strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
        strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '02:05:49pm'
        sprintf(time_output, "%s %s", TXT_UPDATED, update_time);
    }
    Date_str = day_output;
    Time_str = time_output;
    return true;
}
//#########################################################################################
void DrawBattery(int x, int y) {
    uint8_t percentage = 100;
    float voltage = analogRead(35) / 4096.0 * 6.96;

    if ((voltage > 3.0f) && (voltage < 4.9f)) { // Show voltage only display if there is a valid reading
        Serial.println("Voltage = " + String(voltage));
        percentage = constrain((voltage - 3.5f) * 100.0f / (4.2f - 3.5f), 0, 100);
        drawString(x - 5, y, String(percentage) + "%", RIGHT);
        drawString(x + 25, y, String(voltage, 2) + "v", LEFT);
    }
    else {
        Serial.println("Voltage = USB");
        percentage = 100;
        drawString(x - 5, y, "USB", RIGHT);
    }
    display.drawRect(x, y + 1, 19, 10, GxEPD_BLACK);
    display.fillRect(x + 19, y + 4, 2, 4, GxEPD_BLACK);
    display.fillRect(x + 2, y + 3, 15 * percentage / 100.0, 6, GxEPD_BLACK);
}
//#########################################################################################
// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
    linesize = _max(linesize, 2);
    //Draw cloud outer
    display.fillCircle(x - scale * 3, y, scale, GxEPD_BLACK);                // Left most circle
    display.fillCircle(x + scale * 3, y, scale, GxEPD_BLACK);                // Right most circle
    display.fillCircle(x - scale, y - scale, scale * 1.4, GxEPD_BLACK);    // left middle upper circle
    display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, GxEPD_BLACK); // Right middle upper circle
    display.fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, GxEPD_BLACK); // Upper and lower lines
    //Clear cloud inner
    display.fillCircle(x - scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear left most circle
    display.fillCircle(x + scale * 3, y, scale - linesize, GxEPD_WHITE);            // Clear right most circle
    display.fillCircle(x - scale, y - scale, scale * 1.4 - linesize, GxEPD_WHITE);  // left middle upper circle
    display.fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, GxEPD_WHITE); // Right middle upper circle
    display.fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, GxEPD_WHITE); // Upper and lower lines
}
//#########################################################################################
void addraindrop(int x, int y, int scale) {
    display.fillCircle(x, y, scale / 2, GxEPD_BLACK);
    display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y, GxEPD_BLACK);
    x = x + scale * 1.6; y = y + scale / 3;
    display.fillCircle(x, y, scale / 2, GxEPD_BLACK);
    display.fillTriangle(x - scale / 2, y, x, y - scale * 1.2, x + scale / 2, y, GxEPD_BLACK);
}
//#########################################################################################
void addrain(int x, int y, int scale, bool IconSize) {
    if (IconSize == SmallIcon) scale *= 1.34;
    for (int d = 0; d < 4; d++) {
        addraindrop(x + scale * (7.8 - d * 1.95) - scale * 5.2, y + scale * 2.1 - scale / 6, scale / 1.6);
    }
}
//#########################################################################################
void addsnow(int x, int y, int scale, bool IconSize) {
    int dxo, dyo, dxi, dyi;
    for (int flakes = 0; flakes < 5; flakes++) {
        for (int i = 0; i < 360; i = i + 45) {
            dxo = 0.5 * scale * cos((i - 90) * 3.14 / 180); dxi = dxo * 0.1;
            dyo = 0.5 * scale * sin((i - 90) * 3.14 / 180); dyi = dyo * 0.1;
            display.drawLine(dxo + x + flakes * 1.5 * scale - scale * 3, dyo + y + scale * 2, dxi + x + 0 + flakes * 1.5 * scale - scale * 3, dyi + y + scale * 2, GxEPD_BLACK);
        }
    }
}
//#########################################################################################
void addtstorm(int x, int y, int scale) {
    y = y + scale / 2;
    for (int i = 0; i < 5; i++) {
        display.drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, GxEPD_BLACK);
        if (scale != Small) {
            display.drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, GxEPD_BLACK);
            display.drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, GxEPD_BLACK);
        }
        display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, GxEPD_BLACK);
        if (scale != Small) {
            display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, GxEPD_BLACK);
            display.drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, GxEPD_BLACK);
        }
        display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, GxEPD_BLACK);
        if (scale != Small) {
            display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, GxEPD_BLACK);
            display.drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, GxEPD_BLACK);
        }
    }
}
//#########################################################################################
void addsun(int x, int y, int scale, bool IconSize) {
    int linesize = 3;
    if (IconSize == SmallIcon) linesize = 2;
    display.fillRect(x - scale * 2, y, scale * 4, linesize, GxEPD_BLACK);
    display.fillRect(x, y - scale * 2, linesize, scale * 4, GxEPD_BLACK);
    display.drawLine(x - scale * 1.3, y - scale * 1.3, x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
    display.drawLine(1 + x - scale * 1.3, y - scale * 1.3, 1 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
    display.drawLine(x - scale * 1.3, y + scale * 1.3, x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
    display.drawLine(1 + x - scale * 1.3, y + scale * 1.3, 1 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
    if (IconSize == LargeIcon) {
        display.drawLine(1 + x - scale * 1.3, y - scale * 1.3, 1 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
        display.drawLine(2 + x - scale * 1.3, y - scale * 1.3, 2 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
        display.drawLine(3 + x - scale * 1.3, y - scale * 1.3, 3 + x + scale * 1.3, y + scale * 1.3, GxEPD_BLACK);
        display.drawLine(1 + x - scale * 1.3, y + scale * 1.3, 1 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
        display.drawLine(2 + x - scale * 1.3, y + scale * 1.3, 2 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
        display.drawLine(3 + x - scale * 1.3, y + scale * 1.3, 3 + x + scale * 1.3, y - scale * 1.3, GxEPD_BLACK);
    }
    display.fillCircle(x, y, scale * 1.3, GxEPD_WHITE);
    display.fillCircle(x, y, scale, GxEPD_BLACK);
    display.fillCircle(x, y, scale - linesize, GxEPD_WHITE);
}
//#########################################################################################
void addfog(int x, int y, int scale, int linesize, bool IconSize) {
    if (IconSize == SmallIcon) {
        y -= 10;
        linesize = 1;
    }
    for (int i = 0; i < 6; i++) {
        display.fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, GxEPD_BLACK);
        display.fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, GxEPD_BLACK);
        display.fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, GxEPD_BLACK);
    }
}
//#########################################################################################
void Sunny(int x, int y, bool IconSize, String IconName) {
    int scale = Small;
    if (IconSize == LargeIcon) scale = Large;
    else y = y - 3; // Shift up small sun icon
    if (IconName.endsWith("n")) addmoon(x, y + 3, scale, IconSize);
    scale = scale * 1.6;
    addsun(x, y, scale, IconSize);
}
//#########################################################################################
void MostlySunny(int x, int y, bool IconSize, String IconName) {
    int scale = Small, linesize = 3, offset = 5;
    if (IconSize == LargeIcon) {
        scale = Large;
        offset = 20;
    }
    if (scale == Small) linesize = 1;
    if (IconName.endsWith("n")) addmoon(x, y + offset, scale, IconSize);
    addcloud(x, y + offset, scale, linesize);
    addsun(x - scale * 1.8, y - scale * 1.8 + offset, scale, IconSize);
}
//#########################################################################################
void MostlyCloudy(int x, int y, bool IconSize, String IconName) {
    int scale = Small, linesize = 3;
    if (IconSize == LargeIcon) {
        scale = Large;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addcloud(x, y, scale, linesize);
    addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
    addcloud(x, y, scale, linesize);
}
//#########################################################################################
void Cloudy(int x, int y, bool IconSize, String IconName) {
    int scale = Large, linesize = 3;
    if (IconSize == SmallIcon) {
        scale = Small;
        if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
        linesize = 1;
        addcloud(x, y, scale, linesize);
    }
    else {
        y += 10;
        if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
        addcloud(x + 30, y - 45, 5, linesize); // Cloud top right
        addcloud(x - 20, y - 30, 7, linesize); // Cloud top left
        addcloud(x, y, scale, linesize);       // Main cloud
    }
}
//#########################################################################################
void Rain(int x, int y, bool IconSize, String IconName) {
    int scale = Large, linesize = 3;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addcloud(x, y, scale, linesize);
    addrain(x, y, scale, IconSize);
}
//#########################################################################################
void ExpectRain(int x, int y, bool IconSize, String IconName) {
    int scale = Large, linesize = 3;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
    addcloud(x, y, scale, linesize);
    addrain(x, y, scale, IconSize);
}
//#########################################################################################
void ChanceRain(int x, int y, bool IconSize, String IconName) {
    int scale = Large, linesize = 3;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
    addcloud(x, y, scale, linesize);
    addrain(x, y, scale, IconSize);
}
//#########################################################################################
void Tstorms(int x, int y, bool IconSize, String IconName) {
    int scale = Large, linesize = 3;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addcloud(x, y, scale, linesize);
    addtstorm(x, y, scale);
}
//#########################################################################################
void Snow(int x, int y, bool IconSize, String IconName) {
    int scale = Large, linesize = 3;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addcloud(x, y, scale, linesize);
    addsnow(x, y, scale, IconSize);
}
//#########################################################################################
void Fog(int x, int y, bool IconSize, String IconName) {
    int linesize = 3, scale = Large;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addcloud(x, y - 5, scale, linesize);
    addfog(x, y - 5, scale, linesize, IconSize);
}
//#########################################################################################
void Haze(int x, int y, bool IconSize, String IconName) {
    int linesize = 3, scale = Large;
    if (IconSize == SmallIcon) {
        scale = Small;
        linesize = 1;
    }
    if (IconName.endsWith("n")) addmoon(x, y, scale, IconSize);
    addsun(x, y - 5, scale * 1.4, IconSize);
    addfog(x, y - 5, scale * 1.4, linesize, IconSize);
}
//#########################################################################################
void CloudCover(int x, int y, int CCover) {
    addcloud(x - 9, y - 3, Small * 0.5, 2); // Cloud top left
    addcloud(x + 3, y - 3, Small * 0.5, 2); // Cloud top right
    addcloud(x, y, Small * 0.5, 2); // Main cloud
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    drawString(x + 18, y - 8, String(CCover) + "%", LEFT);
}
//#########################################################################################
void Visibility(int x, int y, String Visi) {
    y = y - 3; //
    float start_angle = 0.52, end_angle = 2.61;
    int r = 10;
    for (float i = start_angle; i < end_angle; i = i + 0.05) {
        display.drawPixel(x + r * cos(i), y - r / 2 + r * sin(i), GxEPD_BLACK);
        display.drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i), GxEPD_BLACK);
    }
    start_angle = 3.61; end_angle = 5.78;
    for (float i = start_angle; i < end_angle; i = i + 0.05) {
        display.drawPixel(x + r * cos(i), y + r / 2 + r * sin(i), GxEPD_BLACK);
        display.drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i), GxEPD_BLACK);
    }
    display.fillCircle(x, y, r / 4, GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
    drawString(x + 12, y - 5, Visi, LEFT);
}
//#########################################################################################
void addmoon(int x, int y, int scale, bool IconSize) {
    if (IconSize == LargeIcon) {
        display.fillCircle(x - 62, y - 68, scale, GxEPD_BLACK);
        display.fillCircle(x - 43, y - 68, scale * 1.6, GxEPD_WHITE);
    }
    else
    {
        display.fillCircle(x - 25, y - 15, scale, GxEPD_BLACK);
        display.fillCircle(x - 18, y - 15, scale * 1.6, GxEPD_WHITE);
    }
}
//#########################################################################################
void Nodata(int x, int y, bool IconSize, String IconName) {
    if (IconSize == LargeIcon) u8g2Fonts.setFont(u8g2_font_helvB24_tf); else u8g2Fonts.setFont(u8g2_font_helvB10_tf);
    drawString(x - 3, y - 10, "?", CENTER);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);
}
//#########################################################################################
/* (C) D L BIRD
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
    The variable 'max_readings' determines the maximum number of data elements for each array. Call it with the following parametric data:
    x_pos-the x axis top-left position of the graph
    y_pos-the y-axis top-left position of the graph, e.g. 100, 200 would draw the graph 100 pixels along and 200 pixels down from the top-left of the screen
    width-the width of the graph in pixels
    height-height of the graph in pixels
    Y1_Max-sets the scale of plotted data, for example 5000 would scale all data to a Y-axis of 5000 maximum
    data_array1 is parsed by value, externally they can be called anything else, e.g. within the routine it is called data_array1, but externally could be temperature_readings
    auto_scale-a logical value (TRUE or FALSE) that switches the Y-axis autoscale On or Off
    barchart_on-a logical value (TRUE or FALSE) that switches the drawing mode between barhcart and line graph
    barchart_colour-a sets the title and graph plotting colour
    If called with Y!_Max value of 500 and the data never goes above 500, then autoscale will retain a 0-500 Y scale, if on, the scale increases/decreases to match the data.
    auto_scale_margin, e.g. if set to 1000 then autoscale increments the scale by 1000 steps.
    Marani: Added draw_graph: TRUE = Draw complete graph with values, FALSE = Just draw values (to add multiple values in one graph) thin
    Marani: Fixed error in MinMax Calculations
    Marani: Fixed error in DrawGraph values (Array 0-based index error)
*/
void calcMinMaxFromArray(uint8_t readings, float DataArray[], float& Y1Min, float& Y1Max) {
    float maxYscale = -10000;
    float minYscale = 10000;
    for (uint8_t i = 0; i < readings; i++) {
        if (DataArray[i] > maxYscale) maxYscale = DataArray[i];
        if (DataArray[i] < minYscale) minYscale = DataArray[i];
    }
    Y1Max = ceil(maxYscale + 0.5);
    if (minYscale > 0.5) Y1Min = floor(minYscale - 0.5); else Y1Min = floor(minYscale);
}

void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], uint8_t readings, boolean auto_scale, boolean barchart_mode, boolean draw_graph) {
#define y_minor_axis 5      // 5 y-axis division markers
    int last_x, last_y;
    float x2, y2;
    if (auto_scale == true) calcMinMaxFromArray(readings, DataArray, Y1Min, Y1Max);
    // Draw the graph
    last_x = x_pos + 1;
    last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
    if (draw_graph) {
        display.drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, GxEPD_BLACK);
        drawString(x_pos + gwidth / 2, y_pos - 16, title, CENTER);
    }
    // Draw the data
    for (uint8_t gx = 0; gx < readings; gx++) {
        x2 = x_pos + (gx+1) * gwidth / (readings) - 1; // max_readings is the global variable that sets the maximum data that can be plotted
        y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
        if (barchart_mode) {
            if (draw_graph) display.fillRect(x2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, GxEPD_BLACK);
            else            display.drawRect(x2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, GxEPD_BLACK);
        }
        else {
            display.drawLine(last_x, last_y, x2, y2, GxEPD_BLACK);
            if (draw_graph) {
                if (last_x == x2)
                    display.drawLine(last_x, last_y + 1, x2, y2 + 1, GxEPD_BLACK);
                else
                    display.drawLine(last_x + 1, last_y, x2 + 1, y2, GxEPD_BLACK);
            }
        }
        last_x = x2;
        last_y = y2;
    }
    //Draw the Y-axis scale
#define number_of_dashes 20
    if (draw_graph) {
        float daywidth = gwidth / 3;
        for (int i = 0; i <= 3; i++) {
            drawStringBoundingBox(20 + x_pos + daywidth * i - ((float)CurrentHour / 24.0f * daywidth), y_pos + gheight + 3, weekday_D[(CurrentDay + i) % 7], LEFT, x_pos, x_pos + gwidth + 3);


        }
        for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
            for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
                if (spacing < y_minor_axis) display.drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), GxEPD_BLACK);
            }
            if ((((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) < 5) && ((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) > 0)) || title == TXT_PRESSURE_IN) {
                drawString(x_pos - 1, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
            }
            else
            {
                if (Y1Min < 1 && Y1Max < 10)
                    drawString(x_pos - 1, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
                else
                    drawString(x_pos - 2, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
            }
        }
        // drawString(x_pos + gwidth / 2, y_pos + gheight + 14, TXT_DAYS, CENTER);
    }
}

//#########################################################################################
int drawString(int x, int y, String text, alignment align) {
    uint16_t w, h;
    display.setTextWrap(false);

    w = u8g2Fonts.getUTF8Width(text.c_str());
    h = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();

    if (align == RIGHT)  x = x - w;
    if (align == CENTER) x = x - w / 2;
    u8g2Fonts.setCursor(x, y + h);
    u8g2Fonts.print(text);
//    display.drawRect(x, y - u8g2Fonts.getFontDescent(), w, h, GxEPD_BLACK);
    return x + w;
}
//#########################################################################################
void drawStringBoundingBox(int x, int y, String text, alignment align, int x1b, int x2b) {
    int xend = drawString(x, y, text, align);
    if (x < x1b) {
        display.fillRect(x, y, x1b - x, u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent(), GxEPD_WHITE);
    }
    if (xend > x2b) {
        display.fillRect(x2b, y, xend - x2b, u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent(), GxEPD_WHITE);
    }
}
//#########################################################################################
void drawValue(int x, int y, String value_txt, String unit_txt, const uint8_t* value_font, const uint8_t* unit_font, alignment align) {
    uint16_t w, h, w1, w2, h1, h2;
    const int spacing = 5;

    display.setTextWrap(false);

    // Calc boundaries for Value and Unit in different fontsizes
    u8g2Fonts.setFont(unit_font);
    w2 = u8g2Fonts.getUTF8Width(unit_txt.c_str());
    h2 = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();

    u8g2Fonts.setFont(value_font);
    w1 = u8g2Fonts.getUTF8Width(value_txt.c_str());
    h1 = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();

    w = w1 + w2 + spacing;
    h = _max(h1, h2);

    if (align == RIGHT)  x = x - w;
    if (align == CENTER) x = x - w / 2;

    u8g2Fonts.setCursor(x, y + h);
    u8g2Fonts.print(value_txt);
    u8g2Fonts.setFont(unit_font);
    u8g2Fonts.setCursor(x + w1 + spacing, y + h);
    u8g2Fonts.print(unit_txt);

//    display.drawRect(x, y, w, h, GxEPD_BLACK);
}
//#########################################################################################
void myPrintLn(int x, int y, String text, alignment align) {
    uint16_t w;
    w = u8g2Fonts.getUTF8Width(text.c_str());
    if (align == RIGHT)  x = x - w;
    if (align == CENTER) x = x - w / 2;
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.println(text);
}
void drawStringMaxWidth(int x, int y, unsigned int text_width, String text, alignment align) {
    int tl = text.length();
    if (tl > text_width * 3) {
        u8g2Fonts.setFont(u8g2_font_helvB10_tf);
        text_width = 42;
        y -= 3;
    }
    if (tl < text_width) {
        y += 18;
    }
    u8g2Fonts.setCursor(x, y);
    int tw = 0;
    while (tl > 0)
    {
        String line = text.substring(text_width * tw, text_width * tw + text_width);
        tl -= line.length();
        myPrintLn(x, u8g2Fonts.getCursorY(), line, align);
        tw += 1;
    }
}
//#########################################################################################
void InitialiseDisplay() {
    display.init(115200, true, 2, false); // init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration, bool pulldown_rst_mode)
    // display.init(); for older Waveshare HAT's
    SPI.end();
    SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);
    u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
    u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
    u8g2Fonts.setFontDirection(0);             // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
    u8g2Fonts.setFont(u8g2_font_helvB10_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    display.fillScreen(GxEPD_WHITE);
    display.setFullWindow();
}
//#########################################################################################
/*String Translate_EN_DE(String text) {
  if (text == "clear")            return "klar";
  if (text == "sunny")            return "sonnig";
  if (text == "mist")             return "Nebel";
  if (text == "fog")              return "Nebel";
  if (text == "rain")             return "Regen";
  if (text == "shower")           return "Regenschauer";
  if (text == "cloudy")           return "wolkig";
  if (text == "clouds")           return "Wolken";
  if (text == "drizzle")          return "Nieselregen";
  if (text == "snow")             return "Schnee";
  if (text == "thunderstorm")     return "Gewitter";
  if (text == "light")            return "leichter";
  if (text == "heavy")            return "schwer";
  if (text == "mostly cloudy")    return "größtenteils bewölkt";
  if (text == "overcast clouds")  return "überwiegend bewölkt";
  if (text == "scattered clouds") return "aufgelockerte Bewölkung";
  if (text == "few clouds")       return "ein paar Wolken";
  if (text == "clear sky")        return "klarer Himmel";
  if (text == "broken clouds")    return "aufgerissene Bewölkung";
  if (text == "light rain")       return "leichter Regen";
  return text;
  }
*/
/*
  Version 16.0 reformatted to use u8g2 fonts
   1.  Added ß to translations, eventually that conversion can move to the lang_xx.h file
   2.  Spaced temperature, pressure and precipitation equally, suggest in DE use 'niederschlag' for 'Rain/Snow'
   3.  No-longer displays Rain or Snow unless there has been any.
   4.  The nn-mm 'Rain suffix' has been replaced with two rain drops
   5.  Similarly for 'Snow' two snow flakes, no words and '=Rain' and '"=Snow' for none have gone.
   6.  Improved the Cloud Cover icon and only shows if reported, 0% cloud (clear sky) is no-report and no icon.
   7.  Added a Visibility icon and reported distance in Metres. Only shows if reported.
   8.  Fixed the occasional sleep time error resulting in constant restarts, occurred when updates took longer than expected.
   9.  Improved the smaller sun icon.
   10. Added more space for the Sunrise/Sunset and moon phases when translated.

  Version 16.1 Correct timing errors after sleep - persistent problem that is not deterministic
   1.  Removed Weather (Main) category e.g. previously 'Clear (Clear sky)', now only shows area category of 'Clear sky' and then ', caterory1' and ', category2'
   2.  Improved accented character displays

  Version 16.2 Correct comestic icon issues
   1.  At night the addition of a moon icon overwrote the Visibility report, so order of drawing was changed to prevent this.
   2.  RainDrop icon was too close to the reported value of rain, moved right. Same for Snow Icon.
   3.  Improved large sun icon sun rays and improved all icon drawing logic, rain drops now use common shape.
   5.  Moved MostlyCloudy Icon down to align with the rest, same for MostlySunny.
   6.  Improved graph axis alignment.

  Version 16.3 Correct comestic icon issues
   1.  Reverted some aspects of UpdateLocalTime() as locialisation changes were unecessary and can be achieved through lang_aa.h files
   2.  Correct configuration mistakes with moon calculations.

  Version 16.4 Corrected time server addresses and adjusted maximum time-out delay
   1.  Moved time-server address to the credentials file
   2.  Increased wait time for a valid time setup to 10-secs
   3.  Added a lowercase conversion of hemisphere to allow for 'North' or 'NORTH' or 'nOrth' entries for hemisphere
   4.  Adjusted graph y-axis alignment, redcued number of x dashes

  Version 16.5 Clarified connections for Waveshare ESP32 driver board
   1.  Added SPI.end(); and SPI.begin(CLK, MISO, MOSI, CS); to enable explicit definition of pins to be used.

  Version 16.6 changed GxEPD2 initialisation from 115200 to 0
   1.  Display.init(115200); becomes display.init(0); to stop blank screen following update to GxEPD2

  Version 16.7 changed u8g2 fonts selection
   1.  Omitted 'FONT(' and added _tf to font names either Regular (R) or Bold (B)

  Version 16.8
   1. Added extra 20-secs of sleep to allow for slow ESP32 RTC timers

  Version 16.9
   1. Added probability of precipitation display e.g. 17%

  Version 16.10
   1. Updated display inittialisation for 7.5" T7 display type, which iss now the standard 7.5" display type.

  Version 16.11
   1. Adjusted graph drawing for negative numbers
   2. Correct offset error for precipitation

*/
