/*
 * ESP32 Ultimate Dashboard: CAN Bus + DWIN + OLED (U8g2) + DHT22 + AWS IoT
 * Features:
 * 1. Reads Vehicle Data (CAN) -> Sends to DWIN.
 * 2. Reads Cabin Temp/Hum (DHT22) -> Sends to DWIN (Split Float/Int VPs).
 * 3. LISTENS to DWIN Touch (VP 0x5000) -> Toggles LED & Updates OLED.
 * 4. PUBLISHES all data to AWS IoT Core (MQTT).
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <mcp2515.h>
#include "DHT.h"

// --- AWS & WiFi Libraries ---
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

/* ============================================================================
 * AWS IoT CORE CONFIGURATION
 * ============================================================================ */
const char* WIFI_SSID = "Pro";
const char* WIFI_PASSWORD = "987654321";
const char* AWS_IOT_ENDPOINT = "apqph638f1hgj-ats.iot.ap-south-1.amazonaws.com"; // e.g., a3k7...amazonaws.com
const char* AWS_IOT_TOPIC = "sensors/EV";
const char* CLIENT_ID = "esp32-dash-monitor";

// Certificates (PASTE YOURS HERE)
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUS44HT4sOUwspDLaQygi6jHMnlx8wDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDEyNDA0MzAz
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANvvrpXGoj90iPA3Cfag
sti6o3l3Z+1p1bi1rwkH+q6aO/qzp77V56CH2LfGTr84zHcMf4arQqkVeQbkfxdN
v3ht8CwuoL+CMLub2lCiA7+gMuD8fpUGU9/Ip5WKVqpWVtH6xiHXBTbQV6DbuhiE
8BoleZfl7Xk/fHajsOWeGPu/PdN41L8cXIXi/05YHDsqRzb3AgD8TTnmBTiasZde
g/a2ViXcdwIf/Es8Fl+APW8nEFA6mU9NxiijdU0VTmbVuEhG2iOMMDDUty2FG3Yg
JhZ8Lf0kHuC3MqaxQqDEeRlkcB+bmICXLckz5K1HAt/28vetZ93DRuZbvbMJNMPY
3+ECAwEAAaNgMF4wHwYDVR0jBBgwFoAU7vP42OxL0ut1c9EcqDR0SbRRjBMwHQYD
VR0OBBYEFO8rukObLlLQNTr6D1XGePbQsl6MMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBRIm79+AUbbCX831YSEeasIzjD
I+T9rDzMIadAzYokGtWLaI++nYjjG48+sNLk1FzH+L4HkX0rPWWRSdCKMEF8+5ol
Yd/rhdKkjV9l5H9PlQjApJdh5h8vc9ieTrL7gvTMHOXV6LFibaKofQ4mqdEr23YA
4gqx3d+62rdqYyb0dWG8/+HKn6zJIge+b4sfj3dPC0jMGOplJPfXAHYKjDgK/jnr
CjjTZVqnNFwBkhb+DMsz4xQsDEE8HZqYfgsWnD4gzkmYgYLhsM5sIe0b9BdipcEK
hudJYy050W23atzAElnLjOyI7oIYXYZmEe9fnbu/HJeQl32IL1vn9e02tqxm
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEA2++ulcaiP3SI8DcJ9qCy2LqjeXdn7WnVuLWvCQf6rpo7+rOn
vtXnoIfYt8ZOvzjMdwx/hqtCqRV5BuR/F02/eG3wLC6gv4Iwu5vaUKIDv6Ay4Px+
lQZT38inlYpWqlZW0frGIdcFNtBXoNu6GITwGiV5l+XteT98dqOw5Z4Y+78903jU
vxxcheL/TlgcOypHNvcCAPxNOeYFOJqxl16D9rZWJdx3Ah/8SzwWX4A9bycQUDqZ
T03GKKN1TRVOZtW4SEbaI4wwMNS3LYUbdiAmFnwt/SQe4LcyprFCoMR5GWRwH5uY
gJctyTPkrUcC3/by961n3cNG5lu9swk0w9jf4QIDAQABAoIBAQCa8T4Tcj53acfX
Q08XmpqkqMfmGML0tOzGFejb+e0W+L1snwh8HwHxTALXZTOw73jMHdfxrdFmgSQk
QPFwH2vWkczzs123zSY75U5mI910MI3nhro+jpikPR4i8bSgh/beM0dGK88WkKxL
PMDb6Tulj+Ubp/Ymxx0OD7/d5AVBxGfgnJ1ah1dKd/o/6W2nDdss/6Fw0lWYNZfe
CcUkcTlBWEOoOznyr7dmTONcc+fH7tUhVgxb+mqGcsGydj6q7POQok8iEMoVMR2Y
uvkDPzZY8RvBBrOejkMdKsF40fgm2O7l8d9eiKKzMFQFQc/sbNQBHf6PiQQw232C
azSqi2DpAoGBAP13ftMmXYzvdUCU0m8vtrGioYyIuj6Mvr5zI9kN0HjVgus8PwSj
avcdg73BXE/x0fj44NV6OaSCjfa8GuzILghLYGkjzCgXIdwQCNcUi1K/htP23unr
rPZ+iWnDh9149zZA5v4sKksTjlcQrfip/tZZkmb7yvxXx4LmUhz/OYhPAoGBAN4i
Zb2OeQD4UcBSexOsyJhCImO2iaMu7TvcgpgYaDrpNn60WPUSGmCZlepQilk8T8BS
Q+BG0aANeZNuDsTAwdhBiJILI7EU4RKHr5oDQNNAB0eJoiqJh0U4WNNglVRIoB3x
HBUToTAMYfej6By6mEkTVQOf2IhWRL/ietZ8X9jPAoGBAIHmIs4t5FXdRtchLjOj
XVruQSLX07NIyFysf5u2s7epnN3X02gaJDCfJKw0E445HWYejoN5j18bNYxU3Ouq
r8S6dJ+NpAyLxmOUqCMqOTjgRUYOSDHUAaGWOfBFMDU10GQyoO90TPyK+jDusGo3
HQ5Xe2th8aafifUw+rY3qxGLAoGBANui5zYmST7MSQkAhPFPRohF2/r74duX6rnl
rrr0ZxrYvSlK6SpoW+xn7/Ne72yMVc6ziKmZXGwE5tD/YVrpvME3CvJcUU4mduT7
hIWj8dTu2kPBsrachUPMEwft17keljME09+DKT15AOT7C+ZcGXcnxkdvJYqowhF4
vjyVfa/BAoGAUyb/CTEaE5T5enQE6M9kCwt48NvvsZP64Cq+N1ZAV0QxrJmhqHqv
9t9jY5RNuI2SsEFHWW7VJyQXD5tL3q2XoxdKQGwTiNWsfWNrJHRiIFDGFAhxHQf9
1rLZuElUeHE7oZX8r/GmIE635KZYqaMtzSD+lAXjml2dcxV64Dpbp/c=
-----END RSA PRIVATE KEY-----
)EOF";

/* ============================================================================
 * PIN DEFINITIONS
 * ============================================================================ */

// --- OLED SETTINGS (U8g2) ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// --- DWIN Display (UART2) ---
#define RXD2 16  
#define TXD2 17  

// --- DHT22 Sensor ---
#define DHTPIN          32  
#define DHTTYPE         DHT22   

// --- MCP2515 CAN Module ---
#define CAN_CS_PIN      5   
#define CAN_INT_PIN     4   

// --- Status LEDs ---
#define LED_TOGGLE_PIN  2   
#define LED_BLUE_PIN    15  
#define LED_RED_PIN     33  

/* ============================================================================
 * GLOBAL OBJECTS
 * ============================================================================ */

MCP2515 mcp2515(CAN_CS_PIN);
HardwareSerial dwinSerial(2); 
DHT dht(DHTPIN, DHTTYPE);

// --- AWS Objects ---
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Timers
unsigned long lastDHTReadTime = 0;
unsigned long lastDashboardPrint = 0;

// State Variables
bool isDeviceOn = false; 

struct VehicleData {
    uint16_t rpm;
    uint16_t vehicle_speed;
    float battery_voltage;
    uint8_t battery_soc;
    int16_t battery_current;
    float battery_temperature;
    uint8_t door_status;
    uint8_t seatbelt_status;
    uint32_t last_can_time;
    float cabin_temp;
    float cabin_humidity;
};

VehicleData vehicle_data = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

void updateOLED(String title, String status) {
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 15);
  u8g2.print(title);
  u8g2.setFont(u8g2_font_ncenB18_tr); 
  u8g2.setCursor(0, 45);
  u8g2.print(status);
  if (status == "ON") {
     u8g2.drawFrame(0, 0, 128, 64);
  }
  u8g2.sendBuffer(); 
}

void sendIntNumber(int intValue, uint16_t VPAddress) {
  dwinSerial.write(0x5A); dwinSerial.write(0xA5); dwinSerial.write(0x05); 
  dwinSerial.write(0x82); 
  dwinSerial.write((byte)((VPAddress >> 8) & 0xFF)); 
  dwinSerial.write((byte)(VPAddress & 0xFF));        
  dwinSerial.write((byte)((intValue >> 8) & 0xFF));  
  dwinSerial.write((byte)(intValue & 0xFF));         
}

void FloatToHex(float f, byte* hex) {
  byte* f_byte = reinterpret_cast<byte*>(&f);
  memcpy(hex, f_byte, 4);
}

void sendFloatNumber(float floatValue, uint16_t VPAddress) {
  dwinSerial.write(0x5A); dwinSerial.write(0xA5); dwinSerial.write(0x07); 
  dwinSerial.write(0x82); 
  dwinSerial.write((byte)((VPAddress >> 8) & 0xFF)); 
  dwinSerial.write((byte)(VPAddress & 0xFF));        
  byte hex[4] = {0}; 
  FloatToHex(floatValue, hex);
  dwinSerial.write(hex[3]); dwinSerial.write(hex[2]); dwinSerial.write(hex[1]); dwinSerial.write(hex[0]);
}

/* ============================================================================
 * AWS CONNECTION FUNCTIONS
 * ============================================================================ */

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Wi-Fi!");
}

void connectToAWS() {
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);

  Serial.print("Connecting to AWS IoT Core");
  while (!client.connect(CLIENT_ID)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected to AWS IoT!");
}

/* ============================================================================
 * SETUP
 * ============================================================================ */

void setup() {
    Serial.begin(115200);
    dwinSerial.begin(115200, SERIAL_8N1, RXD2, TXD2);

    u8g2.begin();
    updateOLED("System Init", "WAITING...");

    pinMode(LED_TOGGLE_PIN, OUTPUT);
    digitalWrite(LED_TOGGLE_PIN, LOW); 
    pinMode(LED_BLUE_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);

    dht.begin();
    SPI.begin();
    mcp2515.reset();
    mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();
    Serial.println("[INIT] CAN System Ready.");

    // --- Init AWS & WiFi ---
    connectToWiFi();
    connectToAWS();

    // Final Ready State
    updateOLED("System Ready", "CONNECTED");
    Serial.println("[INIT] AWS Connected. Touch Icon on DWIN to Toggle.");
}

/* ============================================================================
 * LOGIC: DWIN INPUT & CAN PARSING
 * ============================================================================ */

void CheckDWINInput() {
  if (dwinSerial.available()) {
    if(dwinSerial.read() == 0x5A) {
      if(dwinSerial.read() == 0xA5) {
        int len = dwinSerial.read();
        int cmd = dwinSerial.read();
        unsigned char vpHigh = dwinSerial.read();
        unsigned char vpLow = dwinSerial.read();
        uint16_t address = (vpHigh << 8) | vpLow;
        unsigned char dataHigh = dwinSerial.read();
        unsigned char dataLow = dwinSerial.read();
        uint16_t value = (dataHigh << 8) | dataLow; 

        if (address == 0x5000) {
          isDeviceOn = !isDeviceOn;
          if(isDeviceOn) {
            digitalWrite(LED_TOGGLE_PIN, HIGH);
            updateOLED("DEVICE STATUS", "ON");
          } else {
            digitalWrite(LED_TOGGLE_PIN, LOW);
            updateOLED("DEVICE STATUS", "OFF");
          }
        } 
      }
    }
  }
}

void Parse_MotorRPM(struct can_frame *frame) {
    if (frame->can_dlc >= 2) {
        vehicle_data.rpm = (frame->data[0] << 8) | frame->data[1];
        vehicle_data.last_can_time = millis();
        sendIntNumber(vehicle_data.rpm, 0x1200); 
    }
}

void Parse_VehicleSpeed(struct can_frame *frame) {
    if (frame->can_dlc >= 2) {
        vehicle_data.vehicle_speed = (frame->data[0] << 8) | frame->data[1];
        vehicle_data.last_can_time = millis();
        sendIntNumber(vehicle_data.vehicle_speed, 0x1000); 
    }
}

void Parse_BatteryStatus(struct can_frame *frame) {
    if (frame->can_dlc >= 6) { 
        uint16_t voltage_raw = (frame->data[0] << 8) | frame->data[1];
        vehicle_data.battery_voltage = voltage_raw / 100.0;
        vehicle_data.battery_soc = frame->data[2];
        vehicle_data.battery_current = (int16_t)((frame->data[3] << 8) | frame->data[4]);
        vehicle_data.battery_temperature = (float)frame->data[5];
        vehicle_data.last_can_time = millis();

        sendIntNumber(vehicle_data.battery_soc, 0x1100);       
        sendFloatNumber(vehicle_data.battery_voltage, 0x1300); 
        sendFloatNumber(vehicle_data.battery_temperature, 0x1400);
    }
}

void Parse_DoorStatus(struct can_frame *frame) {
    if (frame->can_dlc >= 2) {
        vehicle_data.door_status = frame->data[0];
        vehicle_data.seatbelt_status = frame->data[1];
        vehicle_data.last_can_time = millis();

        sendIntNumber(vehicle_data.door_status, 0x2000);     
        sendIntNumber(vehicle_data.seatbelt_status, 0x2100); 
    }
}

void ProcessCANMessage(struct can_frame *frame) {
    digitalWrite(LED_BLUE_PIN, HIGH); 
    switch (frame->can_id) {
        case 0x100: Parse_MotorRPM(frame); break;
        case 0x101: Parse_BatteryStatus(frame); break;
        case 0x102: Parse_DoorStatus(frame); break;
        case 0x105: Parse_VehicleSpeed(frame); break;
    }
    digitalWrite(LED_BLUE_PIN, LOW);
}

/* ============================================================================
 * MAIN LOOP
 * ============================================================================ */

void loop() {
    struct can_frame frame;
    unsigned long currentMillis = millis();

    // 0. CHECK AWS CONNECTION
    if (!client.connected()) {
        connectToAWS();
    }
    client.loop();

    // 1. FAST LOOP: Check CAN Messages
    if (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
        ProcessCANMessage(&frame);
    }

    // 2. FAST LOOP: Check DWIN Touch
    CheckDWINInput();

    // 3. SLOW TIMER: Read DHT22 (Every 2 seconds)
    if (currentMillis - lastDHTReadTime >= 2000) {
        lastDHTReadTime = currentMillis;

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        if (!isnan(h) && !isnan(t)) {
            vehicle_data.cabin_temp = t;
            vehicle_data.cabin_humidity = h;
            
            // --- UPDATED LOGIC (Preserved) ---
            // 1. Send FLOAT for Text Boxes
            sendFloatNumber(t, 0x3000); 
            sendFloatNumber(h, 0x3100); 

            // 2. Send INTEGER for Gauges/Bars
            int t_int = (int)t; 
            int h_int = (int)h;
            sendIntNumber(t_int, 0x3001); 
            sendIntNumber(h_int, 0x3101); 
        }
    }

    // 4. DASHBOARD PRINT & AWS PUBLISH (Every 5 seconds)
    if (currentMillis - lastDashboardPrint >= 5000) {
        lastDashboardPrint = currentMillis;
        
        // --- SERIAL LOGGING ---
        Serial.println("\n--- VEHICLE STATUS ---");
        Serial.printf("Speed: %d km/h | RPM: %d\n", vehicle_data.vehicle_speed, vehicle_data.rpm);
        Serial.printf("Bat: %d%% | %.2fV | Temp: %.1f C\n", vehicle_data.battery_soc, vehicle_data.battery_voltage, vehicle_data.battery_temperature);
        Serial.printf("Cabin: %.1f C | %.1f %%\n", vehicle_data.cabin_temp, vehicle_data.cabin_humidity);
        if(isDeviceOn) Serial.println("STATUS: [ON]"); else Serial.println("STATUS: [OFF]");
        Serial.println("----------------------");

        // --- AWS MQTT PUBLISH ---
        // Constructing JSON Payload
        char payload[512]; 
        snprintf(payload, sizeof(payload),
           "{\"device_id\":\"%s\",\"timestamp\":\"%lu\",\"status\":\"%s\",\"speed\":%d,\"rpm\":%d,\"bat_soc\":%d,\"bat_vol\":%.2f,\"bat_temp\":%.1f,\"cabin_temp\":%.1f,\"cabin_hum\":%.1f}",
           CLIENT_ID, 
           currentMillis,
           isDeviceOn ? "ON" : "OFF", // Send the toggle status
           vehicle_data.vehicle_speed, 
           vehicle_data.rpm,
           vehicle_data.battery_soc,
           vehicle_data.battery_voltage,
           vehicle_data.battery_temperature,
           vehicle_data.cabin_temp,
           vehicle_data.cabin_humidity
        );

        Serial.print("Publishing to AWS: ");
        Serial.println(payload);
        
        client.publish(AWS_IOT_TOPIC, payload);
    }
    
    // Safety Alert
    if (currentMillis - vehicle_data.last_can_time > 2000) {
       digitalWrite(LED_RED_PIN, HIGH); 
    } else {
       digitalWrite(LED_RED_PIN, LOW);
    }
}