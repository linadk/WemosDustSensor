#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#include <AmazonIOTClient.h>
#include <ESP8266AWSImplementations.h>

#include "PMS.h"

PMS pms(Serial);
PMS::DATA data;

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 OLED(OLED_RESET);
#define MODE_TOTALS 1
#define MODE_GRAMS 2
#define MODE_GRAMS_CF0 3
#define MODE_INFO 4
#define MODE_AWS_INFO 5
#define NUM_TOTAL_MODES 5

// Toggle Pin
const int togglePin = D7;     // the number of the pushbutton pin
int toggleState = 1;          // Pin is pulled up high by default
int lastToggleState = 1;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 500;

// Wifi 
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_WIFI_PASS";
#define WIFI_CONNECT_ATTEMPTS_INTERVAL 500 // Wait 500ms intervals for wifi connection
#define WIFI_CONNECT_MAX_ATTEMPTS 10 // Number of attempts/intervals to wait

// AWS
Esp8266HttpClient http_client;
Esp8266DateTimeProvider dateTimeProvider;
AmazonIOTClient iot_client;
ActionError actionError;
String shadow;

byte dispMode = MODE_TOTALS;
 
void setup()   {
  OLED.begin();
  OLED.clearDisplay();
 
  // Set up screen
  OLED.setTextWrap(false);
  OLED.setTextSize(1);
  OLED.setTextColor(WHITE);
  OLED.setCursor(0,0);
  OLED.println("Booting...");
  OLED.display();

  if(InitWifi()){
    OLED.println("Wifi [CONNECTED]");
  } else {
    OLED.println("Wifi [FAILED]");
  }
  OLED.display();
  delay(1000);

  if(InitAWS()){
    OLED.println("AWS [CONNECTED]");
  } else {
    OLED.println("AWS [FAILED]");
  }
  OLED.display();
  delay(1000);
 
  OLED.display(); //output 'display buffer' to screen  



  Serial.begin(9600);   // GPIO1, GPIO3 (TX/RX pin on ESP-12E Development Board)

  pinMode( togglePin , INPUT_PULLUP ); // Pin for switching screens button

} 

// Handle reading the display change pin and switching the display mode
void DisplayToggle(){

  lastToggleState = toggleState;
  toggleState = digitalRead(togglePin);

  // If button is pressed and it is different from last state.
  if(toggleState == LOW && toggleState != lastToggleState ){

      // We haven't waited long enough so consider press illegitimate
      if(millis() - lastDebounceTime <= debounceDelay){
        return;
      }

      // Switch display
      lastDebounceTime = millis();
      if(dispMode >= NUM_TOTAL_MODES){
        dispMode = 1;
        return;
      }
      else{
        dispMode++;
        return;
      }
   }
}
 
void loop() {
  DisplayToggle();
  
  if (pms.read(data))
  {
    OLED.clearDisplay();
    OLED.setCursor(0,0);

    // Render our displays
    switch(dispMode){
      case MODE_TOTALS:
          OLED.setTextWrap(false);
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

          break;
          
      case MODE_GRAMS:
          OLED.setTextWrap(false);
          OLED.println("ug/m^3 (Atmos.)");
          OLED.print("PM 1.0 (ug/m3): ");
          OLED.println(data.PM_AE_UG_1_0);

          OLED.print("PM 2.5 (ug/m3): ");
          OLED.println(data.PM_AE_UG_2_5);

          OLED.print("PM 10.0 (ug/m3): ");
          OLED.println(data.PM_AE_UG_10_0);


          break;
      case MODE_GRAMS_CF0:
          OLED.setTextWrap(false);
          OLED.println("ug/m^3 (CF=1)");
          OLED.print("PM 1.0 (ug/m3): ");
          OLED.println(data.PM_AE_UG_1_0);

          OLED.print("PM 2.5 (ug/m3): ");
          OLED.println(data.PM_AE_UG_2_5);

          OLED.print("PM 10.0 (ug/m3): ");
          OLED.println(data.PM_AE_UG_10_0);

          break;
      case MODE_INFO:
        OLED.setTextWrap(false);
        OLED.println("Info");
        OLED.print("IP: ");
        OLED.println(WiFi.localIP());
        OLED.print("SSID: ");
        OLED.println(ssid);
        OLED.print("Connected: ");
        OLED.println(WiFi.status() == WL_CONNECTED);
        
        break;

      case MODE_AWS_INFO:
        OLED.setTextWrap(true);
        OLED.println("AWS Info:");
        OLED.println(shadow);
        OLED.println(actionError);
        break;
      
      default:
          OLED.println(dispMode);
          break;
      
    }

    OLED.display();
    PushAWS();
    delay(0.25);

  }
}

// Connect to Wifi! Returns false if can't connect.
bool InitWifi(){
  // Clean up any old auto-connections
  if(WiFi.status() == WL_CONNECTED){ WiFi.disconnect(); }
  WiFi.setAutoConnect(false);

  // RETURN: No SSID no wifi!
  if(sizeof(ssid) == 1){ return false; }

  // Connect to wifi
  WiFi.begin(ssid,password);

  // Wait for connection set amount of intervals
  int num_attempts = 0;
  while(WiFi.status() != WL_CONNECTED && num_attempts <= WIFI_CONNECT_MAX_ATTEMPTS)
  {
    delay(WIFI_CONNECT_ATTEMPTS_INTERVAL);
    num_attempts++;
  }

  if(WiFi.status() != WL_CONNECTED){ 
    return false;
  }
  else
  {
    return true;
  }
}

void PushAWS(){
  shadow = "";
  shadow = "{\"state\":{\"reported\":{\"test_value1\":" + String(data.PM_TOTALPARTICLES_0_5) +
                                    ", \"test_value2\":" + String(data.PM_TOTALPARTICLES_2_5) + 
                                    "}}}";

  // Disabled for testing
  //char* result = iot_client.update_shadow( shadow.c_str() , actionError );
}

// Initialize AWS connection
bool InitAWS(){
  iot_client.setAWSRegion("us-east-1");
  iot_client.setAWSEndpoint("amazonaws.com");
  iot_client.setAWSDomain("xxx.amazonaws.com");
  iot_client.setAWSPath("/things/XXX/shadow");
  iot_client.setAWSKeyID("XXX");
  iot_client.setAWSSecretKey("YYY");
  iot_client.setHttpClient(&http_client);
  iot_client.setDateTimeProvider(&dateTimeProvider);
  return true;
}
