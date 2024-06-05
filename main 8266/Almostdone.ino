#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

#define TX_PIN 5 // D2
#define RX_PIN 4 // D1
SoftwareSerial mySerial(RX_PIN, TX_PIN);

char buffer[14];
int count = 0;

const char *ssid = "POCO X3 Pro";
const char *password = "0916827292";
const char *mqtt_server = "172.16.32.135";
const int mqtt_port = 1883;
const char *mqtt_id = "esp8266";
const char *topic_subscribe = "to-esp8266";
const char *topic_publish = "from-esp8266";

WiFiClient client;
PubSubClient mqtt_client(client);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Recived data from: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (size_t i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("------------------------------------------");
}

void setup()
{
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.print(ssid);
  Serial.println();
  WiFi.begin(ssid, password);
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected to Wifi ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.print(WiFi.localIP());
  Serial.println();
  delay(1000);

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);

  Serial.println("Connecting to mqtt... ");
  while (!mqtt_client.connect(mqtt_id))
  {
    delay(500);
  }
  Serial.println("Connected to mqtt ");
   mqtt_client.subscribe("to-esp8266");
   mqtt_client.publish("from-esp8266", "Waiting");
}



void loop()
{
  if (mySerial.available()) {
      char data = mySerial.read();
      if(data == 'A'){
      Serial.println("90 3E 1D 26");
      mqtt_client.publish("from-esp8266", " 90 3E 1D 26");}
      if(data == 'B'){
      Serial.println("93 AA B6 0F");
      mqtt_client.publish("from-esp8266", " 93 AA B6 0F");}
  
  else mqtt_client.publish("from-esp8266", " UNDEFINE ");}
  mqtt_client.loop();
  }
  


