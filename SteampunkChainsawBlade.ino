/* Chainsaw - For Pete's Steampunk Chainsaw
*  
*  Copyright 2020 - Chris Knight - merlin@ghostwheel.com
*  Released under the CC BY-NC-SA license: https://creativecommons.org/licenses/by-nc-sa/4.0/
*/


#include <APA102.h>   // https://github.com/pololu/apa102-arduino
#include <EEPROM.h>   // https://github.com/Chris--A/EEPROM
//#include <LowPower.h> // https://github.com/rocketscream/Low-Power

// Define which pins to use.
#define DATAPIN 16
#define CLOCKPIN 15

// Create an object for writing to the LED strip.
APA102<DATAPIN, CLOCKPIN> ledStrip;

// Set the number of LEDs to control.
#define MAXLEDS 116
uint16_t ACTIVELEDS = 0;

// Create a buffer for holding the colors (3 bytes per color).
rgb_color colors[MAXLEDS]; // Strip used was actually GRB
byte r, b, g;
float hsvdrift;

// This structure is used to read and write values from EEPROM
struct SettingsObject {
  int NumberOfLEDs;
  int checksum;
};

int skipPixel = 0;
int red;
int green;
float intensity = 1;
float chromatography = .75;
int currentSkip = 0;

void setup()
{
  pinMode(2, INPUT_PULLUP); // 0 = rainbow mode
  pinMode(3, INPUT_PULLUP); // 0 = fire mode
  pinMode(4, INPUT_PULLUP); // 0 = set strip length
  
  ACTIVELEDS = readSettingsFromEEPROM(MAXLEDS);
  hsvdrift = 360/ACTIVELEDS;
  setToBlack();
}



void loop()
{
  float htemp = 0;
  float hbase = 0;
  while ( 0 == digitalRead(2) ) {
    htemp = hbase;
    for (int pixtemp = 0; ACTIVELEDS > pixtemp; pixtemp++){
      HSVtoRGB( htemp, 1.0 ,1.0, &r, &g, &b);
      colors[pixtemp] = rgb_color(g, r, b);  // Strip used was actually GRB
      htemp += hsvdrift;
      if ( 360 <= htemp ) {
        htemp -= 360;
      }
      
    }
    ledStrip.write(colors, MAXLEDS, 12);

    // Move the colors along
    hbase -= (int)map(analogRead(A0), 0, 1020, 0, 30);
    if ( 0 >= hbase ) {
      hbase += 360;
    }
    lengthSetCheck();
  }

  if ( 1 == digitalRead(2) &&  1 == digitalRead(3) ) {
      setToBlack();
      while ( 1 == digitalRead(2) &&  1 == digitalRead(3) ) {
        lengthSetCheck();
      }
  }

  while ( 0 == digitalRead(3) ) {
    for (int pixtemp = 0; ACTIVELEDS > pixtemp; pixtemp++){
      if (currentSkip == skipPixel) {
        colors[pixtemp] = rgb_color(0, 0, 0); // Strip used was actually GRB
      }
      else {
        red=(int)(random(10,256) * intensity);
        green=(int)(random(10,(red * chromatography +1)) * intensity);
        colors[pixtemp] = rgb_color(green, red, 0);  // Strip used was actually GRB
      }
      currentSkip++;
      if ( 3 < currentSkip ) {
        currentSkip = 0;
      }
    }
    ledStrip.write(colors, MAXLEDS, 12);
    skipPixel++;
    if ( 3 < skipPixel ) {
      skipPixel = 0;
    }
    delay(map(analogRead(A0), 1023, 0, 0, 80));
    lengthSetCheck();
  }  
}

void lengthSetCheck() {
  /*
   * - This functiuon allows for on-the-fly setting of the active LEDs by way of a POT on A0
   * - Range is from 1 to MAXLEDS
   * - Once the button on D4 is released, the length is saved to EEPROM so it can be read
   *   on the next boot. 
   */
  if ( 0 == digitalRead(4) ) {
    while ( 0 == digitalRead(4) ) {
      ACTIVELEDS=(int)map(analogRead(A0), 0, 1020, 1, MAXLEDS);
      for ( int i = 0; i < (ACTIVELEDS -1); i++) {
        colors[i] = rgb_color(0, 255, 0);  // Strip used was actually GRB
      }
      colors[(ACTIVELEDS -1)] = rgb_color(0, 0, 255); // Strip used was actually GRB
      for ( int i = ACTIVELEDS; i < MAXLEDS; i++) {
        colors[i] = rgb_color(0, 0, 0); // Strip used was actually GRB
      }
      ledStrip.write(colors, MAXLEDS,31);
    }
    writeSettingsToEEPROM(ACTIVELEDS);
    hsvdrift = 360/ACTIVELEDS;
    setToBlack();
  }
}

int readSettingsFromEEPROM(int defaultLEDs) {

  SettingsObject tempVar; //Variable to store custom object read from EEPROM.
  EEPROM.get(0, tempVar);

  if ( ((tempVar.NumberOfLEDs + 69) * 42) == tempVar.checksum) {
    return(tempVar.NumberOfLEDs);
  }
  else {
    return(defaultLEDs);
  }
}

void writeSettingsToEEPROM(int currentLEDs) {
  //Data to store.
  SettingsObject tempVar = {
    currentLEDs,
    (( currentLEDs + 69) * 42)
  };
  EEPROM.put(0, tempVar);
}

/* Convert hsv values (0<=h<360, 0<=s<=1, 0<=v<=1) to rgb values (0<=r<=255, etc) */
void HSVtoRGB(float h, float s, float v, byte *r, byte *g, byte *b) {
  int i;
  float f, p, q, t;
  float r_f, g_f, b_f;

  if( s < 1.0e-6 ) {
    /* grey */
    r_f = g_f = b_f = v;
  }
  
  else {
    h /= 60.0;              /* Divide into 6 regions (0-5) */
    i = (int)floor( h );
    f = h - (float)i;      /* fractional part of h */
    p = v * ( 1.0 - s );
    q = v * ( 1.0 - s * f );
    t = v * ( 1.0 - s * ( 1.0 - f ) );

    switch( i ) {
      case 0:
        r_f = v;
        g_f = t;
        b_f = p;
        break;
      case 1:
        r_f = q;
        g_f = v;
        b_f = p;
        break;
      case 2:
        r_f = p;
        g_f = v;
        b_f = t;
        break;
      case 3:
        r_f = p;
        g_f = q;
        b_f = v;
        break;
      case 4:
        r_f = t;
        g_f = p;
        b_f = v;
        break;
      default:    // case 5:
        r_f = v;
        g_f = p;
        b_f = q;
        break;
    }
  }
  
  *r = (byte)floor(r_f*255.99);
  *g = (byte)floor(g_f*255.99);
  *b = (byte)floor(b_f*255.99);
}

void setToBlack() {
/* 
   Write 0,0,0 to all LESs twice, justr to make sure.  :)
*/
  for(uint16_t i = 0; i < MAXLEDS; i++) {
    colors[i] = rgb_color(0, 0, 0); // Strip used was actually GRB
  }
  ledStrip.write(colors, MAXLEDS, 31);
  ledStrip.write(colors, MAXLEDS, 31);
}
