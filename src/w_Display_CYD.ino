//D:\Projekte\arduino\libraries\TFT_eSPI\User_Setup_USB2.h für Clone
//D:\Projekte\arduino\libraries\TFT_eSPI\User_Setup_USB1.h für Original

#include "Arduino.h"
#include <SD.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <TimeLib.h>
#include <sunset.h>
#include <Wire.h>
#include "seven_regular11pt7b.h"
#include "seven_regular31pt7b.h"
//#include <NimBLEDevice.h>
#include <HijelHID_BLEKeyboard.h>

//Touch Pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

//SD Karte Pin
#define SD_CS 5

//Backlight Pin
#define backlightPin 21
int brightness = 128; // Helligkeit (0-255)

// RGB Umwandlung
#define RGB565(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

SunSet sun;

//System Variablen
String ssid;
String password;
String tzinfo;
String tformat;
String ntpserver;
String latitude;
String longitude;
String WeekDays[7];
String MonthName[12];
String Translate[5];
String birthday;
String Year[3];
String holiday[3];
String schoolhol[3];
String Dummy;

int yy_mem = 0;
int mm_mem = 0;

int event_tm_hour = -1;
int event_tm_min = -1;
int event_tm_sec = -1;

int function = 0; //0:CAL 1:CLOCK 2:KEYBOARD

// WIEDER HINZUGEFÜGT: Instanz für BLE Keyboard
HijelHID_BLEKeyboard bleKeyboard("CYD MacroPad", "Espressif", 100);

// Array zum Speichern der 8 Makro-Strings von der SD-Karte
String macroStrings[8] = {"", "", "", "", "", "", "", ""};
// Array für die Beschriftungen der Buttons
String macroNames[8] = {"M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8"};

String keyboardLayout = "DE"; // Standardmäßig auf Deutsch eingestellt


// Dimensionen für das Makro-Grid (320x240 Pixel)
const int btnWidth = 93;
const int btnHeight = 66;
const int spacing = 10;

bool wifi_start_STA() //Start WiFi Mode STA
{
  int sync_count = 0;
  WiFi.mode(WIFI_STA);
  if (WiFi.status() != WL_CONNECTED)  
  {
    Serial.println("WiFi Start");
    tft.println("WiFi Start");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      tft.print(".");
      sync_count = ++sync_count;
      if (sync_count == 40)
      {
        Serial.println();
        tft.println();
        return 0;
        break;     
      }
      if (sync_count == 6)
      {
        WiFi.begin(ssid, password); //second try
      }      
      if (sync_count == 20)
      {
        WiFi.begin(ssid, password); //second try
      }      
    }    
  }
  Serial.println();
  tft.println();
  Serial.print(F("IP address STA: "));
  tft.print(F("IP address STA: "));
  Serial.println(WiFi.localIP());
  tft.println(WiFi.localIP());
  Serial.print(F("SSID: "));
  tft.print(F("SSID: "));
  Serial.println(WiFi.SSID());
  tft.println(WiFi.SSID());
  Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
  tft.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
  Serial.print(F("PW: "));
  tft.print(F("PW: "));
  Serial.println(WiFi.psk());
  tft.println(WiFi.psk());
  return 1;
}

bool timesync()
{
  bool exit_status = 1;
  Serial.println("Get NTP Time");
  tft.println("Get NTP Time");
  if (WiFi.status() == WL_CONNECTED)
  {
    struct tm local;
    configTzTime(tzinfo.c_str(), ntpserver.c_str()); // ESP32 Systemzeit mit NTP Synchronisieren
    if (!getLocalTime(&local, 10000)) // Versuche 10 s zu Synchronisieren
    {
      Serial.println("Timeserver cannot be reached !!!");
      tft.println("Timeserver cannot be reached !!!");
      exit_status = 0;
    }
    else
    {
      Serial.print("Timeserver: ");
      tft.print("Timeserver: ");
      Serial.println(&local, "Datum: %d.%m.%y  Zeit: %H:%M:%S Test: %a,%B");
      tft.println(&local, "Datum: %d.%m.%y  Zeit: %H:%M:%S Test: %a,%B");
      Serial.flush();
    }
  }
  else
  {
    Serial.println("WiFi not connected !!!");
    tft.println("WiFi not connected !!!");
    exit_status = 0;    
  }  
  return exit_status;  
}

/***** Den Wochentag nach ISO 8601 (1 = Mo, 2 = Di, 3 = Mi, 4 = Do, 5 = Fr, 6 = Sa, 7 = So) berechnen *****/
uint8_t GetWeekday(uint16_t y, uint8_t m, uint8_t d) 
{
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  uint8_t wd = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
  return (wd == 0 ? 7 : wd);
}

/***** Testen, ob das Jahr ein Schaltjahr ist *****/
bool IsLeapYear(uint16_t y) 
{
  return  !(y % 4) && ((y % 100) || !(y % 400)); // Schaltjahrberechnung (true = Schaltjahr, false = kein Schaltjahr)
}

/***** Anzahl der Tage in dem Monat *****/
uint8_t GetDaysOfMonth(uint16_t y, uint8_t m) 
{
  static const uint8_t mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m == 2 && IsLeapYear(y)) {
    return 29;
  } else {
    return mdays[m - 1];
  }
}

/***** die Anzahl der Tage (Tag des Jahres) berechnen *****/
uint16_t GetDayOfYear(uint16_t y, uint8_t m, uint8_t d) 
{
  static const uint16_t mdays[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  return d + mdays[m - 1] + (m >= 2 && IsLeapYear(y));
}

/***** Die Wochennummer nach ISO 8601 berechnen *****/
uint8_t GetWeekNumber(uint16_t y, uint8_t m, uint8_t d) 
{
  bool LeapYear;
  uint16_t doy = GetDayOfYear(y, m, d);  // Anzahl der Tage im Jahr ermitteln
  uint8_t wd = GetWeekday(y, m, d);      // Wochentag ermitteln
  uint8_t wnr = (doy - wd + 10) / 7;     // die Wochennummer berechnen (angepasst)

  if (wnr == 0) {                        // wenn die Wochennummer Null ergibt, dann liegt der Tag am Anfang des Jahres (1. Sonderfall)
    wd = GetWeekday(y - 1, 12, 31);      // den letzten Wochentag aus dem Vorjahr ermitteln
    LeapYear = IsLeapYear(y - 1);        // ermitteln, ob es sich beim Vorjahr um ein Schaltjahr handelt
    if (wd < 4) {                        // wenn der 31.12. vor dem Donnerstag liegt, dann...
      wnr = 1;                           // ist das die erste Woche des Jahres
    } else {                             // anderenfalls muss ermittelt werden, ob es eine 53. Woche gibt (3. Sonderfall)
      wnr = ((wd == 4) || (LeapYear && wd == 5)) ? 53 : 52;
    }
  } else if (wnr == 53) {                // wenn die Wochennummer 53 ergibt, dann liegt der Tag am Ende des Jahres (2. Sonderfall)
    wd = GetWeekday(y, 12, 31);          // den letzten Wochentag aus diesem Jahr ermitteln
    LeapYear = IsLeapYear(y);            // ermitteln, ob es sich bei diesem Jahr um ein Schaltjahr handelt
    if (wd < 4) {                        // wenn der 31.12. vor dem Donnerstag liegt, dann...
      wnr = 1;                           // ist das die erste Woche des nächsten Jahres
    } else {                             // anderenfalls muss ermittelt werden, ob es eine 53. Woche gibt (3. Sonderfall)
      wnr = ((wd == 4) || (LeapYear && wd == 5)) ? 53 : 52;
    }
  }
  return wnr;
}

uint16_t createColor(uint8_t r, uint8_t g, uint8_t b) 
{
  return RGB565(r >> 3, g >> 2, b >> 3);
}

// CAL zeichnen
void draw_cal(uint16_t y, uint8_t m, uint8_t d)
{
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int h_pos = 0;

  int year_index = 0;
  for (int i = 0; i < sizeof(Year) / sizeof(Year[0]); i++) 
  {
    if (Year[i].toInt() == y) 
    {
        year_index = i;
        break;
    }
  }

  tft.fillRect(0 , 41, 320, 18, createColor(160, 160, 160));
  for (int i = 0; i <= 5; i++) 
  {
    tft.drawString(WeekDays[i], i * 45 + 16, 42, 2);
  }
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString(WeekDays[6], 6 * 45 + 16, 42, 2);

  int j = GetWeekday(y, m, 1) - 1;
  for (int i = 0; i < GetDaysOfMonth(y, m) ; i++)
  {
    int k = (j + i) % 7;
    int l = (j + i) / 7;
    bool feiertag = false;
    int color = TFT_BLACK;
    int colorb = TFT_WHITE;
    String SearchStr  = "," + String(m) + "." + String(i + 1) + "-";
    String SearchStrSchool  = "," + String(m) + "." + String(i + 1) + ",";
    String BirthdayStr = "," + String(birthday) + ",";
    String HolidayStr = "," + String(holiday[year_index]) + ",";
    String SchoolholStr =  "," + String(schoolhol[year_index]) + ",";

    if (k == 6) //dieser Tag ist ein Sonntag
    {
      color = TFT_RED;
    }
    else
    {
      color = TFT_BLACK;
    }

    if (strstr(HolidayStr.c_str(), SearchStr.c_str()) != NULL) //dieser Tag ist ein Feiertag
    {
      feiertag = true;
      color = TFT_RED;
      tft.fillRect(k * 45 + 13, l * 28 + 67, 3, 3, TFT_RED);
      if ((h_pos <= 3) && ((i + 1) >= d))
      {
        char* str1 = strstr(HolidayStr.c_str(), SearchStr.c_str());
        char* str2 = strstr(str1, "-");
        String str3 = String(str2);
        int pos = str3.indexOf(",");
        String str4 = String(i + 1) + "." + String(m) + ". " + str3.substring(1, pos);        
        
        tft.fillRect(170 ,  h_pos * 10 + 3, 3, 3, TFT_RED);        
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString(str4.c_str(), 176, h_pos * 10 + 1, 1);
        ++h_pos;
      }
    }

    if (strstr(BirthdayStr.c_str(), SearchStr.c_str()) != NULL) //dieser Tag ist ein Geburtstag
    {
      if (feiertag == true)
      {
        tft.fillRect(k * 45 + 19, l * 28 + 67, 3, 3, TFT_BLACK);
      }
      else
      {
        tft.fillRect(k * 45 + 13, l * 28 + 67, 3, 3, TFT_BLACK);
      }
      if ((h_pos <= 3) && ((i + 1) >= d))
      {
        char* str1 = strstr(BirthdayStr.c_str(), SearchStr.c_str());
        char* str2 = strstr(str1, "-");
        String str3 = String(str2);
        int pos = str3.indexOf(",");
        String str4 = String(i + 1) + "." + String(m) + ". " + str3.substring(1, pos);        
        
        tft.fillRect(170 ,  h_pos * 10 + 3, 3, 3, TFT_BLACK);        
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString(str4.c_str(), 176, h_pos * 10 + 1, 1);
        ++h_pos;
      }
    }

    if (strstr(SchoolholStr.c_str(), SearchStrSchool.c_str()) != NULL) //dieser Tag ist ein School Ferientag
    {
      colorb = createColor(255,255,150);
    }
    else
    {
      colorb = TFT_WHITE;
    }
    
    tft.setTextColor(color, colorb);

    if (i <= 8) //etwas weiter rechts schreiben wenn Tag 1-9
    {
      tft.drawNumber(i + 1, k * 45 + 20, l * 28 + 70, 2);
    }
    else
    {
      tft.drawNumber(i + 1, k * 45 + 16, l * 28 + 70, 2);  
    }

    if ((i + 1) == d) //aktuellen Tag umranden
    {
      tft.drawRect(k * 45 + 12, l * 28 + 66, 24, 24, color);
    }    
  }

  if (d != 0)
  {
    tft.fillRect(0 , 0, 166, 40, TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
  }
  else
  {
    tft.fillRect(0 , 0, 166, 40, createColor(50, 50, 50));
    tft.setTextColor(TFT_WHITE, createColor(50, 50, 50));
  }
  tft.drawString(MonthName[m - 1].c_str(), 16, 0, 4);
  tft.drawString(String(y), 16, 24, 2);

  tft.drawLine(0, 40, 319, 40, TFT_BLACK);
  tft.drawLine(0, 59, 319, 59, TFT_BLACK);
  tft.drawLine(225, 40, 225, 209, TFT_BLACK);
  tft.drawLine(95, 209, 319, 209, TFT_BLACK);
  tft.drawLine(95, 209, 95, 239, TFT_BLACK);

  tft.setTextColor(TFT_DARKGREEN, TFT_WHITE);
  tft.drawString(Translate[1], 100, 212, 1);
  tft.drawString(Translate[2], 145, 212, 1);
  tft.setTextColor(TFT_GREEN, createColor(90, 90, 90));
  if (d != 0)
  {
    tft.fillRect(99, 222, 40, 26, createColor(90, 90, 90));
    tft.drawString(String(GetWeekNumber(y, m, d)), 102, 222, 2);
    tft.fillRect(144, 222, 40, 26, createColor(90, 90, 90));
    tft.drawString(String(GetDayOfYear(y, m, d)), 147, 222, 2);
  }
  tft.fillRect(189, 210, 131, 30, createColor(90, 90, 90));
}

// Funktion: Zeichnet die 3. Seite (Makro-Tastatur)
void drawMacroPage() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setFreeFont(NULL); 
  tft.setTextFont(1); 
  tft.setTextDatum(MC_DATUM); 

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      int index = row * 3 + col;
      int x = spacing + col * (btnWidth + spacing);
      int y = spacing + row * (btnHeight + spacing);
      
      int centerX = x + (btnWidth / 2);
      int centerY = y + (btnHeight / 2);
      
      if (index < 8) {
        // Makro Buttons 1 bis 8 (Blau)
        tft.fillRoundRect(x, y, btnWidth, btnHeight, 6, TFT_NAVY);
        tft.drawRoundRect(x, y, btnWidth, btnHeight, 6, TFT_BLUE);
        tft.setTextColor(TFT_WHITE);
        
        String name = macroNames[index];
        int slashIndex = name.indexOf('/');
        
        // Beide Modi laufen jetzt auf Textgröße 2
        tft.setTextSize(2);
        
        if (slashIndex != -1) {
          // ZWEIZEILIG: Text splitten, aber TextSize bleibt groß (2)
          String line1 = name.substring(0, slashIndex);
          String line2 = name.substring(slashIndex + 1);
          line1.trim();
          line2.trim();
          
          // Abstand leicht vergrößert (+/- 12 statt 10), damit die große Schrift Platz hat
          tft.drawString(line1, centerX, centerY - 12);
          tft.drawString(line2, centerX, centerY + 12);
        } else {
          // EINZEILIG: In die exakte Mitte
          tft.drawString(name, centerX, centerY);
        }
      } else {
        // 9. Button: Zurück (Rot)
        tft.fillRoundRect(x, y, btnWidth, btnHeight, 6, TFT_MAROON);
        tft.drawRoundRect(x, y, btnWidth, btnHeight, 6, TFT_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.drawString("ZURUECK", centerX, centerY);
      }
    }
  }
}

// Funktion: Sendet Makros über universelle Windows-ALT-Codes (100% Layout-unabhängig)
void sendMacroSequence(String seq) {
  if (!bleKeyboard.isConnected()) {
    Serial.println("Bluetooth nicht verbunden!");
    return;
  }

  int i = 0;
  while (i < seq.length()) {
    
    // 1. STEUERTASTEN
    if (seq.substring(i).startsWith("[TAB]")) {
      bleKeyboard.releaseAll(); delay(100);
      bleKeyboard.press(KEY_TAB); delay(100);
      bleKeyboard.release(KEY_TAB); delay(100); 
      bleKeyboard.releaseAll(); delay(100); 
      i += 5;
    } 
    else if (seq.substring(i).startsWith("[RET]")) {
      bleKeyboard.println(""); delay(100); 
      i += 5;
    } 
    
    // 2. STRATEGIE-WEICHE NACH LAYOUT
    else {
      if (keyboardLayout == "US") {
        bleKeyboard.print(seq[i]);
        i++;
        delay(25);
      } 
      else {
        uint8_t byte1 = (uint8_t)seq[i];

        // --- A) UTF-8 MULTIBYTE-ABFANGEN (Gruppe 0xC3 - Umlaute & ß) ---
        if (byte1 == 0xC3 && i + 1 < seq.length()) {
          uint8_t byte2 = (uint8_t)seq[i+1];
          switch (byte2) {
            case 0xA4: bleKeyboard.print("'");  break; // ä -> US [']
            case 0xB6: bleKeyboard.print(";");  break; // ö -> US [;]
            case 0xBC: bleKeyboard.print("[");  break; // ü -> US [[]
            case 0x84: bleKeyboard.print("\""); break; // Ä -> US [Shift + ']
            case 0x96: bleKeyboard.print(":");  break; // Ö -> US [Shift + ;]
            case 0x9C: bleKeyboard.print("{");  break; // Ü -> US [Shift + []
            case 0x9F: bleKeyboard.print("-");  break; // ß -> US [-]
            default:   break; 
          }
          i += 2;
          delay(25);
        }
        // --- B) UTF-8 MULTIBYTE-ABFANGEN (Gruppe 0xC2 - Paragraph & Akzent) ---
        else if (byte1 == 0xC2 && i + 1 < seq.length()) {
          uint8_t byte2 = (uint8_t)seq[i+1];
          switch (byte2) {
            case 0xA7: bleKeyboard.print("#"); break; // § -> US [Shift + 3]
            case 0xB4: bleKeyboard.print("="); break; // ´ -> US [=]
            default:   break;
          }
          i += 2;
          delay(25);
        }
        // --- C) REINE 1-BYTE ASCII-MATRIX (Symmetrischer Tausch) ---
        else {
          char c = seq[i];
          bool handled = true;
          
          switch (c) {
            // HARDCORE-FIX: Die europäische ISO-Sondertaste (< > |) per explizitem uint8_t Scancode 0x64
            case '<': 
              bleKeyboard.press((uint8_t)0x64); delay(15); 
              bleKeyboard.release((uint8_t)0x64); break;
            case '>': 
              bleKeyboard.press(KEY_LSHIFT); delay(15); 
              bleKeyboard.press((uint8_t)0x64); delay(15);
              bleKeyboard.release((uint8_t)0x64); delay(15); 
              bleKeyboard.release(KEY_LSHIFT); break;
            case '|': 
              bleKeyboard.press(KEY_RALT); delay(15); 
              bleKeyboard.press((uint8_t)0x64); delay(15);
              bleKeyboard.release((uint8_t)0x64); delay(15); 
              bleKeyboard.release(KEY_RALT); break;

            // Buchstaben-Tausch (Z und Y)
            case 'z': bleKeyboard.print("y"); break;
            case 'Z': bleKeyboard.print("Y"); break;
            case 'y': bleKeyboard.print("z"); break;
            case 'Y': bleKeyboard.print("Z"); break;
            
            // Die obere Zahlen-Reihe (Shift-Sonderzeichen auf DE-PC)
            case '"': bleKeyboard.print("@"); break; // Erzeugt "
            case '&': bleKeyboard.print("^"); break; // Erzeugt &
            case '/': bleKeyboard.print("&"); break; // Erzeugt /
            case '(': bleKeyboard.print("*"); break; // Erzeugt (
            case ')': bleKeyboard.print("("); break; // Erzeugt )
            case '=': bleKeyboard.print(")"); break; // Erzeugt =
            case '?': bleKeyboard.print("_"); break; // Erzeugt ?
            case '`': bleKeyboard.print("+"); break; // Erzeugt `
            
            // Sonstige Satz- und Sonderzeichen im Hauptfeld
            case '-': bleKeyboard.print("/"); break; // Erzeugt -
            case '_': bleKeyboard.print("?"); break; // Erzeugt _
            case ':': bleKeyboard.print(">"); break; // Erzeugt :
            case ';': bleKeyboard.print("<"); break; // Erzeugt ;
            case '#': bleKeyboard.print("\\"); break;// Erzeugt #
            case '\'':bleKeyboard.print("|"); break; // Erzeugt '
            case '+': bleKeyboard.print("]"); break; // Erzeugt +
            case '*': bleKeyboard.print("}"); break; // Erzeugt *

            // Die verbleibenden AltGr-Kombinationen ({ [ ] } \ @ ~)
            case '{': 
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("7"); delay(15); bleKeyboard.release(KEY_RALT); break;
            case '[': 
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("8"); delay(15); bleKeyboard.release(KEY_RALT); break;
            case ']': 
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("9"); delay(15); bleKeyboard.release(KEY_RALT); break;
            case '}': 
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("0"); delay(15); bleKeyboard.release(KEY_RALT); break;
            case '\\':
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("-"); delay(15); bleKeyboard.release(KEY_RALT); break;
            case '@':
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("q"); delay(15); bleKeyboard.release(KEY_RALT); break;
            case '~': // KORRIGIERT: Hochkommas für das Tilde-Zeichen gesetzt
              bleKeyboard.press(KEY_RALT); delay(15); bleKeyboard.print("]"); delay(15); bleKeyboard.release(KEY_RALT); break;

            default:  
              handled = false; 
              break; 
          }
          
          if (!handled) {
            bleKeyboard.print(c);
          }
          
          i++;
          delay(25);
        }
      } // Ende DE-Zweig
    } // Ende Zeichen-Generierung
  } // Ende while
}

// Funktion: Überprüft Touch auf der Makro-Seite (Konvertiert rohe TS-Punkte in Pixel)
void handleMacroPageTouch(int pixelX, int pixelY) {
  tft.setFreeFont(NULL);
  tft.setTextFont(1);
  tft.setTextDatum(MC_DATUM);

  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      int index = row * 3 + col;
      int x = spacing + col * (btnWidth + spacing);
      int y = spacing + row * (btnHeight + spacing);
      
      int centerX = x + (btnWidth / 2);
      int centerY = y + (btnHeight / 2);

      if (pixelX >= x && pixelX <= (x + btnWidth) && pixelY >= y && pixelY <= (y + btnHeight)) {
        if (index < 8) {
          Serial.printf("Button '%s' gedrueckt. Sende Makro...\n", macroNames[index].c_str());
          
          String name = macroNames[index];
          int slashIndex = name.indexOf('/');
          
          // ─── VISUELLES FEEDBACK (ORANGE) ───
          tft.fillRoundRect(x, y, btnWidth, btnHeight, 6, createColor(255, 165, 0));
          tft.setTextColor(TFT_BLACK);
          tft.setTextSize(2); // Große Schrift beim Drücken erzeugen
          
          if (slashIndex != -1) {
            tft.drawString(name.substring(0, slashIndex), centerX, centerY - 12);
            tft.drawString(name.substring(slashIndex + 1), centerX, centerY + 12);
          } else {
            tft.drawString(name, centerX, centerY);
          }
          
          // Makro senden
          sendMacroSequence(macroStrings[index]);
          
          // Warte auf Touch-Release (Stift wird abgehoben)
          while (ts.touched()) { delay(10); }
          
          // ─── RESET IN DEN NORMALZUSTAND (BLAU) ───
          tft.fillRoundRect(x, y, btnWidth, btnHeight, 6, TFT_NAVY);
          tft.drawRoundRect(x, y, btnWidth, btnHeight, 6, TFT_BLUE);
          tft.setTextColor(TFT_WHITE);
          
          if (slashIndex != -1) {
            // SICHERHEITSHAKEN: TextSize unmittelbar vor dem Zeichnen erneut erzwingen!
            tft.setTextSize(2); 
            tft.drawString(name.substring(0, slashIndex), centerX, centerY - 12);
            tft.setTextSize(2); 
            tft.drawString(name.substring(slashIndex + 1), centerX, centerY + 12);
          } else {
            tft.setTextSize(2);
            tft.drawString(name, centerX, centerY);
          }
        } else {
          Serial.println("Zurueck-Button gedrueckt -> Gehe zu Kalender");
          function = 0;
          while (ts.touched()) { delay(10); }
          event_tm_hour = -1;
          event_tm_min = -1;
          event_tm_sec = -1;
        }
        return;
      }
    }
  }
}

// parse config.txt Lines to Var
void parseConfigLine(String line) 
{
  int separatorIndex = line.indexOf('=');
  if (separatorIndex == -1) return;

  String key = line.substring(0, separatorIndex);
  String value = line.substring(separatorIndex + 1);
  value.replace("\r", "");
  value.replace("\n", "");
  Serial.print(key + F("="));
  Serial.println(value);
  
  if (key == "ssid") {
    ssid = value;
  } else if (key == "password") {
    password = value;
  } else if (key == "tzinfo") {
    tzinfo = value;
  } else if (key == "ntpserver") {
    ntpserver = value;
  } else if (key == "tformat") {
    tformat = value;
  } else if (key == "brightness") {
    brightness = value.toInt();
  } else if (key == "function") {
    function = value.toInt();      
  } else if (key == "latitude") {
    latitude = value;
  } else if (key == "longitude") {
    longitude = value;
  } else if (key == "layout") {            // ─── NEU: Layout-Schalter für US/DE ───
    keyboardLayout = value;
    keyboardLayout.toUpperCase();
  } else if (key == "WeekDays") {
    int index = 0;
    int start = 0;
    int end = value.indexOf(',');
    while (end != -1) {
      WeekDays[index++] = value.substring(start, end);
      start = end + 1;
      end = value.indexOf(',', start);
    }
    WeekDays[index] = value.substring(start);
  } else if (key == "MonthName") {
    int index = 0;
    int start = 0;
    int end = value.indexOf(',');
    while (end != -1) {
      MonthName[index++] = value.substring(start, end);
      start = end + 1;
      end = value.indexOf(',', start);
    }
    MonthName[index] = value.substring(start);
  } else if (key == "Translate") {
    int index = 0;
    int start = 0;
    int end = value.indexOf(',');
    while (end != -1) {
      Translate[index++] = value.substring(start, end);
      start = end + 1;
      end = value.indexOf(',', start);
    }
    Translate[index] = value.substring(start);  
  } else if (key == "Year") {
    int index = 0;
    int start = 0;
    int end = value.indexOf(',');
    while (end != -1) {
      Year[index++] = value.substring(start, end);
      start = end + 1;
      end = value.indexOf(',', start);
    }
    Year[index] = value.substring(start);  
  } else if (key == "birthday") {
    birthday = value;
  } else if (key == "holiday0") {
    holiday[0] = value;
  } else if (key == "holiday1") {
    holiday[1] = value;
  } else if (key == "holiday2") {
    holiday[2] = value;  
  } else if (key == "schoolhol0") {
    schoolhol[0] = value;
  } else if (key == "schoolhol1") {
    schoolhol[1] = value;
  } else if (key == "schoolhol2") {
    schoolhol[2] = value;
  } 
  // ─── NEU: Makro-Tasten key1 bis key8 einlesen ───
  else if (key.startsWith("key")) {
    int keyNum = key.substring(3).toInt();
    if (keyNum >= 1 && keyNum <= 8) {
      int index = keyNum - 1;
      int commaIndex = value.indexOf(',');
      if (commaIndex != -1) {
        macroNames[index] = value.substring(0, commaIndex);
        macroStrings[index] = value.substring(commaIndex + 1);
        macroNames[index].trim();
      } else {
        macroNames[index] = value;
        macroStrings[index] = "";
      }
    }
  }
}

// read config.txt from SD Card
void read_sd()
{
  if (!SD.begin(SD_CS)) 
  {
    Serial.println("SD-Karte konnte nicht initialisiert werden.");
    return;
  }
  Serial.println("SD-Karte initialisiert.");

  File configFile = SD.open("/config.txt");
  if (configFile) 
  {
    while (configFile.available()) 
    {
      String line = configFile.readStringUntil('\n');
      parseConfigLine(line);
    }
    configFile.close();
  } 
  else 
  {
    Serial.println("Fehler beim Öffnen der Konfigurationsdatei.");
  }
  SD.end();
}

// Debug Touch Position
void printTouchToSerial(TS_Point p) 
{
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}

// Soft Timer for loop
bool SoftTimer(unsigned long time_period_set)
{
  static unsigned long time_start_ms;
  static unsigned long time_period_ms;
  static bool time_flag = false;
  bool bflag = false;

  if (time_period_set != 0)
  {
    time_start_ms = millis();
    time_period_ms = time_period_set;
    time_flag = true;
  }
  else
  {
    if (time_flag && ((millis() - time_start_ms) >= time_period_ms))
    {
      time_flag = false;
      bflag = true;
    }
  }
  return bflag;
}

// Start and Config CYD
void setup() 
{
  Serial.begin(115200);
  read_sd();

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display
  tft.init();
  tft.setRotation(1); //This is the display in landscape

  // Set Backlight brightness
  pinMode(backlightPin, OUTPUT);
  analogWrite(backlightPin, brightness);

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&seven_regular11pt7b);
  tft.drawString("CALENDAR V1.2", 0, 0);

  // STARTET den Bluetooth-Funk der Tastatur
  bleKeyboard.setLogLevel(HIDLogLevel::Normal);
  //bleKeyboard.set_locale(KEYBOARD_LOCALE_GERMAN); 
  bleKeyboard.begin();

  tft.setTextFont(1);

  tft.setTextColor(createColor(128, 255, 128), TFT_BLACK);
  tft.setCursor (0,30);
  tft.println("Calendar Start");

  if (ssid != "")
  {
    if (wifi_start_STA() == true)
    {
      Serial.println("Time Sync ...");
      tft.println("Time Sync ...");
      if (timesync() == true)
      {
        Serial.println("Time Sync Ready");
        tft.println("Time Sync Ready");
      }
      else
      {
        tft.setTextColor(createColor(255, 128, 128), TFT_BLACK);
        Serial.println("non Time sync");
        tft.println("non Time sync");
        delay(3000);
      }
    }
    else
    {
      tft.setTextColor(createColor(255, 128, 128), TFT_BLACK);
      Serial.println("non WiFi connect");
      tft.println("non WiFi connect");
      delay(3000);
    }
  }
  else
  {
    tft.setTextColor(createColor(255, 128, 128), TFT_BLACK);
    Serial.println("non SSID or SD Configuration");
    tft.println("non SSID or SD Configuration");
    delay(3000);
  }

  delay(500);
}

void loop() 
{
  struct tm localtime;
  getLocalTime(&localtime);
  time_t now;
  time(&now);
  struct tm *utctime = gmtime(&now);

  static char localtimeString[10]; // Buffer für die Zeit im Format HH.:MM:SS
  static char locaxtimeString[10]; // Buffer für die Zeit im Format HH.:MM SS
  char utctimeString[7]; // Buffer für die Zeit im Format HH:MM
  char dateString[11]; // Buffer für das Datum im Format TT.MO.YYYY
  char sunriseString[6]; // Buffer für sunriseString im Format HH:MM
  char sunsetString[6]; // Buffer für sunsetString im Format HH:MM
  char hourString[3]; //HH
  int timeZone;
  double sunrise;
  double sunset;
  double sunnow;
  
  // EVENT every hour (Wird nur auf Seite 0 und 1 gezeichnet)
  if (localtime.tm_hour != event_tm_hour && function != 2)
  {
    event_tm_hour = localtime.tm_hour;
    Serial.println("event_tm_hour");
    sprintf(dateString, "%02d.%02d.%04d", localtime.tm_mday, localtime.tm_mon + 1, localtime.tm_year + 1900);
    
    struct tm localtime_copy = localtime;
    localtime_copy.tm_isdst = 0; // DST deaktivieren
    timeZone = (mktime(&localtime_copy) - mktime(utctime)) / 3600;
  
    sun.setPosition(latitude.toDouble(), longitude.toDouble(), timeZone);
    sun.setCurrentDate(localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday);
    
    sunrise = sun.calcSunrise();
    sunset = sun.calcSunset();
  
    int sunrisehours = int(sunrise / 60); 
    int sunriseminutes = int((sunrise / 60 - sunrisehours) * 60);
    int sunsethours = int(sunset / 60); 
    int sunsetminutes = int((sunset / 60 - sunsethours) * 60);
    sprintf(sunriseString, "%02d:%02d", sunrisehours, sunriseminutes);
    sprintf(sunsetString, "%02d:%02d", sunsethours, sunsetminutes);

    if (function == 0)
    {
      // Urzustand für das Kalender-Layout wiederherstellen
      tft.setTextDatum(TL_DATUM); 
      tft.setTextSize(1);
      tft.setFreeFont(NULL);

      tft.fillScreen(TFT_WHITE);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.setCursor (0,0);
      yy_mem = localtime.tm_year;
      mm_mem = localtime.tm_mon;
      draw_cal(yy_mem + 1900, mm_mem + 1, localtime.tm_mday);
    }
    if (function == 1)
    {
      // Urzustand für das Uhren-Layout wiederherstellen
      tft.setTextDatum(TL_DATUM); 
      tft.setTextSize(1);
      tft.setFreeFont(NULL);

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_BLACK, TFT_WHITE);
      tft.setCursor (0,0);
      tft.setTextColor(TFT_WHITE, createColor(0, 0, 90));
      tft.drawString(dateString, 38, 0, 6);      
      
      double x_sunrise = sunrise * 320 / 1440;
      double x_sunset = sunset * 320 / 1440;
      int y_pos = 172;
      tft.fillRect(0 , y_pos + 27, x_sunrise , 10, createColor(0, 0, 90));
      tft.fillRect(x_sunrise , y_pos + 27, x_sunset - x_sunrise , 10, createColor(255, 255, 0));
      tft.fillRect(x_sunset , y_pos + 27, 319 - x_sunset , 10, createColor(0, 0, 90));
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(sunriseString, x_sunrise - 17, y_pos + 37, 2);
      tft.drawString("Sunrise", x_sunrise - 17, y_pos + 55, 1);
      tft.drawString(sunsetString, x_sunset - 17, y_pos + 37, 2);
      tft.drawString("Sunset", x_sunset - 17, y_pos + 55, 1); 
      for (int i = 0; i < 24 ; i += 2)
      {        
        tft.setTextColor(TFT_YELLOW, createColor(0, 120, 0));
        sprintf(hourString, "%02d", i);
        tft.drawString(hourString, (i * 320 / 24), y_pos + 9, 1);
      }   
      for (int i = 1; i < 24 ; i += 2)
      {        
        tft.setTextColor(TFT_YELLOW, createColor(0, 120, 0));
        sprintf(hourString, "%02d", i);
        tft.drawString(hourString, (i * 320 / 24), y_pos - 0, 1);
      }         
    }
  }

  // EVENT every min
  if (localtime.tm_min != event_tm_min)
  {
    event_tm_min = localtime.tm_min;
    Serial.println("event_tm_min");
    utctimeString[0] = '\0'; // Platzhalter zur Vermeidung von Warnings
    sunnow = localtime.tm_min + localtime.tm_hour * 60;
    if (function == 1)
    {
      double x_sunnow = sunnow * 320 / 1440;
      int y_pos = 172;
      tft.fillRect(0 , y_pos + 17, x_sunnow, 10, createColor(255, 0, 0));
      tft.fillRect(x_sunnow , y_pos + 17, 319 - x_sunnow, 10, createColor(128, 0, 0));
    }
  }

  // EVENT every sec (Zeitausgabe wird auf Makro-Schnittstelle unterdrückt)
  if (localtime.tm_sec != event_tm_sec && function != 2)
  {
    event_tm_sec = localtime.tm_sec;
    Serial.println("event_tm_sec");
    if (tformat == "24")
    {
      sprintf(localtimeString, "%02d:%02d:%02d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
      sprintf(locaxtimeString, "%02d:%02d %02d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
    }
    else
    {
      if (localtime.tm_hour < 13)
      {
        sprintf(localtimeString, "%02d:%02d:%02d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
        sprintf(locaxtimeString, "%02d:%02d %02d", localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
      }
      else
      {
        sprintf(localtimeString, "%02d.:%02d:%02d", localtime.tm_hour - 12, localtime.tm_min, localtime.tm_sec);
        sprintf(locaxtimeString, "%02d.:%02d %02d", localtime.tm_hour - 12, localtime.tm_min, localtime.tm_sec);
      }        
    } 
    if (function == 0)
    {
      tft.setTextDatum(TL_DATUM); // Sicherstellen für die Sprite-Ausgabe
      TFT_eSprite sprite = TFT_eSprite(&tft);
      sprite.createSprite(115, 22);
      sprite.fillSprite(createColor(128, 128, 128));
      sprite.setFreeFont(&seven_regular11pt7b);
      sprite.setTextColor(TFT_GREEN, createColor(0, 0, 0));
      sprite.drawString(localtimeString, 0, 0);
      sprite.pushSprite(198, 214);
      sprite.deleteSprite();
      SoftTimer(500);
    }
    if (function == 1)
    {
      tft.setTextDatum(TL_DATUM); // Sicherstellen für die Sprite-Ausgabe
      TFT_eSprite sprite = TFT_eSprite(&tft);
      sprite.createSprite(318, 61);
      sprite.fillSprite(createColor(128, 128, 128));
      sprite.setFreeFont(&seven_regular31pt7b);
      sprite.setTextColor(TFT_RED, TFT_BLACK);
      sprite.drawString(localtimeString, 0, 0);
      sprite.pushSprite(1, 78);
      sprite.deleteSprite();
      SoftTimer(500);
    }
  }

  if (SoftTimer(0) && function != 2)
  {
    if (function == 0)
    {
      tft.setTextDatum(TL_DATUM);
      TFT_eSprite sprite = TFT_eSprite(&tft);
      sprite.createSprite(115, 22);
      sprite.fillSprite(createColor(128, 128, 128));
      sprite.setFreeFont(&seven_regular11pt7b);
      sprite.setTextColor(TFT_GREEN, createColor(0, 0, 0));
      sprite.drawString(locaxtimeString, 0, 0);
      sprite.pushSprite(198, 214);
      sprite.deleteSprite();
    }
    if (function == 1)
    {
      tft.setTextDatum(TL_DATUM);
      TFT_eSprite sprite = TFT_eSprite(&tft);
      sprite.createSprite(318, 61);
      sprite.fillSprite(createColor(128, 128, 128));
      sprite.setFreeFont(&seven_regular31pt7b);
      sprite.setTextColor(TFT_RED, TFT_BLACK);
      sprite.drawString(locaxtimeString, 0, 0);
      sprite.pushSprite(1, 78);
      sprite.deleteSprite();
    }
  }

  // EVENT Pen touch
  if (ts.tirqTouched() && ts.touched()) 
  {
    TS_Point p = ts.getPoint();
    printTouchToSerial(p);
    
    // Berechne Pixel-Koordinaten (0-320 und 0-240) aus den rohen Touchwerten für das Makro-Grid
    int pixelX = map(p.x, 300, 3900, 0, 320);
    int pixelY = map(p.y, 300, 3900, 0, 240);

    // WENN WIR AUF DER MAKRO SEITE SIND: Direkt dorthin weiterleiten
    if (function == 2) {
      handleMacroPageTouch(pixelX, pixelY);
    } 
    // ANDERNFALLS: Normale Navigation (Seite 0 und 1)
    else {
      if (p.y < 800)
      {
        int brightness_step = 32;
        if (brightness < 64) brightness_step = 16; 
        if (brightness < 32) brightness_step = 8; 
        if (brightness < 16) brightness_step = 4; 
        if (brightness < 8)  brightness_step = 2; 
        if (brightness < 4)  brightness_step = 1; 
        
        if (p.x < 800)
        {
          brightness = brightness - brightness_step;
          if (brightness <= 0) brightness = 1;
        }
        if (p.x > 3200)
        {
          brightness = brightness + brightness_step;
          if (brightness >= 255) brightness = 255;
        }
        analogWrite(backlightPin, brightness);
        Serial.print("Brightness=");
        Serial.println(brightness);
      }

      if ((p.y > 800) && (p.y < 3300))
      {
        function = 0;
        if (p.x < 800)
        {
          int mm_mem_x = mm_mem;
          int yy_mem_x = yy_mem;
          mm_mem = mm_mem - 1;
          if (mm_mem < 0)
          {
            yy_mem = yy_mem - 1;
            mm_mem = 11;
          }
          int yy_mem_comp = Year[0].toInt() - 1900;
          if (yy_mem >= yy_mem_comp)
          {
            tft.setTextDatum(TL_DATUM); // Sicherstellen, dass das Datum beim Blättern stimmt
            draw_cal(yy_mem + 1900, mm_mem + 1, 0);
          }
          else
          {
            mm_mem = mm_mem_x;
            yy_mem = yy_mem_x;
          }          
        }
        if (p.x > 3200)
        {
          int mm_mem_x = mm_mem;
          int yy_mem_x = yy_mem;
          mm_mem = mm_mem + 1;
          if (mm_mem > 11)
          {
            yy_mem = yy_mem + 1;
            mm_mem = 0;
          }
          int yy_mem_comp = Year[2].toInt() - 1900;
          if (yy_mem <= yy_mem_comp)
          {
            tft.setTextDatum(TL_DATUM); // Sicherstellen, dass das Datum beim Blättern stimmt
            draw_cal(yy_mem + 1900, mm_mem + 1, 0);
          }
          else
          {
            mm_mem = mm_mem_x;
            yy_mem = yy_mem_x;
          }          
        }
        if ((p.x > 800) && (p.x < 3200))
        {
          yy_mem = localtime.tm_year;
          mm_mem = localtime.tm_mon;        
          event_tm_hour = -1;
          event_tm_min = -1;
          event_tm_sec = -1;
        }
      }
      
      if (p.y > 3300)
      {
        // Wenn bereits auf der Uhr (function == 1), schalte weiter auf Makros (function == 2)
        if (function == 1) {
          function = 2;
          Serial.println("Wechsle zur Makro-Schnittstelle (Seite 3)...");
          drawMacroPage(); // Diese Funktion setzt intern das MC_DATUM
        } else {
          // Wenn auf Kalender (0), schalte zur Uhr (1)
          function = 1;
          event_tm_hour = -1;
          event_tm_min = -1;
          event_tm_sec = -1;      
        }
      }
    }
    delay(300);
  }
}
//end
