#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include "weather_ca.h"

struct WeatherData {
  bool valid=false, failed=false;
  float temp=NAN, humidity=NAN, pressure=NAN, wind=NAN, direction=NAN, rain=NAN, clouds=NAN;
  float low=NAN, high=NAN;
  int code=-1;
  char sunrise[6]="--:--", sunset[6]="--:--";
  uint32_t updated=0;
};
static WeatherData weather;
static portMUX_TYPE weatherMux=portMUX_INITIALIZER_UNLOCKED;
static char weatherLat[16]="51.590748", weatherLon[16]="18.736974";
static volatile bool weatherRefresh=true;
static WeatherData weatherSnapshot(){
  portENTER_CRITICAL(&weatherMux);
  WeatherData d=weather;
  portEXIT_CRITICAL(&weatherMux);
  return d;
}
static void weatherTime(char* out, const char* source){
  if(source && strlen(source)>=16){memcpy(out,source+11,5);out[5]=0;}
}
static void weatherWorker(void*){
  uint32_t last=0, interval=0;
  for(;;){
    uint32_t now=millis();
    if(WiFi.status()==WL_CONNECTED && time(nullptr)>1700000000 &&
      (weatherRefresh || !interval || now-last>=interval)){
      weatherRefresh=false;last=now;
      WiFiClientSecure client;
      // Use the ESP-IDF certificate bundle, never disable TLS verification.
      client.setCACert(WEATHER_CA);
      client.setTimeout(8000);
      HTTPClient http;
      String url=String("https://api.open-meteo.com/v1/forecast?latitude=")+weatherLat+"&longitude="+weatherLon+
        "&current=temperature_2m,relative_humidity_2m,surface_pressure,wind_speed_10m,wind_direction_10m,precipitation,cloud_cover,weather_code&daily=temperature_2m_min,temperature_2m_max,sunrise,sunset&timezone=auto&forecast_days=1";
      WeatherData next=weatherSnapshot();
      bool ok=false;
      http.setConnectTimeout(6000);http.setTimeout(8000);
      if(http.begin(client,url)){
        int status=http.GET();
        if(status==200){
          JsonDocument doc;
          DeserializationError err=deserializeJson(doc,http.getString());
          if(!err && doc["current"]["temperature_2m"].is<float>()){
            JsonObject cur=doc["current"];
            next.temp=cur["temperature_2m"]|NAN;
            next.humidity=cur["relative_humidity_2m"]|NAN;
            next.pressure=cur["surface_pressure"]|NAN;
            next.wind=cur["wind_speed_10m"]|NAN;
            next.direction=cur["wind_direction_10m"]|NAN;
            next.rain=cur["precipitation"]|NAN;
            next.clouds=cur["cloud_cover"]|NAN;
            next.code=cur["weather_code"]|-1;
            next.low=doc["daily"]["temperature_2m_min"][0]|NAN;
            next.high=doc["daily"]["temperature_2m_max"][0]|NAN;
            weatherTime(next.sunrise,doc["daily"]["sunrise"][0]|"");
            weatherTime(next.sunset,doc["daily"]["sunset"][0]|"");
            next.valid=true;next.updated=millis();ok=true;
          }
        }
        http.end();
      }
      next.failed=!ok;interval=ok?900000:60000;
      portENTER_CRITICAL(&weatherMux);weather=next;portEXIT_CRITICAL(&weatherMux);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
