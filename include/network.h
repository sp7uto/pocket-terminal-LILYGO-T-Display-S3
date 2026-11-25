#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <atomic>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <fcntl.h>
#include <esp_ping.h>
#include <ping/ping_sock.h>

constexpr int NET_COUNT=10;
struct NetTarget {char name[24];char host[96];uint16_t port;uint8_t kind;};
static NetTarget targets[NET_COUNT]={};
struct NetResult {int state=0;uint32_t ms=0,stamp=0;int code=0;};
static NetResult results[NET_COUNT], internet4,internet6,dnsResult;
static portMUX_TYPE netMux=portMUX_INITIALIZER_UNLOCKED;
static std::atomic<bool> netRefresh(true);
static NetResult netSnapshot(int i){
  portENTER_CRITICAL(&netMux);
  NetResult n=i==10?internet4:i==11?internet6:i==12?dnsResult:results[i];
  portEXIT_CRITICAL(&netMux);return n;
}
static void netPublish(int i,NetResult n){
  portENTER_CRITICAL(&netMux);
  if(i==10)internet4=n;else if(i==11)internet6=n;else if(i==12)dnsResult=n;else results[i]=n;
  portEXIT_CRITICAL(&netMux);
}
static NetResult tcpProbe(const char* host,uint16_t port,int family=AF_UNSPEC){
  NetResult n;n.state=2;uint32_t start=millis();
  struct addrinfo hints={},*list=nullptr;hints.ai_family=family;hints.ai_socktype=SOCK_STREAM;
  char service[8];snprintf(service,sizeof(service),"%u",port);
  if(getaddrinfo(host,service,&hints,&list)==0){
    for(auto p=list;p;p=p->ai_next){
      int fd=socket(p->ai_family,SOCK_STREAM,0);if(fd<0)continue;
      int flags=fcntl(fd,F_GETFL,0);
      if(flags<0||fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0){close(fd);continue;}
      int rc=connect(fd,p->ai_addr,p->ai_addrlen);
      if(rc==0)n.state=1;
      else if(errno==EINPROGRESS){
        fd_set w;FD_ZERO(&w);FD_SET(fd,&w);timeval tv={1,500000};
        if(select(fd+1,nullptr,&w,nullptr,&tv)>0){
          int err=0;socklen_t sz=sizeof(err);
          if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&err,&sz)==0&&err==0)n.state=1;
        }
      }
      close(fd);if(n.state==1)break;
      if(millis()-start>4000)break;
    }
    freeaddrinfo(list);
  }
  n.ms=millis()-start;n.stamp=millis();return n;
}
struct PingContext {SemaphoreHandle_t done;bool ok;uint32_t ms;};
static void pingSuccess(esp_ping_handle_t h,void* arg){
  auto* p=static_cast<PingContext*>(arg);uint32_t ms=0;
  esp_ping_get_profile(h,ESP_PING_PROF_TIMEGAP,&ms,sizeof(ms));p->ms=ms;p->ok=true;
}
static void pingEnd(esp_ping_handle_t,void* arg){xSemaphoreGive(static_cast<PingContext*>(arg)->done);}
static NetResult pingProbe(const char* host){
  NetResult n;n.state=2;
  IPAddress ip;if(!WiFi.hostByName(host,ip)){n.stamp=millis();return n;}
  esp_ping_config_t cfg=ESP_PING_DEFAULT_CONFIG();
  IP_ADDR4(&cfg.target_addr,ip[0],ip[1],ip[2],ip[3]);cfg.count=1;cfg.timeout_ms=1200;cfg.interval_ms=1200;
  PingContext p={xSemaphoreCreateBinary(),false,0};
  if(!p.done){n.stamp=millis();return n;}
  esp_ping_callbacks_t cb={};cb.on_ping_success=pingSuccess;cb.on_ping_end=pingEnd;cb.cb_args=&p;
  esp_ping_handle_t handle=nullptr;
  if(esp_ping_new_session(&cfg,&cb,&handle)==ESP_OK){
    if(esp_ping_start(handle)==ESP_OK)xSemaphoreTake(p.done,portMAX_DELAY);
    esp_ping_delete_session(handle);
  }
  vSemaphoreDelete(p.done);n.state=p.ok?1:2;n.ms=p.ms;n.stamp=millis();return n;
}
static NetResult httpProbe(const char* host,uint16_t port){
  NetResult n;n.state=2;uint32_t start=millis();
  WiFiClient client;HTTPClient http;
  http.setConnectTimeout(1500);http.setTimeout(1500);
  if(http.begin(client,String("http://")+host+":"+port+"/")){
    n.code=http.sendRequest("HEAD");n.state=n.code>0?1:2;http.end();
  }
  n.ms=millis()-start;n.stamp=millis();return n;
}
static void networkWorker(void*){
  uint32_t last=0;bool initial=true;
  for(;;){
    if(WiFi.status()==WL_CONNECTED&&(initial||netRefresh.exchange(false)||millis()-last>=60000)){
      initial=false;netRefresh=false;
      netPublish(10,tcpProbe("1.1.1.1",443,AF_INET));
      netPublish(11,tcpProbe("2606:4700:4700::1111",443,AF_INET6));
      NetResult d;uint32_t start=millis();IPAddress ip;
      d.state=WiFi.hostByName("example.com",ip)?1:2;d.ms=millis()-start;d.stamp=millis();netPublish(12,d);
      for(int i=0;i<NET_COUNT&&WiFi.status()==WL_CONNECTED;i++){
        if(!targets[i].host[0])continue;
        NetResult n=targets[i].kind==0?pingProbe(targets[i].host):targets[i].kind==1?tcpProbe(targets[i].host,targets[i].port):httpProbe(targets[i].host,targets[i].port);
        netPublish(i,n);vTaskDelay(pdMS_TO_TICKS(50));
      }
      last=millis();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
