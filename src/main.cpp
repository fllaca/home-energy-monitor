#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>

#include <PZEM004Tv30.h>

/* Use software serial for the PZEM
 * Pin 11 Rx (Connects to the Tx pin on the PZEM)
 * Pin 12 Tx (Connects to the Rx pin on the PZEM)
*/
PZEM004Tv30 pzem(4, 5); // D2, D1 (nodemcu)

/*
  AC_USE_SPIFFS indicates SPIFFS or LittleFS as available file systems that
  will become the AUTOCONNECT_USE_SPIFFS identifier and is exported as showing
  the valid file system. After including AutoConnect.h, the Sketch can determine
  whether to use FS.h or LittleFS.h by AUTOCONNECT_USE_SPIFFS definition.
*/
#include <FS.h>
#ifdef AUTOCONNECT_USE_SPIFFS
FS& FlashFS = SPIFFS;
#else
#include <LittleFS.h>
FS& FlashFS = LittleFS;
#endif



#define HELLO_URI   "/hello"

// Declare AutoConnectText with only a value.
// Qualify the Caption by reading style attributes from the style.json file.
ACText(Caption, "Hello, world");
ACRadio(Styles, {}, "");
ACSubmit(Apply, "Apply", HELLO_URI);

//AutoConnectAux for the custom Web page.
AutoConnectAux helloPage(HELLO_URI, "Hello", true, { Caption, Styles, Apply });
AutoConnectConfig config;
AutoConnect portal;

// JSON document loading buffer
String ElementJson;

unsigned long lastTime;

// Load the element from specified file in the flash on board.
void loadParam(const char* fileName) {
  Serial.printf("Style %s ", fileName);
  File param = FlashFS.open(fileName, "r");
  if (param) {
    ElementJson = param.readString();
    param.close();
    Serial.printf("loaded:\n%s", ElementJson.c_str());
  }
  else
    Serial.println("open failed");
}

// Redirects from root to the hello page.
void onRoot() {
  ESP8266WebServer& webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String(HELLO_URI));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void onPzem() {
  DynamicJsonDocument result(1024);
  String resultString;

  float voltage = pzem.voltage();
  if( !isnan(voltage) ){
    result["voltage"] = voltage;
  }
  float current = pzem.current();
  if( !isnan(current) ){
    result["current"] = current;
  }
  float power = pzem.power();
  if( !isnan(power) ){
    result["power"] = power;
  }
  float energy = pzem.energy();
  if( !isnan(energy) ){
    result["energy"] = energy;
  }
  float frequency = pzem.frequency();
  if( !isnan(frequency) ){
    result["frequency"] = frequency;
  }
  float pf = pzem.pf();
  if( !isnan(pf) ){
    result["pf"] = pf;
  }

  serializeJson(result, resultString);

  ESP8266WebServer& webServer = portal.host();
  webServer.send(200, "application/json", resultString);
  webServer.client().flush();
  webServer.client().stop();

}

// Load the attribute of the element to modify at runtime from external.
String onHello(AutoConnectAux& aux, PageArgument& args) {
  // Select the style parameter file and load it into the text element.
  AutoConnectRadio& styles = helloPage["Styles"].as<AutoConnectRadio>();
  String  styleName = styles.value();
  if (styleName.length())
    loadParam(styleName.c_str());

  // List parameter files stored on the flash.
  // Those files need to be uploaded to the filesystem in advance.
  styles.empty();
  Dir dir = FlashFS.openDir("/");
  while (dir.next()) {
    if (!dir.isDirectory())
      styles.add(dir.fileName());
  }

  // Apply picked style
  helloPage.loadElement(ElementJson);
  return String();
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("starting");
  pinMode(LED_BUILTIN, OUTPUT);
  lastTime = millis();
  FlashFS.begin();
  helloPage.on(onHello);      // Register the attribute overwrite handler.
  portal.join(helloPage);     // Join the hello page.
  config.ota = AC_OTA_BUILTIN;
  portal.config(config);
  portal.begin();

  ESP8266WebServer& webServer = portal.host();
  webServer.on("/", onRoot);  // Register the root page redirector.
  webServer.on("/pzem.json", onPzem);  // Register the root page redirector.
}

void loop() {
  portal.handleClient();
  //Serial.println("echo" + portal.host().client().localIP().toString());
  unsigned long currentTime = millis();
  if (currentTime > lastTime + 1000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));   // Turn the LED on by making the voltage LOW
    lastTime = currentTime;
  }
}
