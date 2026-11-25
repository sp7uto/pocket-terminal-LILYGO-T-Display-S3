#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
TFT_eSPI lcd;
constexpr int A=0,B=14,BL=38,PWR=15;
uint8_t page=0, brightness=180;
bool prevA=true,prevB=true;
uint32_t lastA=0,lastB=0;
int networks=-2;
void draw(){
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_CYAN,TFT_BLACK);
  lcd.drawString("POCKET TERMINAL  v0.0.1",8,5,2);
  lcd.drawFastHLine(8,25,304,TFT_DARKGREY);
  lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  if(page==0){
    lcd.drawString("LILYGO T-Display-S3 / TEST",8,34,2);
    lcd.drawString(String("Flash: ")+ESP.getFlashChipSize()/1048576+" MiB",8,54,2);
    lcd.drawString(String("PSRAM: ")+ESP.getPsramSize()/1048576+" MiB",8,74,2);
    lcd.drawString(String("Free heap: ")+ESP.getFreeHeap()/1024+" KiB",8,94,2);
    lcd.drawString(String("LCD: ")+lcd.width()+" x "+lcd.height(),8,114,2);
  } else if(page==1){
    uint16_t colors[]={TFT_RED,TFT_GREEN,TFT_BLUE,TFT_WHITE,TFT_YELLOW,TFT_CYAN};
    for(int i=0;i<6;i++)lcd.fillRect(10+i*50,40,48,78,colors[i]);
    lcd.drawString("RGB / WHITE / YELLOW / CYAN",8,125,2);
  } else if(page==2){
    lcd.drawString("BUTTON / BACKLIGHT TEST",8,40,2);
    lcd.drawString(String("Brightness: ")+brightness,8,66,2);
    lcd.drawString("B: change brightness",8,100,2);
  } else {
    lcd.drawString("WI-FI PASSIVE SCAN",8,40,2);
    lcd.drawString(networks==-2?"B: start scan":networks==-1?"Scanning...":networks<0?"Scan failed":String("Networks found: ")+networks,8,70,2);
    lcd.drawString("No connection / no credentials",8,100,2);
  }
  lcd.drawFastHLine(8,145,304,TFT_DARKGREY);
  lcd.setTextColor(TFT_CYAN,TFT_BLACK);
  lcd.drawString("A: NEXT",8,150,2);
  lcd.drawString("B: ACTION",220,150,2);
}
void setup(){
  Serial.begin(115200);
  pinMode(A,INPUT_PULLUP);pinMode(B,INPUT_PULLUP);
  pinMode(PWR,OUTPUT);digitalWrite(PWR,HIGH);
  lcd.init();lcd.setRotation(1);
  ledcSetup(0,5000,8);ledcAttachPin(BL,0);ledcWrite(0,brightness);
  WiFi.mode(WIFI_OFF);
  draw();
  Serial.printf("POCKET_DIAG_READY flash=%u psram=%u heap=%u\n",ESP.getFlashChipSize(),ESP.getPsramSize(),ESP.getFreeHeap());
}
void loop(){
  bool a=digitalRead(A),b=digitalRead(B);uint32_t now=millis();
  if(!a&&prevA&&now-lastA>180){lastA=now;page=(page+1)%4;Serial.println("BUTTON_A");draw();}
  if(!b&&prevB&&now-lastB>180){
    lastB=now;Serial.println("BUTTON_B");
    if(page==2){brightness=brightness==180?80:brightness==80?255:180;ledcWrite(0,brightness);}
    if(page==3&&networks!=-1){WiFi.mode(WIFI_STA);networks=WiFi.scanNetworks(true,false,true,300);}
    draw();
  }
  prevA=a;prevB=b;
  if(networks==-1){int n=WiFi.scanComplete();if(n!=-1){networks=n;WiFi.scanDelete();WiFi.mode(WIFI_OFF);if(page==3)draw();Serial.printf("WIFI_SCAN=%d\n",n);}}
  delay(15);
}
