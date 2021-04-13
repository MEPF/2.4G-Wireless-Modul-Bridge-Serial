//======================================================================================//
//                          BRIDGE - WiFi <> ModBus V1.0.0 --- TCP                      //
//                                   ModBus Over TCP/IP                                 //
//                                       15.10.2020                                     //
//                                       M.Emanuel                                      //
//======================================================================================//

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>


////////////////////////////////////// CONFIG ////////////////////////////////////////////
//#define MODE_AP                                                 						//// phone connects directly to ESP
#define MODE_STA                                                  						//// ESP connects to WiFi router
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef MODE_AP                                                                			/// For AP mode:
const char* d_name = "MODBUS_PARTIER";                                        			// Network NAME
const char* devicePassword = "KmrI@T";                                        			// Passwoard firmware upgrade
const char *ssid = "SENIS_MODBUS";                                						// You will connect your phone to this Access Point
const char *pw = "KmrI@T";                                        						// And this is the password
#endif

#ifdef MODE_STA                                                               			/// For STATION mode:
const char* d_name = "MODBUS_TEST";                   						  			// Network NAME
const char* devicePassword = "KmrI@T";                                        			// Passwoard firmware upgrade
const char* ssid = "Your SSID";                               				  			// Your ROUTER SSID
const char* password = "Your PASS";                              						// Your WiFi PASSWORD

IPAddress staticIP(192,168,14,31);                               		 				// Static IP adress
IPAddress subnet(255,255,254,0);                                 					 	// Subnet Mask
IPAddress gateway(192,168,15,253);                                						// Gateway IP adresa
#endif
//////////////////////////////////////////////////////////////////////////////////////////
#define UART_BAUD 19200                                                       			// Baud Rate 19200
#define bufferSize 1000                                                       			// bufferSize data UART <> WIFI
#define connectionTimeout 900000                                              			// Time keep the connection alive 15min
////////////////////////////////////// END CONFIG ////////////////////////////////////////

WiFiServer server(502);

unsigned long currentTime = millis();                                         			// Current time
unsigned long previousTime = 0;                                               			// Previous time 

uint8_t buf1[bufferSize];
uint16_t i1=0;

uint8_t buf2[bufferSize];
uint16_t i2=0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);                                      					// initialize the LED_BUILTIN pin as an output
  Serial.begin(UART_BAUD, SERIAL_8N1);
  delay(10);
  
  #ifdef MODE_AP 
  //AP mode (connects directly to ESP) (no router)
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pw);                                             					// configure ssid and password for softAP
  WiFi.setSleep(false);                                              					// this code solves my problem

  ArduinoOTA.setHostname(d_name);
  ArduinoOTA.setPassword(devicePassword);
  ArduinoOTA.begin();
  #endif

  #ifdef MODE_STA
  // STATION mode (ESP connects to router and gets an IP)
  WiFi.mode(WIFI_STA);
  WiFi.hostname(d_name);                                             					// network name in the network
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println('\n');
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println();
  Serial.printf("Starea conexiunii: %d\n", WiFi.status());
  Serial.print("Adresa IP conectata: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  WiFi.printDiag(Serial);
  Serial.println();
  Serial.print("Access point MAC address: ");
  Serial.println(WiFi.BSSIDstr());
  Serial.println();
//---------------------------------------------------------------------------
  ArduinoOTA.setHostname(d_name);
  ArduinoOTA.setPassword(devicePassword);
  ArduinoOTA.begin();
  #endif
  
  Serial.println();
  Serial.println("Starting Mod-Bus TCP Server - Port: 502");
  server.begin();                                                    					// start TCP server 
}


void loop() {
  ArduinoOTA.handle();                                                        			// Wifi firmware upgrade

  WiFiClient client = server.available();                                     			// Listen for incoming clients
  
  if (client) {                                                               			// If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    
    while (client.connected() && currentTime - previousTime <= connectionTimeout) {
      currentTime = millis();
                                                                              
      if (client.available()) {                                               			// if there's bytes to read
        while(client.available()) {
          buf1[i1] = (uint8_t)client.read();                                  			// read char from CLIENT
          if(i1<bufferSize-1) i1++;
      }
        Serial.write(buf1, i1);                                               			// now send to UART
        i1 = 0;
      }

      if(Serial.available()) {                                                			// if there's bytes to read                                                       
        while(Serial.available()) {
          buf2[i2] = (uint8_t)Serial.read();                                  			// read char from UART
          if(i2<bufferSize-1) i2++;
        }                                              
      client.write(buf2, i2);                                                 			// now send to client
      i2 = 0;
      }
  
    }																					
    client.stop();																		// Close the connection
  }
}
