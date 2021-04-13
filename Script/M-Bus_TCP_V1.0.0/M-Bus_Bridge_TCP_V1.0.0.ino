
//======================================================================================//
//                       BRIDGE - WiFi <> M-Bus V1.0.0 --- TCP                          //
//                                M-Bus Over TCP/IP                                     //
//                                    20.10.2020                                        //
//                                    M. Emanuel                                        //
//======================================================================================//

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>

////////////////////////////////////// CONFIG ////////////////////////////////////////////

#define UART_BAUD 2400                                            						// Baud Rate 2400

#define packTimeout 5                                             						// ms (if nothing more on UART, then send packet)
#define bufferSize 8192

//#define MODE_AP                                                 						// phone connects directly to ESP
#define MODE_STA                                                  						// ESP connects to WiFi router


#ifdef MODE_AP
// For AP mode:
const char *ssid = "SENIS_MBUS";                                  						// You will connect your phone to this Access Point
const char *pw = "KmrI@T";                                        						// And this is the password
const int port = 2000;                                            						// And this port
// You must connect the phone to this AP, then:
// menu -> connect -> Internet(TCP) -> 192.168.0.1:2000
#endif

#ifdef MODE_STA
// For STATION mode:
const char* d_name = "MBUS_TEST";                      						  // Network NAME
const char* devicePassword = "KmrI@T";                                        // Passwoard firmware upgrade
const char* ssid = "Your SSID";                               						// Your ROUTER SSID
const char* password = "Your PASS";                              						// Your WiFi PASSWORD

IPAddress staticIP(192,168,14,93);                                						// Static IP adress
IPAddress subnet(255,255,254,0);                                  						// Subnet Mask
IPAddress gateway(192,168,15,253);                                						// Gateway IP adresa

const int port = 2000;
// You must connect the phone to the same router,
// Then somehow find the IP that the ESP got from router, then:
// menu -> connect -> Internet(TCP) -> [ESP_IP]:2000
#endif

//////////////////////////////////// END CONFIG //////////////////////////////////////////

WiFiServer server(port);
WiFiClient client;

uint8_t buf1[bufferSize];
uint16_t i1=0;

uint8_t buf2[bufferSize];
uint16_t i2=0;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);                                    						// Initialize the LED_BUILTIN pin as an output
  Serial.begin(UART_BAUD, SERIAL_8E1);
  delay(10);
  
  #ifdef MODE_AP 
  //AP mode (phone connects directly to ESP) (no router)
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pw);                                          						// configure ssid and password for softAP
  WiFi.setSleep(false);                                              					// this code solves my problem
  #endif

  
  #ifdef MODE_STA
  // STATION mode (ESP connects to router and gets an IP)
  // Assuming phone is also connected to that router
  // from RoboRemo you must connect to the IP of the ESP
  WiFi.mode(WIFI_STA);
  WiFi.hostname(d_name); // Nume bord in retea
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println('\n');
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println();
  Serial.printf("\n Starea conexiunii: %d\n", WiFi.status());
  Serial.print("Adresa IP conectata: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  WiFi.printDiag(Serial);
  //---------------------------------------------------------------------------
  ArduinoOTA.setHostname(d_name);
  ArduinoOTA.setPassword(devicePassword);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  #endif

  Serial.println("Starting M-Bus TCP Server - Port: 2000");
  server.begin();                                                    					// start TCP server 
}


void loop() {
  ArduinoOTA.handle();                                                        // Wifi firmware upgrade
  
  if(!client.connected()) {                                          					// if client not connected
    client = server.available();                                     					// wait for it to connect
    return;
  }

  if(client.available()) {																                    // here we have a connected client
    while(client.available()) {
      buf1[i1] = (uint8_t)client.read();                             					// read char from client
      if(i1<bufferSize-1) i1++;
    }
    // now send to UART:
    Serial.write(buf1, i1);
    i1 = 0;
  }

  if(Serial.available()) {
																						                                  // read the data until pause:
    while(1) {
      if(Serial.available()) {
        buf2[i2] = (char)Serial.read();                              					// read char from UART
        if(i2<bufferSize-1) i2++;
      } else {
        delay(packTimeout);
        if(!Serial.available()) {
          break;
        }
      }
    }
																						                                  // now send to WiFi:
    client.write((char*)buf2, i2);
    i2 = 0;
  }
}
