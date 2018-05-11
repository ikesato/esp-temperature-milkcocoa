#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <Milkcocoa.h>
#include <DHT.h>

#define LED_PIN 4
#define BUTTON_PIN 14
#define DHT_PIN 5
#define PUSH_INTERVAL (60 * 1000)
#define ENABLE_SLEEP 0

DHT dht(DHT_PIN, DHT11);
Ticker ticker;
unsigned long lastTime = 0;

// milkcocoa
#define MILKCOCOA_APP_ID      "icejgooxblm"
#define MILKCOCOA_SERVERPORT  1883
#define MILKCOCOA_DATASTORE   "esp8266"
const char MQTT_SERVER[] PROGMEM    = MILKCOCOA_APP_ID ".mlkcca.com";
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ MILKCOCOA_APP_ID;
WiFiClient client;
Milkcocoa milkcocoa = Milkcocoa(&client, MQTT_SERVER, MILKCOCOA_SERVERPORT, MILKCOCOA_APP_ID, MQTT_CLIENTID);


void deepSleep() {
  Serial.println("zzz...");
  ESP.deepSleep(60 * 1000 * 1000, WAKE_RF_DEFAULT);
}


void pushData() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    milkcocoa.loop();
    DataElement elem = DataElement();
    elem.setValue("temperature", t);
    elem.setValue("humidity", h);
    bool ret = milkcocoa.push(MILKCOCOA_DATASTORE, &elem);

    Serial.print("Pushed to milkcocoa! ret=");
    Serial.println(ret);
}


void blink() {
    static int state = 1;
    digitalWrite(LED_PIN, state);
    state = !state;
}

void setup() {
    Serial.begin(115200);

    dht.begin();
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    // setup WiFi
    WiFiManager wifiManager;
    Serial.println(digitalRead(BUTTON_PIN));
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("reset settings!!!");
        wifiManager.resetSettings();
        Ticker t;
        t.attach_ms(100, blink);
        delay(3000);
        ESP.restart();
        return;
    }
    ticker.attach_ms(500, blink);
    wifiManager.autoConnect("ESP-TEMPERATURE");
    Serial.println("connected...yeey :)");
    ticker.detach();
    digitalWrite(LED_PIN, 1);
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(100);

        float h = dht.readHumidity();
        float t = dht.readTemperature();
        if (isnan(h) || isnan(t)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            Serial.print("Humidity: ");
            Serial.print(h);
            Serial.print("%\tTemperature: ");
            Serial.print(t);
            Serial.println("C");
        }
        return;
    }

#if ENABLE_SLEEP == 1
    pushData();
    deepSleep();
#else
    unsigned long now = millis();
    if (now - lastTime >= PUSH_INTERVAL) {
        pushData();
        lastTime = now;
    }
#endif
}
