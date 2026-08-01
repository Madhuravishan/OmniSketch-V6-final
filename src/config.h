#ifndef Config_h
#define Config_h

// =====================================================================
//  OmniSketch Wall Plotter - Central Hardware Configuration
//  Custom PCB pin map (OmniSketch V1.0)
// =====================================================================

// ---------- Left Stepper  (DRV8825 - U4) ----------
constexpr int LEFT_STEP_PIN    = 18;
constexpr int LEFT_DIR_PIN     = 14;
constexpr int LEFT_ENABLE_PIN  = 13;

// ---------- Right Stepper (DRV8825 - U3) ----------
constexpr int RIGHT_STEP_PIN   = 27;
constexpr int RIGHT_DIR_PIN    = 26;
constexpr int RIGHT_ENABLE_PIN = 25;

// ---------- Dual Pen Servos (MG90S, 5V from buck converter) ----------
constexpr int SERVO_PEN1_PIN   = 33;
constexpr int SERVO_PEN2_PIN   = 32;

// ---------- I2C OLED Status Screen (SSD1306, 128x64, 0x3C) ----------
constexpr int OLED_SDA_PIN     = 21;
constexpr int OLED_SCL_PIN     = 22;

// ---------- Peripherals (OmniSketch only) ----------
constexpr int BUZZER_PIN       = 4;
constexpr int LASER_PIN        = 16;

// =====================================================================
//  OMNISKETCH (Step 4): peripheral driver polarity.
//
//  LASER (MD0186 5V dot diode):
//    Driven via an IRLZ44N logic-level N-MOSFET (low-side) with a 10k
//    pull-down on the gate. ACTIVE HIGH.
//
//  BUZZER (active buzzer module with built-in 8550 transistor):
//    Idle/off state is HIGH on this specific module (confirmed by
//    Penta's standalone buzzer test sketch). The polarity flag only
//    matters for the "off" level between beeps - the beep itself
//    drives the pin with PWM (see BUZZER_LEDC_CHANNEL below).
// =====================================================================
constexpr bool LASER_ACTIVE_HIGH  = true;
constexpr bool BUZZER_ACTIVE_HIGH = false;

// =====================================================================
//  OMNISKETCH (Step 4 v4): LEDC channel pinning for the buzzer.
//
//  THE BUG WE'RE FIXING:
//  Arduino's tone()/noTone() functions and the ESP32Servo library both
//  use the ESP32's LEDC peripheral for PWM. Both allocate channels
//  dynamically starting from channel 0. ESP32Servo grabs channels 0
//  and 1 for pen1/pen2; tone() tends to grab channel 0 as well. When
//  tone() runs (start beep / end beep / tool-change beep), it corrupts
//  the PWM signal pen 1's servo depends on, and after noTone() the
//  servo can't recover until a reboot. Symptoms: pens stop going down
//  during drawing, occasional pen-1 failures during calibration.
//
//  THE FIX:
//  Use LEDC directly on channel 15 - the highest channel, in the
//  "low-speed" group (channels 8-15) which has a SEPARATE hardware
//  timer from the "high-speed" group (channels 0-7) that ESP32Servo
//  uses. Physical separation means no possible interference.
//
//  Channel 15 also acts as documentation: anyone reading the code can
//  see at a glance that the buzzer is on its own dedicated channel
//  and won't fight with the servo allocator.
// =====================================================================
constexpr int BUZZER_LEDC_CHANNEL = 15;

// OMNISKETCH (Step 4): buzzer beep durations for each event.
//   wake-up     -> sweep 800-2000 Hz ........... "I'm alive"
//   start       -> one short beep .............. "going"
//   tool change -> two short beeps ............. "swap happening"
//   end         -> one long beep ............... "done"
constexpr int BUZZER_WAKE_BEEP_MS        = 300;
constexpr int BUZZER_START_BEEP_MS       = 150;
constexpr int BUZZER_TOOL_CHANGE_BEEP_MS = 150;
constexpr int BUZZER_TOOL_CHANGE_GAP_MS  = 100;
constexpr int BUZZER_END_BEEP_MS         = 600;

// =====================================================================
//  Motor driver microstepping
//  OmniSketch PCB: M0/M1/M2 hardwired HIGH -> 1/32 microstepping.
// =====================================================================
constexpr int STEPS_PER_ROTATION = 200 * 32;

// =====================================================================
//  OMNISKETCH (Step 3c): per-pen kinematic offsets (d_p values).
//  v2 (locked): pen 2 sits ~32mm above pen 1 (above the belt line).
// =====================================================================
constexpr double PEN1_D_P_MM =   4.4866;
constexpr double PEN2_D_P_MM = -24.5134;

#endif

