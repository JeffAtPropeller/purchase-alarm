#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "SSID"
#define WLAN_PASS       "password"

/************************* DFPlayer Setup *********************************/

SoftwareSerial dfPlayerSerial(10, 11); // RX, TX
DFRobotDFPlayerMini dfPlayer;

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

/************************* MQTT Setup *********************************/

#define AIO_SERVER      "m10.cloudmqtt.com"
#define AIO_SERVERPORT  23121
#define AIO_USERNAME    "alarm"
#define AIO_KEY         "Password1"

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Setup MQTT Feeds ***************************************/

// Setup the feed 
Adafruit_MQTT_Subscribe purchaseFeed = Adafruit_MQTT_Subscribe(&mqtt, "shopify/purchases");
Adafruit_MQTT_Subscribe volumeFeed = Adafruit_MQTT_Subscribe(&mqtt, "control/volume");
Adafruit_MQTT_Subscribe onOffFeed = Adafruit_MQTT_Subscribe(&mqtt, "control/onoff");

/*************************** Sketch Code ************************************/
void purchaseCallback(double x) {
  Serial.print("Hey we're in a purchase callback, the slider value is: ");
  Serial.println(x);
}

void onOffCallback(bool x) {
  Serial.print("Hey we're in a onoff callback, the value is: ");
  Serial.println(x);
}

void volumeCallback(int x) {
  Serial.print("Hey we're in a volume callback, the slider value is: ");
  Serial.println(x); 
}

void setup() {
  dfPlayerSerial.begin(9600);
  Serial.begin(115200);
  
  if (!dfPlayer.begin(dfPlayerSerial)) {  //Use dfPlayerSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  } 
  Serial.println(F("DFPlayer Mini online."));
  
  delay(10);
  Serial.println(F("MQTT Purchase Alarm started"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  
  // Setup MQTT subscriptions.
  mqtt.subscribe(&purchaseFeed);
  mqtt.subscribe(&volumeFeed);
  mqtt.subscribe(&onOffFeed);
}

uint32_t x=0;


void loop() {
  bool isRunning = true;

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
    
  while ((subscription = mqtt.readSubscription(5000))) {   
    // Check if its the onoff button feed
    if (subscription == &onOffFeed) {
      Serial.print(F("On-Off feed: "));
      Serial.println((char *)onOffFeed.lastread);
      
      if (strcmp((char *)onOffFeed.lastread, "ON") == 0) {
        isRunning = true;
//        Serial.print(F("Turned ON));
      }
      if (strcmp((char *)onOffFeed.lastread, "OFF") == 0) {
        isRunning = false;
//        Serial.print(F("Turned OFF));
      }
    }
    
    // check if its the volume feed
    if (subscription == &volumeFeed) {
      Serial.print(F("Volume Feed: "));
      Serial.println((char *)volumeFeed.lastread);
      int volume = atoi((char *)volumeFeed.lastread);
      dfPlayer.volume(volume);
    }

    if (subscription == &purchaseFeed) {
      Serial.print(F("Purchase Feed: "));
      Serial.println((char *)purchaseFeed.lastread);
      dfPlayer.play(0);
    }
  }
  
  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}


