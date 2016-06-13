/*
   16x Smart Button with Ultra Low Standby Power - Pushingbox example
   Date: 12. Jun, 2016

   Description: (will follow) 
   https://github.com/8n1/16x-smart-button
*/

// Wifi config
const char* ssid     = "xxxxx";
const char* password = "xxxxx";

// Pushingbox config
// Device ID of the pushingbox scenario to launch
const char* devid = "xxxxx";


// I2C pins (SDA = GPIO0, SCL = GPIO2)
const int sda = 0;
const int scl = 2;

// 'bootstrapping done' led (connected to GPIO13)
const int bootstrapping_done_led = 13;


#include <ESP8266WiFi.h>
#include <Wire.h>
#include "Adafruit_MCP23017.h"

Adafruit_MCP23017 mcp;

// Configure interrupts and activate pullups for all pins
void setup_mcp()
{
  Serial.println("Setting up interrupts...");

  // global interrupt config
  // mirroring(ORing interrupt lines?), openDrain(use open drain over push-pull?), polarity(f push-pull is used, should it be active HIGH or active LOW?)
  mcp.setupInterrupts(true, false, HIGH);

  // configure the 16 gpio pins
  for (int i = 0; i < 16; i++) {
    // go sure the pin is set to input
    mcp.pinMode(i, INPUT);
     // turn on a 100K pullup internally
    mcp.pullUp(i, HIGH); 
    // configure the interrupt to trigger when the pin is taken to ground
    mcp.setupInterruptPin(i, FALLING);
  }

  Serial.println("-> Done.\n");
}

// Connect to the wifi network.
// If done prints the ip, the time it took to get it and the wifi signal strength
void connect_to_wifi()
{
  Serial.print("Connecting to access point: ");
  Serial.println(ssid);

  // connect to the given ssid
  WiFi.begin(ssid, password);

  // wait for a ip from the DHCP
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

  Serial.println("-> Connected.");
  Serial.print(" IP: ");
  Serial.print(WiFi.localIP());
  Serial.printf(" (%i.%is)\n", millis() / 1000, millis() % 1000);
  Serial.print(" RSSI:");
  Serial.print( WiFi.RSSI());
  Serial.println("dBm\n");
}

// Launch the pushingbox scenario
// The request includes the number of the button that caused the interrupt and
// can be accessed using $button$ within the message that pushingbox will send
void send_request(int button)
{
  Serial.println("Connecting to api.pushingbox.com...");

  // create the http connection
  WiFiClient client;
  if (!client.connect("api.pushingbox.com", 80)) {
    Serial.println("-> Connection failed");
    return;
  }
  Serial.println("-> Done.");

  // if connected send the GET request,
  // include the number of the button
  Serial.println("Sending request");
  client.print(String("GET /pushingbox?devid=") + devid + "&button=" + button + " HTTP/1.1\r\n" +
               "Host: api.pushingbox.com\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("-> Done.");

  // wait for a response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    // but not longer then 5 seconds
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      shutdown();
    }
  }
  
  // print all lines from the response
  Serial.println("\nServer response:");
  Serial.println("-----------------------");
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println("\n-----------------------");
  
  Serial.println("-> Request sent. Connection closed.\n");
}

// Shut down the system
// The MCP interrupt will be cleared (interrupt line goes LOW again) as soon
// as the interrupt capture (or GPIO) registers are read from the MCP.
// Since the interrupt line is connected to the enable pin of the ldo this
// will turn it(ldo) and the ESP connected to it off.
void shutdown()
{
  Serial.println("Shuting down...");
  Serial.printf("Time elapsed: %i.%is\n\n", millis() / 1000, millis() % 1000);
  delay(20);
  uint8_t val = mcp.getLastInterruptPinValue(); // reads the INTCAPA & INTCAPB register
  while (true) delay(200);
}

// MAIN
void setup()
{
  // Setup the serial port
  Serial.begin(115200);
  Serial.println("16x Smart Button - Pushingbox Example\n");

  // Setup I2C
  Wire.begin(sda, scl);
  // Address bits are set to 0
  mcp.begin(0);

  // Check which button/pin caused the interrupt
  Serial.println("Getting interrupt pin...");
  int pin = mcp.getLastInterruptPin();  // reads the INTFA & INTFB register
  pin = pin + 1; // make it human friendly / convert pin to button number
  Serial.printf("-> Pin: %i\n\n", pin);

  // Setup mcp interrupts if powered up for the first time
  // (the interrupt pin is set to 255 initially)
  if (pin == 256) {
    // setup MCP interrupts
    setup_mcp();

    // light up the "bootstrapping done" led
    pinMode(bootstrapping_done_led, OUTPUT);
    digitalWrite(bootstrapping_done_led, HIGH);

    // bootstrapping done
    Serial.println("Bootstrapping done.");
    Serial.println("You can now release the 'setup mcp' switch");
    Serial.println("and press one of the 16 buttons. Have fun!");
    while (true) delay(100);
  }

  // Connect to the wifi network
  connect_to_wifi();

  // Launch the Pushingbox scenario, include the interrupt pin as variable ($button$)
  send_request(pin);

  // Shutdown
  shutdown();
}

void loop()
{
  // nothing in here
}


