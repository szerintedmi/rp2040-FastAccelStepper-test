// FastAccelStepper example: 8 motors, simultaneous start, non-blocking loop
// Board: Raspberry PI Pico RP2040  (Earle Philhower core)
// - Shared SLEEP pin: D16 -> GPIO16 (FastAccelStepper handles enable)
// - All motors: +1000 steps, wait 1s, -1000 steps, wait 1s
// - Moves are queued non-blocking; loop schedules next leg after completion

#include <Arduino.h>
#include <FastAccelStepper.h>

// Shared SLEEP (enable) for all motors
constexpr uint8_t SLEEP_PIN = D16; // GPIO16 HIGH = awake

// STEP & DIR assigments
// constexpr uint8_t NUM_MOTORS = 8;
// static const uint8_t STEP_PINS[NUM_MOTORS] = {D15, D17, D21, D5, D7, D26, D28, D29};
// static const uint8_t DIR_PINS[NUM_MOTORS] = {D14, D18, D20, D4, D6, D27, D12, D13};
constexpr uint8_t NUM_MOTORS = 3;
static const uint8_t STEP_PINS[NUM_MOTORS] = {D15, D17, D21};
static const uint8_t DIR_PINS[NUM_MOTORS] = {D14, D18, D20};
// constexpr uint8_t NUM_MOTORS = 2;
// static const uint8_t STEP_PINS[NUM_MOTORS] = {D15, D17};
// static const uint8_t DIR_PINS[NUM_MOTORS] = {D14, D18};

// Background task tuning

// keep ~60ms of steps queued. NB: this is needed to avoid jitter at 4000Hz speed even with 2 motors
constexpr uint8_t FORWARD_PLAN_WINDOW_MS = 60;
// run queue maintenance every 1ms.
// NB: Reducing task rate doesn't make much difference for 3 or more motors, still jittering at 4000Hz speed
constexpr uint8_t ENGINE_TASK_RATE_MS = 1;

// Motion parameters
constexpr uint32_t SPEED_HZ = 4000;  // steps/s
constexpr uint32_t ACCEL = 16000;    // steps/s^2
constexpr int32_t MOVE_STEPS = 1000; // steps per leg
constexpr uint32_t PAUSE_MS = 1000;  // pause between legs

FastAccelStepperEngine engine;
FastAccelStepper *steppers[NUM_MOTORS] = {nullptr};

static inline bool anyRunning()
{
  for (auto *s : steppers)
  {
    if (s && s->isRunning())
      return true;
  }

  return false;
}

// Start moving all motors (non-blocking)
static void startAllMoves(int32_t steps)
{
  for (uint8_t i = 0; i < NUM_MOTORS; i++)
  {
    if (steppers[i])
      steppers[i]->move(steps);
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(300);
  }

  engine.init();
  engine.task_rate(ENGINE_TASK_RATE_MS);

  // Create and configure all steppers
  for (uint8_t i = 0; i < NUM_MOTORS; i++)
  {
    steppers[i] = engine.stepperConnectToPin(STEP_PINS[i]);
    if (!steppers[i])
    {
      Serial.printf("Failed to connect stepper to motor #%u (STEP pin D%u DIR pin: D%u)\n", i, (int)STEP_PINS[i], (int)DIR_PINS[i]);
      continue; // skip this motor and try remaining ones
    }

    Serial.printf("Connected stepper #%u (STEP pin D%u DIR pin: D%u)\n", i, (int)STEP_PINS[i], (int)DIR_PINS[i]);

    steppers[i]->setDirectionPin(DIR_PINS[i], /*dirHighCountsUp=*/true, /*dir_change_delay_us=*/200);
    steppers[i]->setEnablePin(SLEEP_PIN, /*low_active_enables_stepper=*/false);
    steppers[i]->setAutoEnable(true);
    steppers[i]->setForwardPlanningTimeInMs(FORWARD_PLAN_WINDOW_MS);

    steppers[i]->setSpeedInHz(SPEED_HZ);
    steppers[i]->setAcceleration(ACCEL);
    steppers[i]->setCurrentPosition(0);
  }
}

void loop()
{
  // Non-blocking scheduler: start forward leg, then let loop manage timing
  static bool isPaused = true;
  static unsigned long pauseUntilMs = 0;
  static int8_t dir = 1;

  if (isPaused && (long)(millis() - pauseUntilMs) >= 0 && !anyRunning())
  {
    isPaused = false;
    startAllMoves(MOVE_STEPS * dir);
    dir = -dir;
    Serial.printf("Starting to move %u steps to direction %i\n", MOVE_STEPS, dir);
  }
  else if (!isPaused && !anyRunning())
  {
    isPaused = true;
    pauseUntilMs = millis() + PAUSE_MS;
    Serial.printf("All motors stopped, pausing for %u ms\n", PAUSE_MS);
  }

  delay(1); // small yield to avoid busy loop
}
