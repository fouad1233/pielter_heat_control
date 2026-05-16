#include <Arduino.h>
#include <ArduinoJson.h>
#include <PID_v1.h>

#define PIN_LPWM 11 // Changed to D11 (supports hardware PWM)
#define PIN_RPWM 3
#define PIN_L_EN 2
#define PIN_LED_RED 6
#define PIN_LED_BLUE 7
#define PIN_NTC A7

// NTC Parameters
const float SERIES_RESISTOR = 1000.0;
const float NOMINAL_RESISTANCE = 1000.0;
const float NOMINAL_TEMPERATURE = 25.0;
const float BCOEFFICIENT = 3435.0;

// State Variables
bool autoMode = false;
double setpoint = 25.0;
double currentTemp = 25.0;
double pidOutput = 0.0;
int manualPwm = 0; // Ranges strictly from -255 to +255 

// PID object mapping to PID logic (direct implies higher temp -> positive action, we must be careful with polarity)
// Note: DIRECT means if current temp < setpoint, output generates a POSITIVE error causing positive PWM (Heating).
// If current temp > setpoint, output goes NEGATIVE (Cooling).
PID myPID(&currentTemp, &pidOutput, &setpoint, 5.0, 0.5, 1.0, DIRECT); 

unsigned long lastUpdate = 0;
unsigned long lastButtonCheck = 0;

float readTemperature() {
  int adcValue = analogRead(PIN_NTC);
  if (adcValue == 0) return -99.0;
  
  // For a pulldown resistor block connected to NTC:
  // V_out = VCC * R_pd / (R_ntc + R_pd)
  // Simplifies to R_ntc = R_pd * (1023 / ADC - 1)
  
  float rNtc = SERIES_RESISTOR * (1023.0 / adcValue - 1.0);
  
  float steinhart;
  steinhart = rNtc / NOMINAL_RESISTANCE;             // (R/Ro)
  steinhart = log(steinhart);                        // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                       // Invert
  steinhart -= 273.15;                               // Convert to °C
  
  return steinhart;
}

void setPeltier(int pwmValue) {
  // Constrain internally to Arduino 8-bit limits
  pwmValue = constrain(pwmValue, -255, 255);
  digitalWrite(PIN_L_EN, HIGH);
  
  if (pwmValue > 0) {
    // Warm Mode
    analogWrite(PIN_LPWM, pwmValue);
    analogWrite(PIN_RPWM, 0);
    digitalWrite(PIN_LED_RED, HIGH);
    digitalWrite(PIN_LED_BLUE, LOW);
  } else if (pwmValue < 0) {
    // Cold Mode
    analogWrite(PIN_LPWM, 0);
    analogWrite(PIN_RPWM, -pwmValue);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_BLUE, HIGH);
  } else {
    // Idle 
    analogWrite(PIN_LPWM, 0);
    analogWrite(PIN_RPWM, 0);
    digitalWrite(PIN_L_EN, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_BLUE, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_LPWM, OUTPUT);
  pinMode(PIN_RPWM, OUTPUT);
  pinMode(PIN_L_EN, OUTPUT);
  
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
  
  setPeltier(0);
  
  // Setup PID
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(-255, 255);
}

void loop() {
  unsigned long now = millis();
  
  // Handle Buttons (Disabled for now)
  /*
  if (now - lastButtonCheck > 50) {
    lastButtonCheck = now;
    if (digitalRead(PIN_BTN_RED) == LOW) {
      autoMode = false;
      manualPwm = 255; // Max Warm
    }
    else if (digitalRead(PIN_BTN_BLUE) == LOW) {
      autoMode = false;
      manualPwm = -255; // Max Cold
    }
  }
  */
  
  // Main telemetry loop & PID processing
  if (now - lastUpdate > 500) {
    lastUpdate = now;
    currentTemp = readTemperature();
    
    if (autoMode) {
      myPID.Compute();
      setPeltier((int)pidOutput);
    } else {
      setPeltier(manualPwm);
    }
    
    // Broadcast state for Web UI
    StaticJsonDocument<200> doc;
    doc["temp"] = currentTemp;
    doc["mode"] = autoMode ? "auto" : "manual";
    
    // Scale the PWM output to the user's requested visual scale (-1024 to 1024)
    int outPwm = autoMode ? (int)pidOutput : manualPwm;
    long mappedPwm = map((long)outPwm, -255, 255, -1024, 1024);
    doc["pwm"] = mappedPwm;
    doc["setpoint"] = setpoint;
    
    serializeJson(doc, Serial);
    Serial.println();
  }
  
  // Listen for Web UI settings
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, input);
    
    if (!error) {
      if (doc.containsKey("mode")) {
        autoMode = String(doc["mode"].as<const char*>()) == "auto";
      }
      if (doc.containsKey("pwm") && !autoMode) {
        // UI provides scale in -1024 to 1024, we map it back
        long uiPwm = doc["pwm"].as<long>();
        manualPwm = map(constrain(uiPwm, -1024, 1024), -1024, 1024, -255, 255);
      }
      if (doc.containsKey("setpoint")) {
        setpoint = doc["setpoint"].as<double>();
      }
    }
  }
}