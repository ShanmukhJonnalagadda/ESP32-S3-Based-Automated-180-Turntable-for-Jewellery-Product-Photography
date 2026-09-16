/// ============================================================
// AUTOMATED 180° JEWELLERY TURNTABLE
//
// Controller : Arduino Nano ESP32
// Motor      : 28BYJ-48 5V
// Driver     : ULN2003
// Speed      : 10k potentiometer
// Control    : Momentary push button
//
// Motion:
// 180° CLOCKWISE
//      ↓
// Pause
//      ↓
// 180° ANTI-CLOCKWISE
//      ↓
// Return to starting position
//      ↓
// Repeat
// ============================================================

// ------------------------------------------------------------
// MOTOR CONNECTIONS
// ------------------------------------------------------------

const int IN1 = D2;
const int IN2 = D3;
const int IN3 = D4;
const int IN4 = D5;

// ------------------------------------------------------------
// USER CONTROLS
// ------------------------------------------------------------

const int POT_PIN = A0;
const int BUTTON_PIN = D6;

// ------------------------------------------------------------
// MOTOR CONFIGURATION
// ------------------------------------------------------------

// Your calibrated motor:
// 4096 half-steps = 360°
//
// Therefore:
// 2048 half-steps = 180°

const int STEPS_PER_SWEEP = 2048;


// Half-step sequence for 28BYJ-48

const byte stepSequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};


const int motorPins[4] = {IN1,IN2,IN3, IN4};


// ------------------------------------------------------------
// MOTOR STATE
// ------------------------------------------------------------

bool motorRunning = false;

// true  = clockwise
// false = anti-clockwise

bool clockwise = true;
int currentStep = 0;

// Counts steps during each 180° sweep

int sweepStepCount = 0;

// ------------------------------------------------------------
// TIMING
// ------------------------------------------------------------

unsigned long lastStepTime = 0;


// Pause between CW and CCW movement

unsigned long directionPauseStart = 0;


// 500 ms = 0.5 second pause

const unsigned long DIRECTION_PAUSE = 500;


bool directionPause = false;

// ------------------------------------------------------------
// BUTTON DEBOUNCE
// ------------------------------------------------------------

bool lastButtonReading = HIGH;

bool stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;

const unsigned long DEBOUNCE_DELAY = 50;

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  // Motor outputs

  for (int i = 0; i < 4; i++) {

    pinMode(motorPins[i], OUTPUT);

  }

  // Potentiometer

  pinMode(POT_PIN, INPUT);

  // Push button:
  // D6 ↔ Button ↔ GND

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Make sure motor is OFF when powered on

  releaseMotor();

  Serial.println();

  Serial.println("================================");
  Serial.println("MANMAY 180 DEGREE TURNTABLE");
  Serial.println("================================");

  Serial.println();

  Serial.println("Motor: STOPPED");

  Serial.println("Press button to START");

}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // READ PUSH BUTTON
  // ----------------------------------------------------------

  handleButton();

  // ----------------------------------------------------------
  // READ POTENTIOMETER
  // ----------------------------------------------------------

  int potValue = analogRead(POT_PIN);

  /*
     Nano ESP32 ADC:

     Pot minimum ≈ 0
     Pot maximum ≈ 4095


     Current speed range:

     Pot low  → larger delay → slower
     Pot high → smaller delay → faster
  */

  unsigned long stepDelay =
      map(
        potValue,
        0,
        4095,
        4000,
        1000
      );

  // ----------------------------------------------------------
  // MOTOR CONTROL
  // ----------------------------------------------------------

  if (motorRunning) {

    // --------------------------------------------------------
    // PAUSE AFTER EACH 180° SWEEP
    // --------------------------------------------------------

    if (directionPause) {


      if (
        millis() - directionPauseStart
        >= DIRECTION_PAUSE
      ) {

        directionPause = false;

        Serial.println();

        if (clockwise) {

          Serial.println(
            "Starting 180° CLOCKWISE"
          );

        }

        else {

          Serial.println(
            "Starting 180° ANTI-CLOCKWISE"
          );

        }

      }

    }

    // --------------------------------------------------------
    // MOVE MOTOR
    // --------------------------------------------------------

    else if (
      micros() - lastStepTime
      >= stepDelay
    ) {

      lastStepTime = micros();

      // ------------------------------------------------------
      // CLOCKWISE
      // ------------------------------------------------------

      if (clockwise) {

        currentStep++;

        if (currentStep >= 8) {

          currentStep = 0;

        }

      }

      // ------------------------------------------------------
      // ANTI-CLOCKWISE
      // ------------------------------------------------------

      else {

        currentStep--;

        if (currentStep < 0) {

          currentStep = 7;

        }

      }

      // Energize motor coils

      performStep(currentStep);

      // Count one half-step

      sweepStepCount++;

      // ------------------------------------------------------
      // CHECK FOR COMPLETE 180° SWEEP
      // ------------------------------------------------------

      if (
        sweepStepCount
        >= STEPS_PER_SWEEP
      ) {


        // Reset counter for next sweep

        sweepStepCount = 0;

        // Turn motor coils off during pause

        releaseMotor();

        // Tell Serial Monitor which sweep completed

        if (clockwise) {

          Serial.println();

          Serial.println(
            "180° CLOCKWISE COMPLETE"
          );

        }

        else {

          Serial.println();

          Serial.println(
            "180° ANTI-CLOCKWISE COMPLETE"
          );

          Serial.println(
            "RETURNED TO START POSITION"
          );

        }

        // Reverse direction

        clockwise = !clockwise;

        // Start pause

        directionPause = true;

        directionPauseStart = millis();

      }

    }

  }

  // ----------------------------------------------------------
  // SERIAL MONITOR INFORMATION
  // ----------------------------------------------------------

  static unsigned long lastPrintTime = 0;

  if (
    millis() - lastPrintTime
    >= 1000
  ) {


    Serial.print("Motor: ");


    if (motorRunning) {

      Serial.print("RUNNING");

    }

    else {

      Serial.print("STOPPED");

    }


    Serial.print(" | Direction: ");


    if (clockwise) {

      Serial.print("CW");

    }

    else {

      Serial.print("CCW");

    }

    Serial.print(" | Pot: ");

    Serial.print(potValue);

    Serial.print(" | Step: ");

    Serial.print(sweepStepCount);

    Serial.print("/");

    Serial.println(STEPS_PER_SWEEP);

    lastPrintTime = millis();

  }

}

// ============================================================
// PUSH BUTTON FUNCTION
// ============================================================

void handleButton() {


  bool reading =
      digitalRead(BUTTON_PIN);

  // Detect change in button state

  if (
    reading
    != lastButtonReading
  ) {

    lastDebounceTime = millis();

  }

  // Wait for stable button reading

  if (
    millis() - lastDebounceTime
    > DEBOUNCE_DELAY
  ) {

    if (
      reading
      != stableButtonState
    ) {


      stableButtonState = reading;

      // Button is pressed

      if (
        stableButtonState
        == LOW
      ) {

        // Toggle motor state

        motorRunning =
            !motorRunning;

        // --------------------------------
        // START
        // --------------------------------

        if (motorRunning) {

          Serial.println();

          Serial.println(
            "***** MOTOR STARTED *****"
          );

          if (clockwise) {

            Serial.println(
              "Direction: CLOCKWISE"
            );

          }

          else {

            Serial.println(
              "Direction: ANTI-CLOCKWISE"
            );

          }

        }


        // --------------------------------
        // STOP
        // --------------------------------

        else {


          Serial.println();

          Serial.println(
            "***** MOTOR STOPPED *****"
          );

          releaseMotor();

        }

      }

    }

  }

  lastButtonReading = reading;

}

// ============================================================
// MOTOR STEP FUNCTION
// ============================================================

void performStep(
  int stepNumber
) {

  for (
    int i = 0;
    i < 4;
    i++
  ) {


    digitalWrite(
      motorPins[i],
      stepSequence[stepNumber][i]
    );

  }

}

// ============================================================
// RELEASE MOTOR
// ============================================================

void releaseMotor() {

  for (
    int i = 0;
    i < 4;
    i++
  ) {

    digitalWrite(
      motorPins[i],
      LOW
    );

  }

}