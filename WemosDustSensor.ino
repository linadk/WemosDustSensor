#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "PMS.h"

PMS pms(Serial);
PMS::DATA data;

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 OLED(OLED_RESET);
#define MODE_TOTALS 1
#define MODE_GRAMS 2

byte dispMode = MODE_TOTALS;
 
void setup()   {
  OLED.begin();
  OLED.clearDisplay();
 
  //Add stuff into the 'display buffer'
  OLED.setTextWrap(false);
  OLED.setTextSize(1);
  OLED.setTextColor(WHITE);
  OLED.setCursor(0,0);
  OLED.println("Booting...");
 
  OLED.display(); //output 'display buffer' to screen  

  Serial.begin(9600);   // GPIO1, GPIO3 (TX/RX pin on ESP-12E Development Board)
} 
 
void loop() {
  
  if (pms.read(data))
  {
    OLED.clearDisplay();
    OLED.setCursor(0,0);

    switch(dispMode){
      case MODE_TOTALS:
          OLED.println("Total particles");

          // 0.3-0.5
          OLED.print(".3M: "); OLED.print(data.PM_TOTALPARTICLES_0_3);  
          OLED.print(" .5M: "); OLED.println(data.PM_TOTALPARTICLES_0_5);

          // 1.0-2.5
          OLED.print("1M: "); OLED.print(data.PM_TOTALPARTICLES_1_0);  
          OLED.print(" 2.5M: "); OLED.println(data.PM_TOTALPARTICLES_2_5);

          // 5.0-10.0
          OLED.print("5M: "); OLED.print(data.PM_TOTALPARTICLES_5_0);  
          OLED.print(" 10M: "); OLED.println(data.PM_TOTALPARTICLES_10_0);


          //OLED.print("PM 0.5 : ");
          //OLED.println(data.PM_TOTALPARTICLES_0_5);

          //OLED.print("PM 1.0 : ");
          //OLED.println(data.PM_TOTALPARTICLES_1_0);

          break;
          
      case MODE_GRAMS:
          OLED.println("Micrograms per Meter^3");
          OLED.print("PM 1.0 (ug/m3): ");
          OLED.println(data.PM_AE_UG_1_0);

          OLED.print("PM 2.5 (ug/m3): ");
          OLED.println(data.PM_AE_UG_2_5);

          OLED.print("PM 10.0 (ug/m3): ");
          OLED.println(data.PM_AE_UG_10_0);

          break;
      
    }


    OLED.display();
    delay(0.25);

  }

  // Do other stuff...
}
