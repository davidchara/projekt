#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FirebaseESP32.h>

#define WIFI_SSID "IDK"
#define WIFI_PASSWORD "Achtung!!!"
#define FIREBASE_HOST "https://esp32-d6787-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "3qPQuPncCULXfQPNdww3hJGpii4S8osK1833KzCS"

AsyncWebServer server(80);
HardwareSerial espSerial(2);
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Ovládání nápojů</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        button { font-size: 20px; padding: 10px; margin: 10px; cursor: pointer; border: none; }
        .on { background-color: green; color: white; }
        .off { background-color: red; color: white; }
    </style>
</head>
<body>
    <h1>Výběr nápoje</h1>
    <button onclick="selectDrink('Mix1')">Mix1</button>
    <button onclick="selectDrink('Mix2')">Mix2</button>
    <button onclick="selectDrink('Mix3')">Mix 3</button>
    <button onclick="selectDrink('Mix4')">Mix 4</button>

    <h1>Manuální ovládání čerpadel</h1>
    <button id="p1" class="off" onclick="togglePump('1')">Čerpadlo 1</button>
    <button id="p2" class="off" onclick="togglePump('2')">Čerpadlo 2</button>
    <button id="p3" class="off" onclick="togglePump('3')">Čerpadlo 3</button>

    <script>
        function selectDrink(drink) {
            fetch('/mix?drink=' + drink)
                .then(response => response.text())
                .then(data => alert(data))
                .catch(error => console.error('Chyba:', error));
        }

        function togglePump(pump) {
            let button = document.getElementById("p" + pump);
            let state = button.classList.contains("off") ? "on" : "off";
            fetch('/pump?name=' + pump + '&state=' + state)
                .then(response => response.text())
                .then(data => {
                    if (state === "on") {
                        button.classList.remove("off");
                        button.classList.add("on");
                    } else {
                        button.classList.remove("on");
                        button.classList.add("off");
                    }
                })
                .catch(error => console.error('Chyba:', error));
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    espSerial.begin(115200, SERIAL_8N1, 16, 17);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi připojeno!");
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());

    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/pump", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("name") || !request->hasParam("state")) {
        request->send(400, "text/plain", "Chyba: Chybí parametry");
        return;
    }

    String pump = request->getParam("name")->value();
    String state = request->getParam("state")->value();
    int pumpNumber = pump.toInt();
    int pumpState = (state == "on") ? 1 : 0;

    String command = "P|" + String(pumpNumber) + "|" + String(pumpState);
    espSerial.println(command);
    Serial.println("Odesláno Arduinu: " + command);

    request->send(200, "text/plain", "Čerpadlo " + pump + " změněno na " + state);
});


    server.on("/mix", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("drink")) {
            request->send(400, "text/plain", "Chyba: Chybí parametr 'drink'");
            return;
        }

        String drink = request->getParam("drink")->value();
        String path = "/" + drink;
        Serial.println("Načítám mix: " + path);

        if (Firebase.getJSON(firebaseData, path)) {
            Serial.println("Data z Firebase načtena!");
            FirebaseJson &json = firebaseData.jsonObject();
            FirebaseJsonData jsonData;
            
            int p1 = 0, p1_cas = 0;
            int p2 = 0, p2_cas = 0;
            int p3 = 0, p3_cas = 0;

            json.get(jsonData, "p1"); if (jsonData.success) p1 = jsonData.to<int>();
            json.get(jsonData, "p1_cas"); if (jsonData.success) p1_cas = jsonData.to<int>();
            json.get(jsonData, "p2"); if (jsonData.success) p2 = jsonData.to<int>();
            json.get(jsonData, "p2_cas"); if (jsonData.success) p2_cas = jsonData.to<int>();
            json.get(jsonData, "p3"); if (jsonData.success) p3 = jsonData.to<int>();
            json.get(jsonData, "p3_cas"); if (jsonData.success) p3_cas = jsonData.to<int>();

            Serial.printf("Mix: P1=%d (%ds), P2=%d (%ds), P3=%d (%ds)\n", p1, p1_cas, p2, p2_cas, p3, p3_cas);
            
            String command = "M|" + String(p1) + "|" + String(p1_cas) + "|" +
                                       String(p2) + "|" + String(p2_cas) + "|" +
                                       String(p3) + "|" + String(p3_cas);
            espSerial.println(command);
            request->send(200, "text/plain", "Mix " + drink + " spuštěn.");
        } else {
            Serial.println("Firebase chyba: " + firebaseData.errorReason());
            request->send(500, "text/plain", "Chyba načítání mixu z Firebase!\n" + firebaseData.errorReason());
        }
    });

    server.begin();
}

void loop() {
   if (espSerial.available()) {
        String command = espSerial.readStringUntil('\n');
        Serial.println("Přijato: " + command);
    }
}