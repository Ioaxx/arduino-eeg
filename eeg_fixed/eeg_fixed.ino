#include <SPI.h>
#include <SD.h>

const int analogPin = A0;
const int buzzerPin = 8;
const int chipSelect = 4; // SD module CS pin
unsigned long lastPrintTime = 0;  // FIXED: initialized to 0
unsigned long startTime;
bool calibrating = true;

int maxVal = 0;       // max value during calibration
int minVal = 1023;    // min value during calibration
int baseline = 0;     // center of EEG signal
float thresholdFactor = 0.26; // fraction of amplitude to set threshold above baseline
int threshold = 0;

int keyPressCount = 0; // counter to toggle manual messages
int manualEyeState = 0; // FIXED: track manual eye state (0=open, 1=closed)

File logfile; // SD file object

void setup() {
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("Recording EEG...");
  Serial.println("Press any key to toggle eyes closed/opened.");
  Serial.println("Keep eyes OPEN for first 15 seconds (calibration).");
  startTime = millis();

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD initialization failed!");
    while (1);
  }

  logfile = SD.open("EEG_data.csv", FILE_WRITE);
  if (!logfile) {
    Serial.println("Error opening file!");
    while (1);
  }

  // FIXED: Write CSV header with 3 columns
  logfile.println("EEG_value,manual_eyes_closed,auto_eyes_closed");
  logfile.flush();
}

void loop() {
  int val = analogRead(analogPin);

  // ---- Check for key press to toggle manual eye state ----
  if (Serial.available() > 0) {
    char c = Serial.read(); // read the key
    keyPressCount++;        // increment counter

    if (keyPressCount % 2 == 1) {   // odd press
      Serial.println("Eyes closed (manual)");
      manualEyeState = 1; // FIXED: store state
    } else {                        // even press
      Serial.println("Eyes open (manual)");
      manualEyeState = 0; // FIXED: store state
    }
  }

  // ---- Print EEG value and save to SD every 25 ms ----
  if (millis() - lastPrintTime >= 15) {
    lastPrintTime = millis();
    Serial.print("EEG value: ");
    Serial.println(val);

    if (!calibrating) {
      // Automatic state based on threshold
      int autoEyeState = (val > threshold) ? 1 : 0;

      // Buzzer control
      if (autoEyeState == 1) {
        tone(buzzerPin, 1000);
      } else {
        noTone(buzzerPin);
      }

      // FIXED: Save EEG + manual state + auto state to SD
      if (logfile) {
        logfile.print(val);
        logfile.print(",");
        logfile.print(manualEyeState);  // manual marking
        logfile.print(",");
        logfile.println(autoEyeState);   // automatic detection
        logfile.flush();
      }
    } else {
      // FIXED: During calibration, still log data (auto_eyes_closed = 0)
      if (logfile) {
        logfile.print(val);
        logfile.print(",");
        logfile.print(manualEyeState);  // manual state
        logfile.print(",");
        logfile.println(0);              // auto state not active yet
        logfile.flush();
      }
    }
  }

  // ---- 10s calibration phase ----
  if (calibrating) {
    if (val > maxVal) maxVal = val;
    if (val < minVal) minVal = val;

    if (millis() - startTime >= 15000) { // after 10 seconds
      calibrating = false;
      baseline = (maxVal + minVal) / 2;
      threshold = baseline + (maxVal - minVal) * thresholdFactor;
      Serial.print("Calibration done. baseline=");
      Serial.print(baseline);
      Serial.print(" threshold=");
      Serial.println(threshold);
    }
  }
}
