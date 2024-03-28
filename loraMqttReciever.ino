/***************************************************
  Adafruit MQTT Library Arbitrary Data Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Stuart Feichtinger
  Modifed from the mqtt_esp8266 example written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <WiFi.h>
#include <heltec.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************ Laura Stuff ****************************************/
#define FREQUENCY           905.2
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    12
#define TRANSMIT_POWER      22

String rxdata;
volatile bool rxFlag = false;

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "Haystack Guest"
#define WLAN_PASS       "guestwifi"

/************************* Adafruit.io Setup *********************************/

#define ARB_SERVER      "192.168.1.103"
#define ARB_SERVERPORT  1883                   // use 8883 for SSL
#define ARB_USERNAME    "fab"
#define ARB_PW          "fab"


/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, ARB_SERVER, ARB_SERVERPORT, ARB_USERNAME, ARB_PW);

/****************************** Feeds ***************************************/

// Setup a feed called 'arb_packet' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
#define ARB_FEED "/feeds/lora_packets"
Adafruit_MQTT_Publish ap = Adafruit_MQTT_Publish(&mqtt, ARB_FEED);


// Arbitrary Payload
// Union allows for easier interaction of members in struct form with easy publishing
// of "raw" bytes
typedef union{
    //Customize struct with whatever variables/types you like.

    struct __attribute__((__packed__)){  // packed to eliminate padding for easier parsing.
        char charAry[10];
        int16_t val1;
        unsigned long val2;
        uint16_t val3;
    }s;

    uint8_t raw[sizeof(s)];                    // For publishing

    /*
        // Alternate Option with anonymous struct, but manual byte count:

        struct __attribute__((__packed__)){  // packed to eliminate padding for easier parsing.
            char charAry[10];       // 10 x 1 byte  =   10 bytes
            int16_t val1;           // 1 x  2 bytes =   2 bytes
            unsigned long val2;     // 1 x  4 bytes =   4 bytes
            uint16_t val3;          // 1 x  2 bytes =   2 bytes
                                            -------------------
                                            TOTAL =    18 bytes
        };
        uint8_t raw[18];                    // For publishing
*/

} packet_t;

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  heltec_setup();
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  both.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  both.println(); Serial.println();
  both.print(F("Connecting to "));
  both.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    both.print(F("."));
  }
  both.println();

  both.println(F("WiFi connected"));
  both.println(F("IP address: ")); Serial.println(WiFi.localIP());

}

packet_t arbPac;

void publishMqtt(String value) {
  MQTT_connect();
  char _value[value.length() + 1];
  value.toCharArray(_value, sizeof(_value));
  strcpy_P(arbPac.s.charAry, _value);
  if (! ap.publish(arbPac.raw, sizeof(packet_t))) {
    Serial.println(F("Publish Failed."));
    Serial.println(F(millis()));  
  }else {
    Serial.println(F("Publish Success!"));
    //Serial.println(F(millis()));
    delay(500);
  }
}


void loop() {
  heltec_loop();
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    String value = rxdata.c_str();
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("%s\n", value);
      publishMqtt(value);
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
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

  Serial.print(F("Connecting to MQTT... "));

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println(F("Retrying MQTT connection in 5 seconds..."));
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println(F("MQTT Connected!"));
}

void rx() {
  rxFlag = true;
}
