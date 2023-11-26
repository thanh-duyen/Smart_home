#define IRQ_PIN 36
#define NUMBER_OF_RELAY 4
#define CE_PIN 17
#define CSN_PIN 16
#define DAY 86400000
const uint8_t RELAY_PIN[4] = {32,25,12,22};
const uint8_t BUTTON_PIN[4] = {T7,T6,T0,T0};
bool relay_enable[4] = {1,1,0,0};
bool button_status[4] = {0,0,0,0};
bool relay_status[4] = {0,0,0,0};
uint32_t time_holdEvent[4] = {-1,-1,-1,-1};
bool button_hold[4] = {0,0,0,0};
bool irq_status = false; // read nRF when wake up
bool is_useWed = false;
char last_message[32] = "";
uint32_t timeToClearMessage = -1;
bool is_nRF_ok;
const char ssid[30] = "Lighning";
const char pass[30] = "12345678";
char relay_name[4][16];
uint8_t address[6];
uint32_t time_runing;
bool connected = false;
const char* host = "Lighting";
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";
