#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char *ssid = "esp32wifi";
bool ledOn = true;

WebServer server(80);
DNSServer dnsServer;

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

const String mainHtmlOutput = "<!DOCTYPE html><html><body><h1>Temperatur</h1><div style=\"font-size:xx-large;\" id=\"result\"></div><br><div><input type=\"button\" onclick=\"location.href='http://192.168.0.1/led';\" value=\"LED ON/OFF\"/></div><script>if(typeof(EventSource) !== \"undefined\") {var source = new EventSource(\"temp\");source.onmessage = function(event) {document.getElementById(\"result\").innerHTML = event.data + \"<br>\";};} else {document.getElementById(\"result\").innerHTML = \"Sorry, your browser does not support server-sent events...\";}</script></body></html>";


void setup()
{ 
  // Setup IO pins 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(oneWireBus, INPUT);
  delay(1000);

  // Debug serial port
  Serial.begin(115200);
  Serial.println();

  // Start the DS18B20 sensor
  sensors.begin();

  // Setup Wifi as AP
  setupAP();

  // Webserver callbacks
  server.on("/", handleMainPage);
  server.on("/temp", handleTemperaturePage);
  server.on("/led", handleLed);
  server.begin();
  
  Serial.println("Server started");
}


void loop()
{
  dnsServer.processNextRequest();
  server.handleClient();  
}


void handleMainPage() {
  Serial.println("handleMainPage");
  server.send(200, "text/html", mainHtmlOutput); 
}


void handleTemperaturePage() {
  Serial.println("handleTemperaturePage");

  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.println(temperatureC);

  String htmlParsed = "data: " + String(temperatureC) + "\n\n";
  server.sendHeader("Content-Type", "text/event-stream");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/event-stream", htmlParsed);
}


void handleLed() {
  Serial.println("handleLed");
  if (ledOn) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOn = false;
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
    ledOn = true;
  }
  
  server.send(200, "text/html", mainHtmlOutput); 
}


void setupAP()
{
  const byte DNS_PORT = 53;

  IPAddress local_IP(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  delay(100);  // Wait for AP to start before setting IP
  WiFi.softAPConfig(local_IP, local_IP, subnet);

  dnsServer.start(DNS_PORT, "*", local_IP);

  IPAddress myIP = WiFi.softAPIP();
  Serial.println("AP ssid:" + String(ssid));
  Serial.println("AP IP address: " + myIP.toString());
}
