#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>


#define SSID "HomeWiFi"
#define PASS "azsxdcfv"
#define DHTPIN 2
#define DHTTYPE DHT11
#define HTTP_PORT 80
#define TCP_PORT 8080
#define SMS_SEND_PERIOD 1000*60*5 // 5 min
#define SMS_RETRY_PERIOD 1000*10 // 10 sec
#define SMS_URL "http://sms.ru/sms/send?api_id=E157D178-503A-437C-DA54-A93332A0B657&to=79113228054&json=1&msg="
#define TEMPRATURE_ALERT 35
#define HUMIDITY_ALERT 65


DHT dht(DHTPIN, DHTTYPE);
WiFiServer httpServer(HTTP_PORT);
WiFiServer tcpServer(TCP_PORT);

int temperature = 0;
int humidity = 0;
bool alertStatus = false;
unsigned long nextSMSSendTime = 0;


void setup() {
  Serial.begin(115200);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(SSID);

  delay(50);
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  delay(250);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  delay(50);
  httpServer.begin();
  Serial.println("HTTP server started");

  //delay(50);
  //tcpServer.begin();
  //Serial.println("TCP server started");

  delay(50);
  dht.begin();
  Serial.println("DHT started");

  delay(500);
}

void updateSensorValues() {
  bool updated = false;
  while(!updated){
      float temperatureFloat = dht.readTemperature();
      float humidityFloat = dht.readHumidity();
      if (!isnan(temperatureFloat) && !isnan(humidityFloat)) {
        temperature = static_cast<int>(temperatureFloat);
        humidity = static_cast<int>(humidityFloat);
        updated = true;
      }else{
        //Serial.println("Failed to read from DHT sensor!");
        delay(1);
      }
  }

  if(temperature > 200){
    temperature = -1;
  }
  if(humidity > 200){
    humidity = -1;
  }
}

void updateAlertStatus() {
  alertStatus = (temperature > TEMPRATURE_ALERT) || (humidity > HUMIDITY_ALERT);
}

void sendAlertSMS() {
  unsigned long now = millis();
  if (alertStatus && (now > nextSMSSendTime)) {
    String message = "ESP->";
    message += "temperature:";
    message += temperature;
    message += ",humidity:";
    message += humidity;

    String requestUrl = SMS_URL;
    requestUrl += message;
    Serial.print("Send alert SMS on: ");
    Serial.println(requestUrl);

    HTTPClient httpClient;
    httpClient.begin(requestUrl);
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) {
      nextSMSSendTime = now + SMS_SEND_PERIOD;
      Serial.printf("Response code: %d, ok\n", httpCode);
    } else {
      nextSMSSendTime = now + SMS_RETRY_PERIOD;
      Serial.printf("Response code: %d, retry\n", httpCode);
    }
  }
}

String getJson() {
  String text;
  text += "{\"temperature\":";
  text += temperature;
  text += ", \"humidity\":";
  text += humidity;
  text += ", \"alert\":";
  text += alertStatus ? "true" : "false";
  text += "}";
  return text;
}

void handleHTTPClient() {
  if (httpServer.hasClient() == false) {
    return;
  }

  WiFiClient client = httpServer.available();
  if (!client) {
    return;
  }

  Serial.println("New HTTP client");

  String text = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  text +=
    "<html>"
    "<head>"
    "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
    "  <title>ESP8268 status page</title>"
    "  <script>"
    "      function timedRefresh(timeoutPeriod) {"
    "          setTimeout(\"location.reload(true);\",timeoutPeriod);"
    "      }"
    "  </script>"
    "</head>"
    "<body onload=\"JavaScript:timedRefresh(1000);\">";
  text += getJson();
  text += "</body></html>";

  client.print(text);
  client.flush();
  //client.stop();

  Serial.println("Client HTTP disconnected.");
}

/*void handleTCPClient() {
  if (tcpServer.hasClient() == false) {
    return;
  }

  WiFiClient client = tcpServer.available();
  if (!client) {
    return;
  }

  Serial.println("New tcp client");

  client.print(getJson());
  client.flush();
  client.stop();

  Serial.println("Client tcp disconnected.");
  }*/

void loop() {
  delay(10);

  updateSensorValues();
  updateAlertStatus();
  sendAlertSMS();
  handleHTTPClient();
  //handleTCPClient();
}
