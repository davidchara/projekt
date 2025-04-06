#define PUMP_1 5
#define PUMP_2 6
#define PUMP_3 7

void setup() {
    Serial.begin(115200);
    pinMode(PUMP_1, OUTPUT);
    pinMode(PUMP_2, OUTPUT);
    pinMode(PUMP_3, OUTPUT);
    digitalWrite(PUMP_1, LOW);
    digitalWrite(PUMP_2, LOW);
    digitalWrite(PUMP_3, LOW);
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\\n');
        Serial.println("Přijatý příkaz: " + command);

        if (command.startsWith("P|")) {
            int pump, state;
            sscanf(command.c_str(), "P|%d|%d", &pump, &state);
            
            int pin = (pump == 1) ? PUMP_1 : (pump == 2) ? PUMP_2 : PUMP_3;
            digitalWrite(pin, state ? HIGH : LOW);
        }

        if (command.startsWith("M|")) {

            int p1, p1_cas, p2, p2_cas, p3, p3_cas;
            sscanf(command.c_str(), "M|%d|%d|%d|%d|%d|%d", &p1, &p1_cas, &p2, &p2_cas, &p3, &p3_cas);

            if (p1) {
                delay(1000);
                digitalWrite(PUMP_1, HIGH);
                delay(p1_cas * 1000);
                digitalWrite(PUMP_1, LOW);
                delay(1000);
            }
            if (p2) {
                delay(1000);
                digitalWrite(PUMP_2, HIGH);
                delay(p2_cas * 1000);
                digitalWrite(PUMP_2, LOW);
                 delay(1000);
            }
            if (p3) {
                delay(1000);
                digitalWrite(PUMP_3, HIGH);
                delay(p3_cas * 1000);
                digitalWrite(PUMP_3, LOW);
                delay(1000);
            }
        }
    }
}
