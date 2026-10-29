
#include <Arduino.h>

// ValleyFlow SensorNode - Firmware v0.1
// Board: Seeed Studio XIAO ESP32C3
// Framework: Arduino via PlatformIO

namespace Pins {
    constexpr uint8_t TRIG = D0;
    constexpr uint8_t ECHO = D1;
    constexpr uint8_t FLOAT_SWITCH = D2;
}

namespace Config {
    constexpr uint32_t SERIAL_BAUD = 115200;
    constexpr uint32_t SAMPLE_INTERVAL_MS = 2000;

    // JSN-SR04T variants have different minimum ranges.
    // Confirm the specifications of your exact sensor.
    constexpr float MIN_DISTANCE_CM = 25.0f;
    constexpr float MAX_DISTANCE_CM = 450.0f;

    // Echo timeout accommodates a typical 4.5 m measurement.
    constexpr uint32_t ECHO_TIMEOUT_US = 30000;

    // Assumes the float switch closes to GND when wet.
    // Change to false if your switch behaves oppositely.
    constexpr bool FLOAT_CLOSED_WHEN_WET = true;
}

uint32_t lastSampleTime = 0;
uint32_t sampleNumber = 0;

struct SensorReading {
    float distanceCm;
    bool valid;
};

SensorReading readUltrasonic() {
    digitalWrite(Pins::TRIG, LOW);
    delayMicroseconds(3);

    digitalWrite(Pins::TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(Pins::TRIG, LOW);

    const uint32_t durationUs = pulseIn(
        Pins::ECHO,
        HIGH,
        Config::ECHO_TIMEOUT_US
    );

    if (durationUs == 0) {
        return {0.0f, false};
    }

    // Distance = round-trip time * speed of sound / 2.
    const float distanceCm =
        (durationUs * 0.0343f) / 2.0f;

    if (distanceCm < Config::MIN_DISTANCE_CM ||
        distanceCm > Config::MAX_DISTANCE_CM) {
        return {distanceCm, false};
    }

    return {distanceCm, true};
}

bool readFloatSwitchWet() {
    const bool closed =
        digitalRead(Pins::FLOAT_SWITCH) == LOW;

    return Config::FLOAT_CLOSED_WHEN_WET
        ? closed
        : !closed;
}

void printTelemetry(
    const SensorReading& reading,
    bool floatWet
) {
    Serial.print("{\"device_id\":\"VF-NODE-01\"");
    Serial.print(",\"sample\":");
    Serial.print(++sampleNumber);

    Serial.print(",\"distance_cm\":");

    if (reading.valid) {
        Serial.print(reading.distanceCm, 2);
    } else {
        Serial.print("null");
    }

    Serial.print(",\"sensor_ok\":");
    Serial.print(reading.valid ? "true" : "false");

    Serial.print(",\"float_switch_wet\":");
    Serial.print(floatWet ? "true" : "false");

    Serial.println("}");
}

void setup() {
    Serial.begin(Config::SERIAL_BAUD);

    pinMode(Pins::TRIG, OUTPUT);
    digitalWrite(Pins::TRIG, LOW);

    pinMode(Pins::ECHO, INPUT);

    // Suitable for a simple, dry-contact float switch.
    pinMode(Pins::FLOAT_SWITCH, INPUT_PULLUP);

    delay(300);

    Serial.println();
    Serial.println("VALLEYFLOW // SENSOR NODE");
    Serial.println("Firmware v0.1 - Sensor diagnostics started");
}

void loop() {
    const uint32_t now = millis();

    if (now - lastSampleTime >= Config::SAMPLE_INTERVAL_MS) {
        lastSampleTime = now;

        const SensorReading reading = readUltrasonic();
        const bool floatWet = readFloatSwitchWet();

        printTelemetry(reading, floatWet);
    }
}