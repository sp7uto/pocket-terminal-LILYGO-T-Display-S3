#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <atomic>
#include <math.h>

static String aprsCall, aprsWatch[3];
static float aprsRadius=15, aprsLat=51.590748, aprsLon=18.736974;
static bool aprsAll=false;
static std::atomic<bool> aprsRefresh(false),aprsOnline(false);
struct AprsStation {char call[16]={};float lat=0,lon=0,km=0,bearing=0,speed=NAN;uint32_t seen=0;char symbol=0;};
static AprsStation aprsStations[24];
static portMUX_TYPE aprsMux=portMUX_INITIALIZER_UNLOCKED;
static bool aprsValidCall(const String& s){
 if(s.length()<3||s.length()>15)return false;
 for(unsigned i=0;i<s.length();i++)if(!isalnum((unsigned char)s[i])&&s[i]!='-')return false;
 return true;
}
static bool aprsDigits(const String& s,int start,int count){
 for(int i=start;i<start+count;i++)if(i>=s.length()||!isdigit((unsigned char)s[i]))return false;
 return true;
}
static bool aprsBase91(const String& s,int pos,int count,uint32_t& v){
 v=0;for(int i=0;i<count;i++){int c=s[pos+i]-33;if(c<0||c>90)return false;v=v*91+c;}return true;
}
static bool aprsDecode(const String& s,AprsStation& p){
 if(s.length()<2)return false;
 int pos=(s[0]=='!'||s[0]=='=')?1:(s[0]=='/'||s[0]=='@')?8:-1;
 if(pos<0||s.length()<pos+13)return false;
 if(aprsDigits(s,pos,4)&&s[pos+4]=='.'){
  if(s.length()<pos+19||!aprsDigits(s,pos+5,2)||!aprsDigits(s,pos+9,5)||s[pos+14]!='.'||!aprsDigits(s,pos+15,2))return false;
  char ns=s[pos+7],ew=s[pos+17];if((ns!='N'&&ns!='S')||(ew!='E'&&ew!='W'))return false;
  float lm=s.substring(pos+2,pos+7).toFloat(),om=s.substring(pos+12,pos+17).toFloat();if(lm>=60||om>=60)return false;
  p.lat=(s.substring(pos,pos+2).toInt()+lm/60)*(ns=='S'?-1:1);
  p.lon=(s.substring(pos+9,pos+12).toInt()+om/60)*(ew=='W'?-1:1);
  p.symbol=s[pos+18];
  if(s.length()>=pos+26&&s[pos+22]=='/'&&aprsDigits(s,pos+19,3)&&aprsDigits(s,pos+23,3))p.speed=s.substring(pos+23,pos+26).toInt()*1.852f;
 }else{
  uint32_t y,x;if(!aprsBase91(s,pos+1,4,y)||!aprsBase91(s,pos+5,4,x))return false;
  p.lat=90-y/380926.0;p.lon=-180+x/190463.0;p.symbol=s[pos+9];
  int c=s[pos+10]-33,v=s[pos+11]-33;if(c>=0&&c<90&&v>=0&&v<=89)p.speed=(powf(1.08f,v)-1)*1.852f;
 }
 if(!isfinite(p.lat)||!isfinite(p.lon)||fabs(p.lat)>90||fabs(p.lon)>180)return false;
 double a=aprsLat*DEG_TO_RAD,b=p.lat*DEG_TO_RAD,dl=(p.lon-aprsLon)*DEG_TO_RAD,dp=b-a;
 double h=sin(dp/2)*sin(dp/2)+cos(a)*cos(b)*sin(dl/2)*sin(dl/2);
 p.km=6371*2*asin(sqrt(constrain(h,0.0,1.0)));
 p.bearing=fmod(atan2(sin(dl)*cos(b),cos(a)*sin(b)-sin(a)*cos(b)*cos(dl))*RAD_TO_DEG+360,360);
 return true;
}
static int aprsWatchIndex(const char* c){for(int i=0;i<3;i++)if(aprsWatch[i].length()&&aprsWatch[i]==c)return i;return -1;}
static bool aprsMobile(char c){return c=='>'||c=='b'||c=='['||c=='j'||c=='k'||c=='v'||c=='u'||c=='s'||c=='<'||c=='^'||c=='R';}
static void aprsPacket(const String& packet){
 int gt=packet.indexOf('>'),colon=packet.indexOf(':');if(gt<=0||colon<=gt||gt>15)return;
 String call=packet.substring(0,gt);if(!aprsValidCall(call))return;
 AprsStation p;if(!aprsDecode(packet.substring(colon+1),p))return;
 call.toCharArray(p.call,sizeof(p.call));p.seen=millis();
 int w=aprsWatchIndex(p.call);
 portENTER_CRITICAL(&aprsMux);
 int slot=w;
 if(w<0){
  if(p.km>aprsRadius||(!aprsAll&&!aprsMobile(p.symbol))){for(int i=3;i<24;i++)if(!strcmp(aprsStations[i].call,p.call))aprsStations[i]=AprsStation();portEXIT_CRITICAL(&aprsMux);return;}
  slot=3;
  for(int i=3;i<24;i++){if(!strcmp(aprsStations[i].call,p.call)){slot=i;break;}if(!aprsStations[i].seen||millis()-aprsStations[i].seen>millis()-aprsStations[slot].seen)slot=i;}
 }
 aprsStations[slot]=p;portEXIT_CRITICAL(&aprsMux);
}
static void aprsSnapshot(AprsStation* out){portENTER_CRITICAL(&aprsMux);memcpy(out,aprsStations,sizeof(aprsStations));portEXIT_CRITICAL(&aprsMux);}
static void aprsWorker(void*){
 WiFiClient client;String buffer;buffer.reserve(512);bool overflow=false;uint32_t last=0;
 for(;;){
  if(WiFi.status()!=WL_CONNECTED||!aprsValidCall(aprsCall)){client.stop();aprsOnline=false;vTaskDelay(pdMS_TO_TICKS(1000));continue;}
  if(aprsRefresh.exchange(false)){client.stop();aprsOnline=false;last=0;}
  if(!client.connected()){
   aprsOnline=false;if(last&&millis()-last<15000){vTaskDelay(pdMS_TO_TICKS(500));continue;}last=millis();
   if(client.connect("euro.aprs2.net",14580,4000)){
    String login="user "+aprsCall+" pass -1 vers PocketTerminal 0.4 filter r/"+String(aprsLat,6)+"/"+String(aprsLon,6)+"/"+String(aprsRadius,0);
    String buddies;for(int i=0;i<3;i++)if(aprsWatch[i].length())buddies+="/"+aprsWatch[i];
    if(buddies.length())login+=" b"+buddies;
    client.print(login+"\r\n");buffer="";overflow=false;
   }
  }
  int budget=2048;
  while(client.available()&&budget--){
   char c=client.read();if(c=='\n'){
    if(!overflow){if(buffer.startsWith("#")&&buffer.indexOf("logresp")>=0)aprsOnline=buffer.indexOf("unverified")>=0||buffer.indexOf("verified")>=0;else if(!buffer.startsWith("#"))aprsPacket(buffer);}
    buffer="";overflow=false;
   }else if(c!='\r'){if(buffer.length()<512&&!overflow)buffer+=c;else overflow=true;}
  }
  vTaskDelay(pdMS_TO_TICKS(50));
 }
}
