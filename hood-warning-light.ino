// Warning light for kitchen hood not turned on while cooktop is heating

// Wiring of Arduino Pro Mini:
//
//                     +----------+
//                     |      Raw ø----- Hood DC+
//                     |      GND ø----- Hood GND
//   LED output - -----ø GND      |
//   LED output + -----ø 2    Vcc ø 
//                     |          |
// Cooktop switch -----ø 7*       |
// Cooktop switch -----ø 8**      |
//    Hood signal -----ø 9**      |
//                     +----------+
//
//  * Mount 10kOhm pull-down resistor (to GND) on pin 7
// ** Mount 10kOhm pull-up resistors (to Vcc) on pins 8 and 9.
//
// LED output:     LED needs resistor.
// Hood signal:    Active low, external pull-up resistor.
// Cooktop switch: May be stable or closed for a few milliseconds at least once per 3 sec.

#define INPUT_READING_INTERVAL_MSEC           50
#define HOOD_INPUT_CHANGE_STABLE_MSEC       1000
#define COOKTOP_INPUT_ACTIVE_STABLE_MSEC      50
#define COOKTOP_INPUT_INACTIVE_STABLE_MSEC  5000
#define LED_BLINK_HALF_PERIOD_MSEC           250

#define EXT_LED_OUTPUT_PIN   2
#define COOKTOP_OUTPUT_PIN   7
#define COOKTOP_INPUT_PIN    8
#define HOOD_INPUT_PIN       9
#define INT_LED_OUTPUT_PIN  13

#define COOKTOP_ACTIVE_LEVEL  LOW
#define HOOD_ACTIVE_LEVEL     LOW

#define SERIAL_BAUD_RATE  9600

// Startup assuming idle state with stable inactive inputs
unsigned long hoodChangeMillis = 0;
unsigned long cooktopChangeMillis = 0;
unsigned long ledToggleMillis = 0;
int cooktopInputState = (COOKTOP_ACTIVE_LEVEL == LOW) ? HIGH : LOW;
int hoodInputState    = (HOOD_ACTIVE_LEVEL == LOW) ? HIGH : LOW;

void setup()
{
  pinMode(INT_LED_OUTPUT_PIN, OUTPUT);
  pinMode(EXT_LED_OUTPUT_PIN, OUTPUT);
  pinMode(COOKTOP_OUTPUT_PIN, OUTPUT);
  pinMode(COOKTOP_INPUT_PIN, INPUT);
  pinMode(HOOD_INPUT_PIN, INPUT);

  digitalWrite(INT_LED_OUTPUT_PIN, LOW);
  digitalWrite(EXT_LED_OUTPUT_PIN, LOW);
  digitalWrite(COOKTOP_OUTPUT_PIN, COOKTOP_ACTIVE_LEVEL);

  Serial.begin(SERIAL_BAUD_RATE);

  Serial.println("Hood Warning Light");
  Serial.println("==================");
}

void loop()
{
  checkInputs();
  updateLED();
  delay(INPUT_READING_INTERVAL_MSEC);
}

void checkInputs()
{
  checkInput("Cooktop", COOKTOP_INPUT_PIN, COOKTOP_ACTIVE_LEVEL, &cooktopInputState, &cooktopChangeMillis, COOKTOP_INPUT_ACTIVE_STABLE_MSEC, COOKTOP_INPUT_INACTIVE_STABLE_MSEC);
  checkInput("Hood",    HOOD_INPUT_PIN,    HOOD_ACTIVE_LEVEL,    &hoodInputState,    &hoodChangeMillis,    HOOD_INPUT_CHANGE_STABLE_MSEC,    HOOD_INPUT_CHANGE_STABLE_MSEC);
}

void checkInput(String name, int pinNo, int signalActiveLevel, int* pSignalState, unsigned long* pSignalChangeMillis, unsigned long activeStableMillis, unsigned long inactiveStableMillis)
{
  int signalLevel = digitalRead(pinNo);
  unsigned long millisNow = millis();

  bool goingActive = (signalLevel == signalActiveLevel) && (signalLevel != *pSignalState);
  bool goingInactive = (signalLevel != signalActiveLevel) && (signalLevel != *pSignalState);

  // Not currently waiting for a stable signal change?
  if (*pSignalChangeMillis == 0)
  {
    // Signal about to change?
    if (signalLevel != *pSignalState)
    {
      // Remember time of the change
      *pSignalChangeMillis = millisNow;

      // Reserved value?
      if (*pSignalChangeMillis == 0)
        *pSignalChangeMillis = 1;

      if (goingActive || goingInactive)
      {
        Serial.print(name);
        Serial.print(" going ");
        Serial.println(goingActive ? "active?" : "inactive?");
      }
    }
  }
  else // Waiting for a stable signal change
  {
    // State change stable for a certain time?
    if ((goingActive   && (millisNow - *pSignalChangeMillis) >= activeStableMillis) ||
        (goingInactive && (millisNow - *pSignalChangeMillis) >= inactiveStableMillis))
    {
      // State change done
      *pSignalState = signalLevel;
      *pSignalChangeMillis = 0;

      Serial.print(name);
      Serial.print(" now ");
      Serial.println(goingActive ? "active." : "inactive.");
    }
    else // No stable state change (yet)
    {
      // No state change anyway?
      if (!goingActive && !goingInactive)
      {
        // Forget it for now
        *pSignalChangeMillis = 0;
      }
    }
  }
}

void updateLED()
{
  // Should warn that cooktop is on but hood not ventilating?
  if ((cooktopInputState == COOKTOP_ACTIVE_LEVEL) && (hoodInputState != HOOD_ACTIVE_LEVEL))
  {
    unsigned long millisNow = millis();

    // Time to toggle LED state now?
    if ((millisNow - ledToggleMillis) >= LED_BLINK_HALF_PERIOD_MSEC)
    {
      // Toggle LED state to make it blink
      digitalWrite(EXT_LED_OUTPUT_PIN, (digitalRead(EXT_LED_OUTPUT_PIN) == LOW) ? HIGH : LOW);
      ledToggleMillis = millisNow;
    }
  }
  else // Should not warn
  {
    digitalWrite(EXT_LED_OUTPUT_PIN, LOW);
  }

  bool isChangingInputState = (hoodChangeMillis != 0) || (cooktopChangeMillis != 0);

  // Turn on internal LED when changing input state - otherwise same as external LED
  if (isChangingInputState)
    digitalWrite(INT_LED_OUTPUT_PIN, HIGH);
  else
    digitalWrite(INT_LED_OUTPUT_PIN, digitalRead(EXT_LED_OUTPUT_PIN));
}
