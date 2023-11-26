#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "printf.h"
#include "wed.h"
#include "data.h"

RF24 radio(CE_PIN, CSN_PIN); // CE, CSN
WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);

void send2wed(String str){
  if(connected == false)
    return;
  char chr[100];
  str.toCharArray(chr,str.length()+1);
  webSocket.sendTXT(0, chr, strlen(chr));
}
void checkAndReadEeprom(){
  /*
   * [0:3]: relay status
   * [4:7]: relay use?
   * [8]: wed use?
   * [20:24]: Address
   * [25+15*n:25+15+15*n]: Relay name, 15 byte per relay
   */
  bool is_commit = false;
  for(uint8_t i = 0; i < 8; i++){
    if(EEPROM.read(i) > 1){
      EEPROM.write(i,0);
      is_commit = true;
    }
  }
  for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
    for(uint8_t j = 0; j < 15; j++){
      if(EEPROM.read(25+15*i+j) < 32 || EEPROM.read(25+15*i+j) >= 127){
        EEPROM.write(25+15*i+j,0);
        is_commit = true;
      }
    }
  }
  if(is_commit){
    EEPROM.commit();
  }

  /* Read last relay status */
  for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
    relay_status[i] = EEPROM.read(i);
    relay_enable[i] = EEPROM.read(i+NUMBER_OF_RELAY);
    digitalWrite(RELAY_PIN[i], relay_status[i]);
  }
}
bool send_nRF(String stringSend, int times = 1){
  radio.stopListening();
  radio.powerUp();delay(5);
  char text[32] = {NULL};
  stringSend.toCharArray(text,stringSend.length()+1);
  while(--times >= 0){
    if(radio.write(&text, sizeof(text))){
      radio.startListening();
      return true;
    }
    delay(10);
  }
  radio.startListening();
  return false;
}
void control_relay(uint8_t relay, int8_t value){
  if(relay >= NUMBER_OF_RELAY){
    return;
  }
  if(value >= 0){
    relay_status[relay] = value;
  }
  else{
    relay_status[relay] = !relay_status[relay];
  }
  Serial.println("Relay " + String(relay) + " to " + String(relay_status[relay]));
  digitalWrite(RELAY_PIN[relay], relay_status[relay]);
  EEPROM.write(relay, relay_status[relay]);
  EEPROM.commit();
}
void read_nRF(){
  if(radio.available()){
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
    if(strcmp(text,last_message) != 0){
      strcpy(last_message,text);
      String str = String(text);
      send2wed(str);
      char name[15];
      uint8_t k = 0;
      while(k < str.length()){
        if(str.charAt(k) == '%'){
          break;
        }
        name[k] = str.charAt(k);
        k++;
      }
      name[k] = '\0';
      for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
        if(strcmp(name, relay_name[i]) == 0){
          if(str.indexOf("%0") >= 0){
            control_relay(i, 0);
          }
          else if(str.indexOf("%1") >= 0){
            control_relay(i, 1);
          }
          else{
            control_relay(i, -1);
          }
          
          if(relay_status[i] == 0){
            send2wed("OFF" + String(i+1));
          }
          else{
            send2wed("ON" + String(i+1));
          }
        }
      }
      timeToClearMessage = millis()+2000;
    }
  }
}
void readButton(){
  for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
    if(relay_enable[i] == 1){
      uint16_t touch = touchRead(BUTTON_PIN[i]);
      if((button_status[i] == 0 && touch < 35) || (button_status[i] == 1 && touch >= 35)){
        uint8_t t = 0;
        uint8_t times = 0;
        while(t < 10){
          touch = touchRead(BUTTON_PIN[i]);
          if((button_status[i] == 0 && touch < 35) || (button_status[i] == 1 && touch >= 35)){
            times++;
          }
          delay(2);
          t++;
        }
        if(times >= 7){
          button_status[i] = !button_status[i];
          if(button_status[i] == 1){
            time_holdEvent[i] = millis() + 3000;
          }
          else if(button_hold[i] == 0){
            time_holdEvent[i] = -1;
            control_relay(i, -1);
          }
          else{
            send2wed("Restart");
            button_hold[i] = 0;
            EEPROM.write(8,1);
            EEPROM.commit();
            gpio_hold_en(GPIO_NUM_32);
            gpio_hold_en(GPIO_NUM_25);
            gpio_hold_en(GPIO_NUM_12);
            gpio_hold_en(GPIO_NUM_22);
            delay(10);
            ESP.restart();
          }
        }
      }
    }
    if(relay_enable[i] == 1){
      if(millis() >= time_holdEvent[i]){
        time_holdEvent[i] = -1;
        button_hold[i] = 1;
      }
    }
  }
}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT && num == 0){
    if(payload[0] != '#'){
      String dataRecive = "";
      for(int i = 0; i < length; i++){
        dataRecive += (char) payload[i];
      }
      if(dataRecive.indexOf("USE") == 0){
        uint8_t index = (int)dataRecive.charAt(3) - 48 - 1;
        if(index < NUMBER_OF_RELAY){
          relay_enable[index] = !relay_enable[index];
        }
        String str = "USE%";
        for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
          str += relay_enable[i];
        }
        send2wed(str);
      }
      else if(dataRecive.indexOf("Done") == 0){
        gpio_hold_en(GPIO_NUM_32);
        gpio_hold_en(GPIO_NUM_25);
        gpio_hold_en(GPIO_NUM_12);
        gpio_hold_en(GPIO_NUM_22);
        delay(10);
        ESP.restart();
      }
      else if(dataRecive.indexOf("ON") == 0){
        uint8_t index = (int)dataRecive.charAt(2) - 48 - 1;
        if(index < NUMBER_OF_RELAY && relay_enable[index] == 1){
          control_relay(index, 1);
        }
      }
      if(dataRecive.indexOf("OFF") == 0){
        uint8_t index = (int)dataRecive.charAt(3) - 48 - 1;
        if(index < NUMBER_OF_RELAY && relay_enable[index] == 1){
          control_relay(index, 0);
        }
      }
      else if(dataRecive.indexOf("IN4") == 0){
        Serial.println(dataRecive);
        String str = "";
        uint8_t i = 4, j = 0;
        while(i < dataRecive.length()){
          if(dataRecive.charAt(i) != '%'){
            str += dataRecive.charAt(i);
          }
          else{
            if(j == 0){ // Address
              char temp[3] = "\0\0";
              for(uint8_t i = 0, k = 4; i < str.length(); i++){
                temp[0] = str.charAt(i);
                i++;
                temp[1] = str.charAt(i);
                address[k] = strtol(temp, 0, 16);
                k--;
              }
              radio.openWritingPipe(address);
              radio.openReadingPipe(1, address);
            }
            else{ // Relay name
              str.toCharArray(relay_name[j-1], str.length()+1);
            }
            str = "";
            j++;
          }
          i++;
        }
        for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
          EEPROM.write(i+NUMBER_OF_RELAY, relay_enable[i]);
        }
        for(uint8_t i = 0; i < 5; i++){
          EEPROM.write(i+20, address[i]);
        }
        
        for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
          uint8_t j = 0;
          for(j = 0; j < strlen(relay_name[i]) && j < 14; j++){
            EEPROM.write(25+15*i+j, (int)relay_name[i][j]);
          }
          EEPROM.write(25+15*i+j, 0);
        }
        EEPROM.commit();
        send2wed("Saved");
      }
    }
  }
  else if(type == WStype_CONNECTED){
    Serial.printf("[%u] Connected\n", num);
    connected = true;
    String str = "USE%";
    for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
      str += relay_enable[i];
    }
    send2wed(str);
    delay(10);
    char ch[3];
    str = "IN4%";
    for(int8_t i = 4; i >= 0; i--){
      sprintf(ch,"%02X",address[i]);
      str += String(ch);
    }
    str += "%";
    for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
      bool is_ok = true;
      for(uint8_t j = 0; j < 15; j++){
        if(int(relay_name[i][j]) == 0){
          break;
        }
        if(int(relay_name[i][j]) < 32 ||  int(relay_name[i][j]) >= 127){
          is_ok = false;
        }
      }
      if(is_ok){
        str += String(relay_name[i]) + "%";
      }
      else{
        str += "%";
      }
    }
    
    send2wed(str);
    delay(10);
    if(is_nRF_ok){
      send2wed("nRF is OK!");
    }
    else{
      send2wed("nRF is BAD!");
    }
    delay(10);
    send2wed(WiFi.localIP().toString());
    delay(10);
    send2wed("V1.0");
  }
  else if(type == WStype_DISCONNECTED){
    connected = false;
    Serial.printf("[%u] Disconnected!\n", num);
  }
}
void interruptHandler() {
  irq_status = true;
  delayMicroseconds(250);
  bool tx_ds, tx_df, rx_dr;                       // declare variables for IRQ masks
  radio.whatHappened(tx_ds, tx_df, rx_dr);        // get values for IRQ masks
}
void setup() {
  Serial.begin(115200);
  printf_begin();
  EEPROM.begin(120);
  pinMode(IRQ_PIN, INPUT);
  checkAndReadEeprom();

  bool is_ok = false;
  for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
    if(EEPROM.read(i + 4) == 1){
      is_ok = true;
    }
  }
  gpio_hold_dis(GPIO_NUM_32);
  gpio_hold_dis(GPIO_NUM_25);
  gpio_hold_dis(GPIO_NUM_12);
  gpio_hold_dis(GPIO_NUM_22);
  
  if(EEPROM.read(8) == 1 || is_ok == false){
    Serial.println("Wed enable");
    is_useWed = true;

    WiFi.begin("Lightning", "12345678");
    uint8_t i = 0;
    while (WiFi.status() != WL_CONNECTED) {
      if(millis() > 10000){
        WiFi.softAP(ssid, pass);
      }
    }
    Serial.println(WiFi.localIP());
    server.on("/",[](){
      server.send_P(200, "text/html", GiamSat);
    });
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      MDNS.begin(host);
      server.on("/server", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
      });
      server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
      }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.setDebugOutput(true);
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin()) { //start with max available size
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(true)) { //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        } else {
          Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
          ESP.restart();
        }
      });
      Serial.printf("Ready! Open http://%s.local in your browser\n", host);
    }
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    MDNS.addService("http", "tcp", 80);
    EEPROM.write(8,0);
    EEPROM.commit();
  }

  if(is_useWed == false){
    setCpuFrequencyMhz(80);
  }

  for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
    pinMode(RELAY_PIN[i], OUTPUT);
  }
  
  for(uint8_t i = 0; i < NUMBER_OF_RELAY; i++){
    for(uint8_t j = 0; j < 15; j++){
      if(EEPROM.read(25+15*i+j) == 0){
        break;
      }
      relay_name[i][j] = EEPROM.read(25+15*i+j);
    }
  }
  for(uint8_t i = 0; i < 5; i++){
    address[i] = EEPROM.read(20+i);
  }
  
  if(radio.begin()){
    Serial.println("nRF is OK!");
    is_nRF_ok = true;
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_2MBPS);
    radio.openWritingPipe(address);
    radio.openReadingPipe(1, address);
    radio.setPALevel(RF24_PA_MAX);
    radio.printDetails();
    if(is_useWed == false){
      attachInterrupt(digitalPinToInterrupt(IRQ_PIN), interruptHandler, FALLING);
      radio.enableDynamicPayloads();
      radio.enableAckPayload();
      radio.maskIRQ(0, 0, 0); // args = "data_sent", "data_fail", "data_ready"
    }
    radio.startListening();
  }
  else{
    is_nRF_ok = false;
    Serial.println("nRF is BAD!");
  }
}
void loop() {
  if(is_useWed){
    webSocket.loop();
    server.handleClient();
    
    if(is_nRF_ok == true){
      read_nRF();
    }

    if(millis() >= time_runing){
      send2wed("TM" + String(time_runing/1000));
      time_runing = millis() + 1000;
    }
  }
  else{
    if(irq_status){
      read_nRF();
      irq_status = false;
    }
  }
  readButton();
  if(millis() >= timeToClearMessage){
    strcpy(last_message,"");
    timeToClearMessage = -1;
  }
  if(millis() >= 10*DAY){
    gpio_hold_en(GPIO_NUM_32);
    gpio_hold_en(GPIO_NUM_25);
    gpio_hold_en(GPIO_NUM_12);
    gpio_hold_en(GPIO_NUM_22);
    delay(10);
    ESP.restart();
  }
}
