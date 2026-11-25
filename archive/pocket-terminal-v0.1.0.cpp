#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_system.h>
#include <time.h>

TFT_eSPI lcd;
TFT_eSprite ui(&lcd);
Preferences prefs;
WebServer server(80);
constexpr int A=0,B=14,BL=38,PWR=15;
const char* names[]={"HOME","RADIO","APRS","SIEC","HA","METEO","MQTT","SETUP"};
const uint16_t accents[]={TFT_CYAN,TFT_ORANGE,TFT_GREEN,TFT_SKYBLUE,TFT_CYAN,TFT_YELLOW,TFT_MAGENTA,TFT_LIGHTGREY};
const uint16_t bg=0x0884, tile=0x1927;
String ssid[2],pass[2],apKey;
uint8_t selection=0,brightness=180,slot=0;
bool inApp=false,apActive=false,asleep=false,dirty=true,connected=false;
bool restartPending=false, displayReady=false;
uint32_t apStarted=0;
uint32_t restartAt=0,lastInput=0,lastDraw=0,lastAttempt=0,attemptStarted=0;
uint32_t sleepSeconds=60;
bool trying=false;
struct Button {int pin; bool raw=true,stable=true; uint32_t changed=0,pressed=0; explicit Button(int p):pin(p){} };
Button a{A},b{B};

void line(const String& s,int y,uint16_t color=TFT_WHITE){
  ui.setTextColor(color,bg);ui.drawString(s,10,y,2);
}
void iconFor(int i,int x,int y,uint16_t c){
  switch(i){
    case 0: ui.drawLine(x-10,y,x,y-10,c);ui.drawLine(x,y-10,x+10,y,c);ui.drawRect(x-7,y,15,11,c);break;
    case 1: ui.drawCircle(x,y,9,c);ui.drawCircle(x,y,4,c);ui.drawLine(x,y,x+11,y-11,c);break;
    case 2: ui.drawCircle(x,y,10,c);ui.drawFastHLine(x-13,y,26,c);ui.drawFastVLine(x,y-13,26,c);break;
    case 3: for(int j=0;j<4;j++)ui.fillRect(x-11+j*6,y+9-j*5,4,3+j*5,c);break;
    case 4: ui.drawRect(x-10,y-9,20,18,c);ui.fillCircle(x,y,4,c);break;
    case 5: ui.drawCircle(x,y,6,c);for(int j=0;j<8;j++){float z=j*PI/4;ui.drawLine(x+8*cos(z),y+8*sin(z),x+12*cos(z),y+12*sin(z),c);}break;
    case 6: ui.drawFastHLine(x-11,y-4,22,c);ui.drawLine(x+11,y-4,x+6,y-9,c);ui.drawFastHLine(x-11,y+5,22,c);ui.drawLine(x-11,y+5,x-6,y+10,c);break;
    case 7: ui.drawCircle(x,y,10,c);ui.drawCircle(x,y,4,c);ui.drawFastHLine(x-13,y,4,c);ui.drawFastHLine(x+10,y,4,c);break;
  }
}
void render(){
  if(asleep)return;
  ui.fillSprite(bg);
  ui.setTextColor(TFT_CYAN,bg);ui.drawString("POCKET TERMINAL  v0.1.0",6,2,2);
  ui.setTextColor(connected?TFT_GREEN:TFT_LIGHTGREY,bg);
  ui.drawRightString(connected?"WiFi "+String(WiFi.RSSI())+"dBm":apActive?"SETUP AP":"OFFLINE",314,2,2);
  if(!inApp){
    for(int i=0;i<8;i++){
      int x=5+(i%4)*78,y=22+(i/4)*62;
      ui.fillRoundRect(x,y,74,58,5,tile);
      ui.drawRoundRect(x,y,74,58,5,i==selection?accents[i]:0x31aa);
      if(i==selection)ui.drawRoundRect(x+1,y+1,72,56,4,accents[i]);
      iconFor(i,x+37,y+21,accents[i]);
      ui.setTextColor(TFT_WHITE,tile);ui.drawCentreString(names[i],x+37,y+38,2);
    }
    line("A: NEXT / hold PREV",151,TFT_LIGHTGREY);
    ui.drawRightString("B: OPEN",310,151,2);
  }else{
    line(names[selection],24,accents[selection]);
    if(selection==0){
      struct tm t; char buf[40];
      time_t now=time(nullptr);
      if(now>1700000000 && localtime_r(&now,&t)){
        strftime(buf,sizeof(buf),"%H:%M:%S",&t);
        ui.setTextColor(TFT_WHITE,bg);ui.drawString(buf,10,47,2);
        strftime(buf,sizeof(buf),"%d.%m.%Y",&t);line(buf,83);
      }else line("NTP: waiting for time",52);
      line(connected?"IP: "+WiFi.localIP().toString():"WiFi: offline",104);
      line("PSRAM: "+String(ESP.getPsramSize()/1048576)+" MiB",124);
    }else if(selection==3){
      line(connected?"WiFi: connected":"WiFi: offline",50);
      line(connected?"IP: "+WiFi.localIP().toString():"IP: unavailable",72);
      line(connected?"RSSI: "+String(WiFi.RSSI())+" dBm":"RSSI: unavailable",94);
      line("Uptime: "+String(millis()/1000)+" s",116);
    }else if(selection==7){
      line("Brightness: "+String(brightness),46);
      line("B: cycle brightness",66);
      line(apActive?"AP: "+WiFi.softAPSSID():"Hold A: start setup AP",86);
      if(apActive){line("Key: "+apKey,106);line("http://192.168.4.1",126);}
    }else{
      line("Module not configured yet",55);
      line("Planned for a later release",80,TFT_LIGHTGREY);
      line("No live data",105,TFT_ORANGE);
    }
    line("Hold B: MENU",151,TFT_LIGHTGREY);
  }
  ui.pushSprite(0,0);
}
String esc(String s){s.replace("&","&amp;");s.replace("<","&lt;");s.replace(">","&gt;");s.replace("\"","&quot;");s.replace("'","&#39;");return s;}
void configPage(){
  String html="<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'><title>Pocket Terminal Setup</title><style>body{font:18px system-ui;background:#0a1522;color:white;max-width:560px;margin:30px auto;padding:20px}input,button{display:block;width:95%;padding:10px;margin:10px 0}</style><h1>Pocket Terminal</h1><p>Local configuration. Leave password blank to keep it for an unchanged SSID. Empty SSID removes network.</p><form method=POST action=/save>";
  for(int i=0;i<2;i++){
    html+="<label>WiFi "+String(i+1)+" SSID<input maxlength=32 name=s"+String(i)+" value='"+esc(ssid[i])+"'></label>";
    html+="<label>Password<input type=password maxlength=63 name=p"+String(i)+"></label>";
  }
  html+="<label>Brightness (30-255)<input type=number min=30 max=255 name=bl value='"+String(brightness)+"'></label><label>Screen timeout seconds (0=never)<input type=number min=0 max=3600 name=sl value='"+String(sleepSeconds)+"'></label><button>Save and restart</button></form>";
  server.sendHeader("Cache-Control","no-store");server.send(200,"text/html",html);
}
void startAP(){
  if(apActive)return;
  WiFi.mode(WIFI_AP_STA);
  char key[13];snprintf(key,sizeof(key),"%08lX",(unsigned long)esp_random());apKey=key;
  if(!WiFi.softAP("Pocket-Terminal-Setup",apKey.c_str()))return;
  apActive=true;apStarted=millis();dirty=true;
  server.on("/",HTTP_GET,[]{if(!apActive||server.client().localIP()!=WiFi.softAPIP()){server.send(403);return;}configPage();});
  server.on("/save",HTTP_POST,[]{
    if(!apActive||server.client().localIP()!=WiFi.softAPIP()){server.send(403);return;}
    for(int i=0;i<2;i++){
      String ns=server.arg("s"+String(i)),np=server.arg("p"+String(i));
      if(ns.length()>32||np.length()>63||(np.length()>0&&np.length()<8)){server.send(400,"text/plain","Invalid WiFi credentials");return;}
    }
    for(int i=0;i<2;i++){
      String ns=server.arg("s"+String(i)),np=server.arg("p"+String(i));
      if(ns!=ssid[i]||np.length())pass[i]=np;
      ssid[i]=ns;if(!ns.length())pass[i]="";
      prefs.putString(("s"+String(i)).c_str(),ssid[i]);prefs.putString(("p"+String(i)).c_str(),pass[i]);
    }
    brightness=constrain(server.arg("bl").toInt(),30,255);
    sleepSeconds=constrain(server.arg("sl").toInt(),0,3600);
    prefs.putUChar("bl",brightness);prefs.putUInt("sl",sleepSeconds);
    server.send(200,"text/plain","Saved. Restarting.");restartPending=true;restartAt=millis();
  });
  server.begin();
}
void handleWiFi(){
  bool now=WiFi.status()==WL_CONNECTED;
  if(now!=connected){connected=now;dirty=true;if(now){trying=false;configTzTime("CET-1CEST,M3.5.0,M10.5.0/3","pool.ntp.org","time.nist.gov");}}
  if(now)return;
  uint32_t ms=millis();
  if(trying && ms-attemptStarted>15000){WiFi.disconnect();trying=false;slot=(slot+1)%2;lastAttempt=ms;}
  if(!trying && ms-lastAttempt>1500){
    lastAttempt=ms;
    if(ssid[slot].length()){
      WiFi.begin(ssid[slot].c_str(),pass[slot].c_str());trying=true;attemptStarted=ms;
    }else slot=(slot+1)%2;
  }
}
int poll(Button& x){
  bool v=digitalRead(x.pin);uint32_t n=millis();
  if(v!=x.raw){x.raw=v;x.changed=n;}
  if(v!=x.stable&&n-x.changed>30){x.stable=v;if(!v)x.pressed=n;else return n-x.pressed>=700?2:1;}
  return 0;
}
void setup(){
  Serial.begin(115200);pinMode(A,INPUT_PULLUP);pinMode(B,INPUT_PULLUP);
  pinMode(PWR,OUTPUT);digitalWrite(PWR,HIGH);
  prefs.begin("pocket",false);
  brightness=prefs.getUChar("bl",180);sleepSeconds=prefs.getUInt("sl",60);
  for(int i=0;i<2;i++){ssid[i]=prefs.getString(("s"+String(i)).c_str(),"");pass[i]=prefs.getString(("p"+String(i)).c_str(),"");}
  lcd.init();lcd.setRotation(1);ledcSetup(0,5000,8);ledcAttachPin(BL,0);ledcWrite(0,brightness);
  ui.setColorDepth(16);if(!ui.createSprite(320,170)){lcd.drawString("Display buffer allocation failed",0,0,2);return;}
  displayReady=true;
  ui.fillSprite(bg);ui.setTextColor(TFT_CYAN,bg);ui.drawCentreString("POCKET TERMINAL",160,50,2);ui.drawCentreString("LILYGO T-Display-S3",160,95,2);ui.pushSprite(0,0);delay(800);
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(false);
  if(!ssid[0].length()&&!ssid[1].length()){selection=7;inApp=true;startAP();}
  lastInput=millis();dirty=true;
  Serial.println("POCKET_TERMINAL_V0.1.0");
}
void loop(){
  if(!displayReady){delay(100);return;}
  handleWiFi();if(apActive)server.handleClient();
  if(apActive&&!restartPending&&millis()-apStarted>600000UL){server.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);apActive=false;dirty=true;}
  if(restartPending&&millis()-restartAt>1000)ESP.restart();
  int ea=poll(a),eb=poll(b);
  if(ea||eb){
    lastInput=millis();
    if(asleep){asleep=false;ledcWrite(0,brightness);}
    else if(!inApp){if(ea)selection=(selection+(ea==2?7:1))%8;if(eb==1)inApp=true;}
    else if(eb==2){inApp=false;}
    else if(selection==7){if(ea==2)startAP();if(eb==1){brightness=brightness==180?80:brightness==80?255:180;ledcWrite(0,brightness);prefs.putUChar("bl",brightness);}}
    dirty=true;
  }
  if(!asleep&&sleepSeconds&&millis()-lastInput>sleepSeconds*1000UL){asleep=true;ledcWrite(0,0);}
  if(!asleep&&(dirty||millis()-lastDraw>=1000)){render();dirty=false;lastDraw=millis();}
  delay(10);
}
