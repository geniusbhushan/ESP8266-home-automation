// by bhushan patil
// change blynk id Credentials
// 🔹 Blynk Config
#define BLYNK_TEMPLATE_ID "your template id"
#define BLYNK_TEMPLATE_NAME "your template name"
#define BLYNK_AUTH_TOKEN "your authentication"
#define BLYNK_PRINT Serial

// 🔹 Libraries
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <DHT.h>

BlynkTimer timer;

// 🔹 WiFi Credentials
char ssid[] = " wifi name ";
char pass[] = " password ";

// 🔹 GPIO Definitions
#define RELAY_1 D1
#define RELAY_2 D2
#define RELAY_3 D3
#define RELAY_4 D4

#define SWITCH_1 D5
#define SWITCH_2 D6
#define SWITCH_3 D7
#define SWITCH_4 D8

#define DHTPIN   D0
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// 🔹 State Arrays
bool relayState[4] = {LOW, LOW, LOW, LOW};
bool lastSwitchState[4] = {HIGH, HIGH, HIGH, HIGH};

// 🔹 Save/Load Relay State to EEPROM
void saveRelayStates() {
  for (int i = 0; i < 4; i++) EEPROM.write(i, relayState[i]);
  EEPROM.commit();
}
void loadRelayStates() {
  for (int i = 0; i < 4; i++) relayState[i] = EEPROM.read(i);
}

// 🔹 Switch Handling
void checkSwitch(int index, int switchPin, int relayPin, int virtualPin) {
  bool switchState = digitalRead(switchPin);

  // Toggle relay only when switch state changes
  if (switchState != lastSwitchState[index]) {
    lastSwitchState[index] = switchState;

    // Toggle the relay state
    relayState[index] = !relayState[index];
    digitalWrite(relayPin, relayState[index]);

    // Sync with Blynk only if connected
    if (Blynk.connected()) {
      Blynk.virtualWrite(virtualPin, relayState[index]);
    }

    saveRelayStates(); // Save the new state to EEPROM
    delay(200); // debounce delay
  }
}

void checkAllSwitches() {
  checkSwitch(0, SWITCH_1, RELAY_1, V0);
  checkSwitch(1, SWITCH_2, RELAY_2, V1);
  checkSwitch(2, SWITCH_3, RELAY_3, V2);
  checkSwitch(3, SWITCH_4, RELAY_4, V3);
}

// 🔹 Blynk Virtual Write Callbacks
BLYNK_WRITE(V0) { relayState[0] = param.asInt(); digitalWrite(RELAY_1, relayState[0]); saveRelayStates(); }
BLYNK_WRITE(V1) { relayState[1] = param.asInt(); digitalWrite(RELAY_2, relayState[1]); saveRelayStates(); }
BLYNK_WRITE(V2) { relayState[2] = param.asInt(); digitalWrite(RELAY_3, relayState[2]); saveRelayStates(); }
BLYNK_WRITE(V3) { relayState[3] = param.asInt(); digitalWrite(RELAY_4, relayState[3]); saveRelayStates(); }

BLYNK_CONNECTED() {
  Blynk.syncAll();
}

// 🔹 Send DHT11 Data to Blynk
void sendDHTData() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("❌ Failed to read DHT11");
    return;
  }
  Serial.print("🌡 Temp: "); Serial.print(t);
  Serial.print("°C  💧 Humidity: "); Serial.println(h);

  Blynk.virtualWrite(V4, t);  // Temperature
  Blynk.virtualWrite(V5, h);  // Humidity
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  dht.begin();

  // Set pin modes
  pinMode(RELAY_1, OUTPUT); pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT); pinMode(RELAY_4, OUTPUT);
  pinMode(SWITCH_1, INPUT_PULLUP); pinMode(SWITCH_2, INPUT_PULLUP);
  pinMode(SWITCH_3, INPUT_PULLUP); pinMode(SWITCH_4, INPUT_PULLUP);

  // Load relay states from EEPROM
  loadRelayStates();
  digitalWrite(RELAY_1, relayState[0]);
  digitalWrite(RELAY_2, relayState[1]);
  digitalWrite(RELAY_3, relayState[2]);
  digitalWrite(RELAY_4, relayState[3]);

  // Non-blocking WiFi and Blynk setup
  WiFi.begin(ssid, pass);
  Blynk.config(BLYNK_AUTH_TOKEN);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected");
    Blynk.connect(5000);  // try Blynk connect for 5 sec
    if (Blynk.connected()) {
      Serial.println("✅ Blynk Connected");
    } else {
      Serial.println("❌ Blynk NOT Connected");
    }
  } else {
    Serial.println("\n❌ WiFi NOT Connected");
  }

  // Start timers
  timer.setInterval(100L, checkAllSwitches);
  timer.setInterval(5000L, sendDHTData);
}

void loop() {
  Blynk.run();    // works only when connected
  timer.run();    // always runs
}


