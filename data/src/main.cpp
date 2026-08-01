#include <Arduino.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <Wire.h>
#include <ESPmDNS.h>
#include "movement.h"
#include "runner.h"
#include "pen.h"
#include "display.h"
#include "config.h"   // OMNISKETCH: pin map
#include "phases/phasemanager.h"

AsyncWebServer server(80);

Movement *movement;
Runner *runner;
Pen *pen;       // OMNISKETCH: pen 1 (D33), not mirrored
Pen *pen2;      // OMNISKETCH: pen 2 (D32), MIRRORED
Display *display;

PhaseManager* phaseManager;

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    phaseManager->getCurrentPhase()->handleUpload(request, filename, index, data, len, final);
}

void handleGetState(AsyncWebServerRequest *request) {
    phaseManager->respondWithState(request);
}

std::vector<const char *> menu = {"wifi", "sep"};
void setup()
{
    delay(10);
    Serial.begin(9600);

    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    // OMNISKETCH (Step 4): peripheral pin setup. Both devices start OFF.
    pinMode(LASER_PIN, OUTPUT);
    digitalWrite(LASER_PIN, LASER_ACTIVE_HIGH ? LOW : HIGH);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_HIGH ? LOW : HIGH);

    // OMNISKETCH (Step 4 v4): wake-up sweep using LEDC channel 15 directly.
    //
    // Previously this used Arduino's tone()/noTone(), but tone() and
    // ESP32Servo both grab LEDC channels starting from channel 0, and
    // when they collide the servo PWM is corrupted - which we observed
    // as the pens failing to go down during drawing.
    //
    // Using ledcAttachPin/ledcWriteTone with explicit channel 15 puts
    // the buzzer on a hardware timer that can't physically conflict
    // with the servos on channels 0 and 1.
    //
    // The sweep itself mirrors Penta's standalone test sketch:
    // 800 -> 2000 Hz in 200 Hz steps, 50 ms per step (~350 ms total).
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    for (int freq = 800; freq <= 2000; freq += 200) {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, freq);
        delay(50);
    }
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);   // mute
    ledcDetachPin(BUZZER_PIN);
    // Restore the pin to its idle/off state via plain GPIO.
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_HIGH ? LOW : HIGH);

    // OMNISKETCH (Step 4 fix): explicit I2C init at 400 kHz for faster OLED.
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(400000);

    display = new Display();
    Serial.println("Initialized display");

    // initialize movement right away or the motors can start creeping due to floating output
    movement = new Movement(display);
    Serial.println("Initialized steppers");

    bool resetAfterConnect = false;
    std::function<void()> serverCallback = [&] () {
        resetAfterConnect = true;
    };
    
    WiFiManager wifiManager;
    
    wifiManager.setConnectTimeout(20);
    wifiManager.setTitle("Connect to WiFi");
    wifiManager.setMenu(menu);
    wifiManager.setWebServerCallback(serverCallback);
    wifiManager.autoConnect("OmniSketch");

    if (resetAfterConnect) {
        Serial.println("Connected to WiFi through captive portal, restarting...");
        ESP.restart();
    }
    
    Serial.println("Connected to wifi");

    MDNS.begin("OmniSketch");

    Serial.println("Started mDNS for OmniSketch");

    pen  = new Pen(SERVO_PEN1_PIN);
    pen2 = new Pen(SERVO_PEN2_PIN, true);
    Serial.println("Initialized both servos");

    runner = new Runner(movement, pen, pen2, display);
    Serial.println("Initialized runner");

    server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html").setCacheControl("no-cache");

    server.on("/command", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->handleCommand(request); });

    server.on("/setTopDistance", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->setTopDistance(request); });

    server.on("/extendToHome", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->extendToHome(request); });

    server.on("/setServo", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->setServo(request); });

    server.on("/setPenDistance", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->setPenDistance(request); });

    server.on("/estepsCalibration", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->estepsCalibration(request); });

    server.on("/doneWithPhase", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->doneWithPhase(request); });

    server.on("/run", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->run(request); });

    server.on("/resume", HTTP_POST, [](AsyncWebServerRequest *request)
              { phaseManager->getCurrentPhase()->resumeTopDistance(request); });

    server.on("/getState", HTTP_GET, [](AsyncWebServerRequest *request)
              { handleGetState(request); });

    server.on("/parkPen2", HTTP_POST, [](AsyncWebServerRequest *request) {
        pen2->setRawValue(0);
        request->send(200, "text/plain", "OK");
    });

    server.on("/previewBoundary", HTTP_POST, [](AsyncWebServerRequest *request) {
        runner->runBoundaryPreview();
        request->send(200, "text/plain", "OK");
    });

    server.on(
        "/uploadCommands", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            handleGetState(request);
        }, 
        handleUpload
    );

    server.on(
        "/downloadCommands", HTTP_GET,
        [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/commands", "text/plain");
        }
    );

    server.onNotFound(notFound);

    Serial.println("Finished setting up the server");

    phaseManager = new PhaseManager(movement, pen, pen2, runner, &server);

    server.begin();
    Serial.println("Server started");

    display->displayHomeScreen("http://" + WiFi.localIP().toString(), "or", "http://OmniSketch.local");
    
}

void loop()
{
    movement->runSteppers();
    runner->run();
    phaseManager->getCurrentPhase()->loopPhase();
}

