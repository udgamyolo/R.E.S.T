#include <M5Core2.h>
#include "MAX30100_PulseOximeter.h"
#include <Wire.h>

#define REPORTING_PERIOD_MS 1000
#define MEASUREMENT_PERIOD 60000




// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;



int readings{0};
float STRateSum{0};
int STReadings{0};

TouchPoint_t touch;

struct AwakeRecord {
  uint32_t awakeBegin;
  uint32_t awakeEnd;
};

struct LightSleepRecord {
  uint32_t lightSleepBegin;
  uint32_t lightSleepEnd;
};

struct DeepSleepRecord {
  uint32_t deepSleepBegin;
  uint32_t deepSleepEnd;
};

struct REMSleepRecord {
  uint32_t remSleepBegin;
  uint32_t remSleepEnd;
};

bool SleepTracking = false;

double CurrentHeartRate;

bool Awake = false;

int LightSleepCount{ 0 }, AwakeCount{ 0 }, DeepSleepCount{ 0 }, REMSleepCount{ 0 };

double LightSleepLower, LightSleepUpper;
bool LightSleep = false;

double REMSleepLower, REMSleepUpper;
bool REMSleep = false;

double DeepSleepLower, DeepSleepUpper;
bool DeepSleep = false;

bool AwakeCheck = false;

double MovementInitial;
double MovementChange;

uint32_t AwakeBegin, AwakeEnd;
uint32_t LightSleepBegin, LightSleepEnd;
uint32_t DeepSleepBegin, DeepSleepEnd;
uint32_t REMSleepBegin, REMSleepEnd;

uint32_t AwakeTime, LightSleepTime, DeepSleepTime, REMSleepTime;

AwakeRecord* awakeRecords;
LightSleepRecord* lightSleepRecords;
DeepSleepRecord* deepSleepRecords;
REMSleepRecord* remSleepRecords;

uint32_t start = 0;
uint32_t tsLastReport = 0;

uint32_t sleep_start = 0;
uint32_t sleep_end = 0;

double HeartRateSum = 0;
double RestingHeartRate = 0;

bool menu = true;
bool recording_RestingHeartRate = false;
bool showing_RestingHeartRate = false;
bool SleepMenu = false;
bool sleeping = false;
bool displayResults = false;

int displayPage{ 1 };

// Callback (registered below) fired when a pulse is detected
void onBeatDetected() {
  Serial.println("Beat!");
}


void setup() {
  M5.begin();
  Serial.begin(115200);

  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
}





void loop() {
  // Make sure to call update as fast as possible
  pox.update();


  // Menu Loop
  if (menu) {
    touch = M5.Touch.getPressPoint();
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Welcome to R.E.S.T!", 30, 20);
    M5.Lcd.drawRoundRect(80, 80, 160, 60, 4, TFT_GREEN);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.drawString("Start", 130, 105);

    if (touch.x > 80 && touch.x < 240 && touch.y > 80 && touch.y < 140) {

      M5.Lcd.fillRoundRect(80, 80, 160, 60, 4, TFT_GREEN);
      M5.Lcd.setTextColor(TFT_BLACK);
      M5.Lcd.drawString("Start", 130, 105);
      // delay(500);
      M5.Lcd.drawRoundRect(80, 80, 160, 60, 4, TFT_WHITE);
      M5.Lcd.fillRoundRect(80, 80, 160, 60, 4, TFT_BLACK);
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.drawString("Start", 130, 105);

      M5.Lcd.fillScreen(TFT_BLACK);

      start = millis();
      recording_RestingHeartRate = true;
      menu = false;
      HeartRateSum = 0;
      readings = 0;
    }
  }

  // Record heart rate for 10 seconds after button press
  if ((recording_RestingHeartRate)) {
    if ((millis() - start < MEASUREMENT_PERIOD)) {
      if (millis() - tsLastReport > REPORTING_PERIOD_MS) {


        M5.Lcd.setCursor(0, 20);
        M5.Lcd.print("Heart rate:");

        M5.Lcd.drawRoundRect(60, 50, 200, 140, 4, TFT_WHITE);
        M5.Lcd.fillRect(90, 100, 140, 40, TFT_BLACK);
        M5.Lcd.setCursor(90, 100);
        M5.Lcd.setTextSize(4);
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.print(pox.getHeartRate());

        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_WHITE);


        tsLastReport = millis();
        if (pox.getHeartRate() > 0){
          HeartRateSum += pox.getHeartRate();
          readings++;
        }
      }
    } else {
      M5.Lcd.fillScreen(TFT_BLACK);
      showing_RestingHeartRate = true;
      recording_RestingHeartRate = false;
    }
  }

  // Average Hear Rate Display Loop
  if ((showing_RestingHeartRate)) {
    touch = M5.Touch.getPressPoint();

    double avgHeartRate = HeartRateSum / readings;
    RestingHeartRate = avgHeartRate;
    M5.Lcd.setCursor(1, 20);
    M5.Lcd.println("Resting heart rate:");
    M5.Lcd.print(avgHeartRate);
    M5.Lcd.print("bpm");

    M5.Lcd.drawRoundRect(40, 120, 110, 60, 4, TFT_WHITE);
    M5.Lcd.drawRoundRect(170, 120, 110, 60, 4, TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Measure", 47, 130);
    M5.Lcd.drawString("Again", 49, 150);
    M5.Lcd.drawString("Continue", 180, 145);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE);

    if (touch.x > 40 && touch.x < 150 && touch.y > 120 && touch.y < 180) {
      M5.Lcd.fillRoundRect(40, 120, 110, 60, 4, TFT_GREEN);
      M5.Lcd.setTextColor(TFT_BLACK);
      M5.Lcd.drawString("Measure", 47, 130);
      M5.Lcd.drawString("Again", 49, 150);
      M5.Lcd.drawRoundRect(40, 120, 110, 60, 4, TFT_WHITE);
      M5.Lcd.fillRoundRect(40, 120, 110, 60, 4, TFT_BLACK);

      showing_RestingHeartRate = false;
      recording_RestingHeartRate = true;
      start = millis();
      HeartRateSum = 0;

      M5.Lcd.fillScreen(TFT_BLACK);
    }

    if (touch.x > 170 && touch.x < 280 && touch.y > 120 && touch.y < 180) {
      M5.Lcd.fillRoundRect(170, 120, 110, 60, 4, TFT_RED);
      M5.Lcd.setTextColor(TFT_BLACK);
      M5.Lcd.drawString("Continue", 180, 145);
      M5.Lcd.drawRoundRect(170, 120, 110, 60, 4, TFT_WHITE);
      M5.Lcd.fillRoundRect(170, 120, 110, 60, 4, TFT_BLACK);

      LightSleepUpper = RestingHeartRate;
      LightSleepLower = 0.8 * RestingHeartRate;
      REMSleepLower = 1.1 * RestingHeartRate;

      showing_RestingHeartRate = false;
      SleepMenu = true;

      M5.Lcd.fillScreen(TFT_BLACK);
    }
  }

  if ((SleepMenu)) {
    touch = M5.Touch.getPressPoint();
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(1, 20);
    M5.Lcd.println("Let's track your sleep cycle...");
    M5.Lcd.drawRoundRect(80, 80, 160, 60, 4, TFT_GREEN);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.drawString("Start", 130, 105);
    if (touch.x > 80 && touch.x < 240 && touch.y > 80 && touch.y < 140) {

      M5.Lcd.fillRoundRect(80, 80, 160, 60, 4, TFT_GREEN);
      M5.Lcd.setTextColor(TFT_BLACK);
      M5.Lcd.drawString("Start", 130, 105);
      // delay(500);
      M5.Lcd.drawRoundRect(80, 80, 160, 60, 4, TFT_WHITE);
      M5.Lcd.fillRoundRect(80, 80, 160, 60, 4, TFT_BLACK);
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.drawString("Start", 130, 105);
      SleepMenu = false;
      SleepTracking = true;
      Awake = true;
      sleep_start = millis();

      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE);

      M5.Lcd.fillScreen(TFT_BLACK);
    }
  }

  if (SleepTracking) {

    M5.Lcd.drawRoundRect(120, 200, 80, 30, 4, TFT_RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.drawString("Stop", 138, 207);

    touch = M5.Touch.getPressPoint();

    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

      M5.Lcd.setCursor(0, 20);
      M5.Lcd.print("Tracking your sleep: ");

      M5.Lcd.drawRoundRect(60, 50, 200, 140, 4, TFT_WHITE);
      M5.Lcd.fillRect(90, 100, 140, 40, TFT_BLACK);
      M5.Lcd.setCursor(90, 100);
      M5.Lcd.setTextSize(4);
      M5.Lcd.setTextColor(TFT_GREEN);
      M5.Lcd.print(pox.getHeartRate());

      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE);
      tsLastReport = millis();
      if (pox.getHeartRate() > 0){
        STRateSum += pox.getHeartRate();
        STReadings++;
      }
      if (STReadings >= 5){
        CurrentHeartRate = STRateSum/STReadings;
      }
    }

    if (Awake) {
      if (touch.x > 120 && touch.x < 200 && touch.y > 200 && touch.y < 230) {

        AwakeEnd = millis();
        sleep_end = millis();


        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_BLACK);
        M5.Lcd.drawString("Stop", 138, 207);
        M5.Lcd.drawRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_BLACK);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.drawString("Stop", 138, 207);

        displayResults = true;
        displayPage = 1;

        SleepTracking = false;

        M5.Lcd.setCursor(1, 20);
        M5.Lcd.setTextSize(2);
        
        AwakeEnd = millis();
        AwakeTime += (AwakeEnd - AwakeBegin);

        M5.Lcd.fillScreen(TFT_BLACK);
        
        // awakeRecords[0].awakeBegin = AwakeBegin;
        // awakeRecords[1].awakeEnd = AwakeEnd;
        // M5.Lcd.print("Values have been added");
        AwakeCount++;
      }

      if ((CurrentHeartRate <= LightSleepUpper) && (CurrentHeartRate > LightSleepLower)) {
        AwakeEnd = millis();
        AwakeTime += (AwakeEnd - AwakeBegin);
        // awakeRecords[0].awakeBegin = AwakeBegin;
        // awakeRecords[1].awakeEnd = AwakeEnd;
        AwakeCount++;

        Awake = false;
        LightSleep = true;
        LightSleepBegin = millis();
      }
    }

    if (LightSleep) {
      if (CurrentHeartRate <= LightSleepLower) {
        LightSleepEnd = millis();
        LightSleepTime += (LightSleepEnd - LightSleepBegin);
        // lightSleepRecords[LightSleepCount].lightSleepBegin = LightSleepBegin;
        // lightSleepRecords[LightSleepCount].lightSleepEnd = LightSleepEnd;
        LightSleepCount++;
        LightSleep = false;
        DeepSleep = true;
        DeepSleepBegin = millis();
      }

      else if ((CurrentHeartRate >= REMSleepLower)) {
        LightSleepEnd = millis();
        LightSleepTime += (LightSleepEnd - LightSleepBegin);
        // lightSleepRecords[LightSleepCount].lightSleepBegin = LightSleepBegin;
        // lightSleepRecords[LightSleepCount].lightSleepEnd = LightSleepEnd;
        LightSleepCount++;
        LightSleep = false;
        REMSleep = true;
        REMSleepBegin = millis();
      }

      if (touch.x > 120 && touch.x < 200 && touch.y > 200 && touch.y < 230) {

        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_BLACK);
        M5.Lcd.drawString("Stop", 138, 207);
        M5.Lcd.drawRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_BLACK);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.drawString("Stop", 138, 207);

        LightSleepEnd = millis();
        sleep_end = millis();
        LightSleepTime += (LightSleepEnd - LightSleepBegin);
        // lightSleepRecords[LightSleepCount].lightSleepBegin = LightSleepBegin;
        // lightSleepRecords[LightSleepCount].lightSleepEnd = LightSleepEnd;
        LightSleepCount++;

        M5.Lcd.fillScreen(TFT_BLACK);



        displayResults = true;
        displayPage = 1;

        SleepTracking = false;
        LightSleep = false;
      }
    }

    if (DeepSleep) {
      if ((CurrentHeartRate <= LightSleepUpper) && (CurrentHeartRate >= LightSleepLower)) {
        DeepSleepEnd = millis();
        DeepSleepTime += (DeepSleepEnd - DeepSleepBegin);
        // deepSleepRecords[DeepSleepCount].deepSleepBegin = DeepSleepBegin;
        // deepSleepRecords[DeepSleepCount].deepSleepEnd = DeepSleepEnd;
        DeepSleepCount++;
        DeepSleep = false;
        LightSleep = true;
        LightSleepBegin = millis();
      }

      if ((CurrentHeartRate >= REMSleepLower)) {
        DeepSleepEnd = millis();
        DeepSleepTime += (DeepSleepEnd - DeepSleepBegin);
        // deepSleepRecords[DeepSleepCount].deepSleepBegin = DeepSleepBegin;
        // deepSleepRecords[DeepSleepCount].deepSleepEnd = DeepSleepEnd;
        DeepSleepCount++;
        DeepSleep = false;
        REMSleep = true;
        REMSleepBegin = millis();
      }

      if (touch.x > 120 && touch.x < 200 && touch.y > 200 && touch.y < 230) {

        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_BLACK);
        M5.Lcd.drawString("Stop", 138, 207);
        M5.Lcd.drawRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_BLACK);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.drawString("Stop", 138, 207);

        DeepSleepEnd = millis();
        sleep_end = millis();
        DeepSleepTime += (DeepSleepEnd - DeepSleepBegin);
        // deepSleepRecords[DeepSleepCount].deepSleepBegin = DeepSleepBegin;
        // deepSleepRecords[DeepSleepCount].deepSleepEnd = DeepSleepEnd;
        DeepSleepCount++;

        displayResults = true;
        displayPage = 1;

        DeepSleep = false;
        SleepTracking = false;

        M5.Lcd.fillScreen(TFT_BLACK);
      }
    }

    if (REMSleep) {
      if ((CurrentHeartRate <= LightSleepUpper) && (CurrentHeartRate >= LightSleepLower)) {
        REMSleepEnd = millis();
        REMSleepTime += (REMSleepEnd - REMSleepBegin);
        // remSleepRecords[REMSleepCount].remSleepBegin = REMSleepBegin;
        // remSleepRecords[REMSleepCount].remSleepEnd = REMSleepEnd;
        REMSleepCount++;
        REMSleep = false;
        LightSleep = true;
        LightSleepBegin = millis();
      }

      else if ((CurrentHeartRate < LightSleepLower)) {
        REMSleepEnd = millis();
        REMSleepTime += (REMSleepEnd - REMSleepBegin);
        // remSleepRecords[REMSleepCount].remSleepBegin = REMSleepBegin;
        // remSleepRecords[REMSleepCount].remSleepEnd = REMSleepEnd;
        REMSleepCount++;
        REMSleep = false;
        DeepSleep = true;
        DeepSleepBegin = millis();
      }

      if (touch.x > 120 && touch.x < 200 && touch.y > 200 && touch.y < 230) {

        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_BLACK);
        M5.Lcd.drawString("Stop", 138, 207);
        M5.Lcd.drawRoundRect(120, 200, 80, 30, 4, TFT_RED);
        M5.Lcd.fillRoundRect(120, 200, 80, 30, 4, TFT_BLACK);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.drawString("Stop", 138, 207);


        REMSleepEnd = millis();
        sleep_end = millis();
        REMSleepTime += (REMSleepEnd - REMSleepBegin);
        // remSleepRecords[REMSleepCount].remSleepBegin = REMSleepBegin;
        // remSleepRecords[REMSleepCount].remSleepEnd = REMSleepEnd;
        REMSleepCount++;

        displayResults = true;
        displayPage = 1;

        REMSleep = false;
        SleepTracking = false;
        M5.Lcd.fillScreen(TFT_BLACK);
      }
    }
  }

  if (displayResults == true) {
    M5.Lcd.fillTriangle(300, 210, 300, 230, 320, 220, TFT_WHITE);

    touch = M5.Touch.getPressPoint();

    if (touch.x > 300 && touch.x < 320 && touch.y > 210 && touch.y < 230) {
      displayPage++;
      M5.Lcd.fillScreen(TFT_BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE);
    }

    if (displayPage == 1) {
      M5.Lcd.fillScreen(TFT_BLACK);
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setTextSize(3);
      M5.Lcd.drawString("Sleepers", 90, 85);
      M5.Lcd.drawString("Summary", 95, 115);
    }

    if (displayPage == 2) {
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(1, 20);
      M5.Lcd.println("Awake Time (in hours): ");
      M5.Lcd.println((float)((AwakeTime/1000)/3600));
      M5.Lcd.println("No of times awoken: ");
      M5.Lcd.print(AwakeCount);
       M5.Lcd.println("Percentage of Sleep: ");
      M5.Lcd.print((float)(AwakeTime/ (sleep_end - sleep_start)));
      // M5.Lcd.println("Awake Periods: ");
      // for (int i = 0; i < AwakeCount; i++) {
      //   M5.Lcd.println(awakeRecords[i].awakeBegin);
      //   M5.Lcd.print(" - ");
      //   M5.Lcd.print(awakeRecords[i].awakeEnd);
      // }
    }

     if (displayPage == 3) {
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(1, 20);
      M5.Lcd.println("Light Sleep Time (in hours): ");
      M5.Lcd.println((float)((LightSleepTime/1000)/3600));
      M5.Lcd.println("No of times reached Light Sleep: ");
      M5.Lcd.println(LightSleepCount);
       M5.Lcd.println("Percentage of Sleep: ");
      M5.Lcd.println((float)(LightSleepTime/ (sleep_end - sleep_start)));
      // M5.Lcd.println("LightSleep Periods: ");
      // for (int i = 0; i < LightSleepCount; i++) {
      //   M5.Lcd.println(LightSleepRecords[i].LightSleepBegin);
      //   M5.Lcd.print(" - ");
      //   M5.Lcd.print(LightSleepRecords[i].LightSleepEnd);
      // }
    }

     if (displayPage == 4) {
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(1, 20);
      M5.Lcd.println("Deep Sleep Time (in hours): ");
      M5.Lcd.println((float)((DeepSleepTime/1000)/3600));
      M5.Lcd.println("No of times reached to Deep Sleep: ");
      M5.Lcd.println(DeepSleepCount);
       M5.Lcd.println("Percentage of Sleep: ");
      M5.Lcd.println((float)(DeepSleepTime/ (sleep_end - sleep_start)));
      // M5.Lcd.println("DeepSleep Periods: ");
      // for (int i = 0; i < DeepSleepCount; i++) {
      //   M5.Lcd.println(DeepSleepRecords[i].DeepSleepBegin);
      //   M5.Lcd.print(" - ");
      //   M5.Lcd.print(DeepSleepRecords[i].DeepSleepEnd);
      // }
    }

     if (displayPage == 5) {
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(1, 20);
      M5.Lcd.println("REM Sleep Time (in hours): ");
      M5.Lcd.println((float)((REMSleepTime/1000)/3600));
      M5.Lcd.println("No of times reached REM Sleep ");
      M5.Lcd.println(REMSleepCount);
      M5.Lcd.println("Percentage of Sleep: ");
      M5.Lcd.println((float)(REMSleepTime/ (sleep_end - sleep_start)));
      // M5.Lcd.println("REMSleep Periods: ");
      // for (int i = 0; i < REMSleepCount; i++) {
      //   M5.Lcd.println(REMSleepRecords[i].REMSleepBegin);
      //   M5.Lcd.print(" - ");
      //   M5.Lcd.print(REMSleepRecords[i].REMSleepEnd);
      // }
    }

    if (displayPage > 5){
      displayResults = false;
      menu = true;
    }



  }
}
