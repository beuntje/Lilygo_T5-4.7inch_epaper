#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "epd_driver.h"
#include "esp_adc_cal.h"

#include "credentials.h"

#define BATT_PIN 36

uint8_t *framebuffer;
int vref = 1100;
RTC_DATA_ATTR int next_refresh = 0;
int default_sleep_time = 30;

void setup() {
  Serial.begin(115200);
  check_time();
  init_battery();
  init_screen() ;
  if (connect_wifi()) {
    get_image();
    update_screen();
    send_battery_power();
    set_sleeptime();
    disconnect_wifi();
  }
  goto_sleep(default_sleep_time);
}

void update_screen() {
  epd_poweron();
  epd_clear();

  volatile uint32_t t1 = millis();
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  volatile uint32_t t2 = millis();
  Serial.printf("EPD draw took %dms.\n", t2 - t1);

  epd_poweroff_all();
}

void loop() {
  Serial.println("loop: what am I doing here?");
  int color;
  for (int x = 0; x <= EPD_WIDTH; x++) {
    for (int y = 0; y <= EPD_HEIGHT; y++) {
      color = (x + y) % 255; // random(0, 255);
      epd_draw_pixel(x, y, color , framebuffer);
    }
  }

  update_screen();

  delay(300000);
}

void init_screen() {
  epd_init();
  framebuffer = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

bool connect_wifi() {
  int loops = 0;
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++loops > 20) return false;
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  return true;
}

void disconnect_wifi() {
  WiFi.disconnect(true);
}

int get_next_color(String &dataline) {
  char val = dataline.charAt(0);
  if (dataline.charAt(1) < 65) {
    int count = 0;
    while (dataline.charAt(1) >= 48 && dataline.charAt(1) < 58) {
      count = count * 10 + (dataline.charAt(1) - 48);
      dataline.remove(1, 1);
    }
    for (int i = 0; i < count - 1; i++) {
      dataline = val + dataline;
    }
  }
  dataline.remove(0, 1);
  int color = (val - 65) * 16;
  if (color >= 256) color = 255;
  return color;
}

void get_image() {
  String url_part;
  int color;

  HTTPClient http;
  int httpResponseCode;
  String payload;

  int block_width = 20;

  Serial.print ("EPD_WIDTH: ");
  Serial.println (EPD_WIDTH);
  Serial.print ("EPD_H: ");
  Serial.println (EPD_HEIGHT);

  for (int block_x = 0; block_x < EPD_WIDTH; block_x += block_width) {
    Serial.print ("x: ");
    Serial.println(block_x);
    url_part = url + "?x=" + block_x + "&y=0&w=" + block_width + "&h=" + EPD_HEIGHT;
    http.begin(url_part);

    int retries = 2;
    while (retries > 0) {

      httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        payload = http.getString();
        Serial.println(payload);

        for (int x = block_x; x < block_x + block_width; x ++ ) {
          for (int y = 0; y < EPD_HEIGHT; y ++) {
            color  = get_next_color(payload);
            epd_draw_pixel(x, y, color , framebuffer);
          }
        }
        retries = 0;
      } else {
        Serial.println("HTTP ERROR ");
        retries --;
      }
    }

    http.end();

    //update_screen();
  }

}

void init_battery() {
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    Serial.println('.');
    vref = adc_chars.vref;
  }
}

void send_battery_power() {
  HTTPClient http;

  // Correct the ADC reference voltage

  uint16_t v = analogRead(BATT_PIN);
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  String voltage = "Voltage :" + String(battery_voltage) + "V";
  Serial.println(voltage);

  String url = thingspeak + battery_voltage;

  http.begin(url.c_str());
  int httpResponseCode = http.GET();
  http.end();
}


void goto_sleep(int sleep_time) {
  if (sleep_time > next_refresh) sleep_time = next_refresh;
  next_refresh -= sleep_time;
  Serial.print("going to sleep for ");
  Serial.print(sleep_time);
  Serial.println(" minutes");
  esp_sleep_enable_timer_wakeup(sleep_time * 60 * 1000000);
  esp_deep_sleep_start();
}

void check_time() {
  if (next_refresh > 0) {
    Serial.print ("count: ");
    Serial.println(next_refresh);
    goto_sleep(default_sleep_time);
  }
}


void set_sleeptime() {
  HTTPClient http;

  String gettime = url + "?time";  
  http.begin(gettime.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
    next_refresh = payload.toInt();
    Serial.print ("Gotten next_refresh: ");
    Serial.println(next_refresh);
  }
  http.end();
}
