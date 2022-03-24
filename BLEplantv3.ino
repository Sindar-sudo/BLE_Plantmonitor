/*
   ESP32 based BLE beacon for liquid sensors
   See https://github.com/oh2mp/esp32_watersensor
   set CPU to 80Mhz!!!
*/

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "driver/adc.h"
#include "MAX17043.h"

#define SENS 36 //sensor's data output
#define volt 12 //sensor's power pin
#define uS_TO_S_FACTOR 1000000ULL   /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3600        /* Time ESP32 will go to sleep (in seconds) */
#define MIN 1500 //completely in water //plant2 =1500 //plant1=1000
#define MAX 3600 //completely dry //plant2 = 3600 //plant1 = 3100

int sensorbuff;
byte sensor;
byte battery;

char datastr[16];
char convtable[255];

BLEAdvertising *advertising;
std::string mfdata = "";

/* ----------------------------------------------------------------------------------
   Set up data packet for advertising
*/
void set_beacon() {
  BLEBeacon beacon = BLEBeacon();
  BLEAdvertisementData advdata = BLEAdvertisementData();
  BLEAdvertisementData scanresponse = BLEAdvertisementData();

  advdata.setFlags(0x06); // BR_EDR_NOT_SUPPORTED 0x04 & LE General discoverable 0x02

  mfdata = "";
  mfdata += (char)0xE5; mfdata += (char)0x02;  // Espressif Incorporated Vendor ID = 0x02E5
  mfdata += (char)(sensorbuff >> 8);          // MSB
  mfdata += (char)(sensorbuff & 0xFF);        // LSB 
 // mfdata += (char)0x00; mfdata += (char)0x00;  // Empty 2 bytes (for temperature maybe)
  mfdata += (char)(sensor);                    // Raw (calculated) value from the sensor
  mfdata += (char)(battery);                   // Battery value if any
  mfdata += (char)0xBE; mfdata += (char)0xEF;  // Beef is always good nutriment

  advdata.setManufacturerData(mfdata);
  advertising->setAdvertisementData(advdata);
  advertising->setScanResponseData(scanresponse);
}


void setup() {

  pinMode(SENS, INPUT);
  pinMode(volt, OUTPUT);
  FuelGauge.begin();
  digitalWrite(SDA, 0); //disable double resistors https://github.com/porrey/max1704x/issues/1
  digitalWrite(SCL, 0);
  FuelGauge.quickstart();
  //Serial.begin(115200);
  BLEDevice::init("ESP32_plantmonitor");
  advertising = BLEDevice::getAdvertising();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

/* ---------------------------------------------------------------------------------- */
void loop() {
  // Read sensor and calculate value
  digitalWrite(volt, HIGH); //give power to sensor
  FuelGauge.wake();
  delay(500);
  sensorbuff = analogRead(SENS);
  battery = round(FuelGauge.percent());
  digitalWrite(volt, LOW); //cut power
  FuelGauge.sleep();
  sensor = map(sensorbuff, MIN, MAX, 99, 0); //map min and max values to 0-99
  //Serial.println(sensorbuff);
  //Serial.println(sensor);
  set_beacon();  //put variables into the beacon
  advertising->start();
  delay(8000); //advertise for 10sec
  advertising->stop();
  //esp_bluedroid_disable();
  //esp_bluedroid_deinit();
  btStop();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  adc_power_off(); //turn everything off
  esp_deep_sleep_start();
}

/* ---------------------------------------------------------------------------------- */
