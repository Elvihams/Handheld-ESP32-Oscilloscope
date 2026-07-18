#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite graph = TFT_eSprite(&tft); 

// ==========================================
// 1. ĐỊNH NGHĨA CHÂN & MÀU SẮC
// ==========================================
#define BTN_UP    32
#define BTN_DOWN  25
#define BTN_OK    26
#define BTN_HOLD  27
#define PIN_ADC   33

#define GRID_COLOR 0x2965 
#define WAVE_COLOR TFT_YELLOW
#define BG_COLOR   0x0005 

// ==========================================
// 2. BIẾN TOÀN CỤC & TRẠNG THÁI
// ==========================================
enum SystemMode { MODE_TIME_DIV = 0, MODE_VOLT_DIV = 1, MODE_Y_SHIFT = 2, MODE_X_SHIFT = 3 };
SystemMode currentMode = MODE_TIME_DIV;
bool isRunning = true;
bool isCalib = true;

int timeDivs[] = {5, 10, 50, 100, 250, 500, 1000, 2000}; 
int timeIndex = 3; 
float voltDivs[] = {0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 3.3}; 
int voltIndex = 5; 

float yOffset = 0.0; 
int xOffset = 0;     
float bias = 1.65; 

#define SCREEN_WIDTH 320
#define ADC_BUF_LEN  400 
uint16_t adcBuffer[ADC_BUF_LEN];

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 150;

// ==========================================
// 3. CẤU HÌNH WI-FI AP & WEB SERVER
// ==========================================
const char* ssid = "IC_Scope_AP";
const char* password = "admin123"; 

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Giao Diện Web (Giữ nguyên bản Cyberpunk siêu đẹp)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Web Oscilloscope</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
body { 
      /* Dải gradient xanh tím ngân hà */
      background: linear-gradient(120deg, #290a59 0%, #3d5af1 100%);
      color: #e0f7fa; 
      font-family: 'Segoe UI', sans-serif; 
      display: flex; 
      flex-direction: row; 
      justify-content: center; 
      align-items: center; 
      min-height: 100vh; 
      margin: 0; 
      padding: 20px; 
      box-sizing: border-box; 
    }
.container { 
      display: flex; 
      flex-direction: row; 
      gap: 25px; 
      /* Nền kính mờ bo góc cho bảng điều khiển */
      background: rgba(15, 12, 41, 0.55); 
      padding: 30px; 
      border-radius: 16px; 
      box-shadow: 0 15px 35px rgba(0, 0, 0, 0.6), inset 0 0 10px rgba(255, 255, 255, 0.1); 
      backdrop-filter: blur(12px); 
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid rgba(255, 255, 255, 0.08);
    }    #sidebar { display: flex; flex-direction: column; gap: 15px; width: 220px; justify-content: center;}
    .badge { padding: 15px; border-radius: 12px; font-weight: bold; font-size: 14px; box-shadow: 0 4px 15px rgba(0,0,0,0.2); background: rgba(20, 30, 50, 0.7); border: 1px solid rgba(255, 255, 255, 0.1); }
    .bg-blue { border-left: 5px solid #00e5ff; }
    .bg-green { border-left: 5px solid #39ff14; }
    .bg-red { border-left: 5px solid #ff073a; }
    canvas { background: #020205; border: 2px solid rgba(255, 255, 255, 0.1); border-radius: 12px; box-shadow: 0 0 25px rgba(0, 229, 255, 0.2); }
    .ctrl-row { display: flex; justify-content: space-between; align-items: center; margin-top: 10px;}
    .val { color: #fff; font-family: monospace; font-size: 16px; font-weight: bold; letter-spacing: 1px;}
    button { background: rgba(255, 255, 255, 0.1); color: #fff; border: 1px solid rgba(255,255,255,0.3); border-radius: 6px; cursor: pointer; padding: 4px 12px; font-weight: bold; transition: all 0.2s ease-in-out; }
    button:hover { background: rgba(255, 255, 255, 0.3); transform: translateY(-2px);}
    .btn-run { width: 100%; padding: 12px; margin-top: 15px; background: rgba(57, 255, 20, 0.1); border: 1px solid #39ff14; color: #39ff14; text-transform: uppercase; letter-spacing: 2px; }
    .btn-run:hover { background: rgba(57, 255, 20, 0.3); box-shadow: 0 0 15px rgba(57, 255, 20, 0.4);}
    .btn-run:active { transform: scale(0.95); }
  </style>
</head>
<body>
  <div class="container">
    <div id="sidebar">
      <div class="badge bg-blue">
        <div class="ctrl-row">Vpp: <span id="vpp" class="val">0.0V</span></div>
        <div class="ctrl-row"><span>Y-Sft: <span id="ysft" class="val">0.0V</span></span><div><button onclick="sendCmd('Y_UP')">&#9650;</button> <button onclick="sendCmd('Y_DOWN')">&#9660;</button></div></div>
      </div>
      <div class="badge bg-green">
        <div class="ctrl-row">F: <span id="freq" class="val">0Hz</span></div>
        <div class="ctrl-row">Max: <span id="vmax" class="val">0.0V</span></div>
        <button class="btn-run" onclick="sendCmd('RUN_TOGGLE')">RUN / STOP</button>
      </div>
      <div class="badge bg-red">
        <div class="ctrl-row"><span id="time" class="val">100us/d</span><div><button onclick="sendCmd('TIME_UP')">&#9650;</button> <button onclick="sendCmd('TIME_DOWN')">&#9660;</button></div></div>
        <div class="ctrl-row" style="margin-top: 15px;"><span>V/d: <span id="volt" class="val">1.0</span></span><div><button onclick="sendCmd('VOLT_UP')">&#9650;</button> <button onclick="sendCmd('VOLT_DOWN')">&#9660;</button></div></div>
      </div>
    </div>
    <canvas id="scope" width="640" height="480"></canvas>
  </div>
  <script>
    var canvas = document.getElementById('scope');
    var ctx = canvas.getContext('2d');
    var websocket = new WebSocket('ws://' + window.location.hostname + '/ws');
    function sendCmd(cmd) { websocket.send(cmd); }
    websocket.onmessage = function(event) {
      var parts = event.data.split('|');
      if(parts.length < 9) return;
      document.getElementById('vmax').innerText = parseFloat(parts[0]).toFixed(1) + "V";
      document.getElementById('vpp').innerText = parseFloat(parts[2]).toFixed(1) + "V";
      document.getElementById('freq').innerText = parts[3] + "Hz";
      document.getElementById('time').innerText = parts[4] + "us/d";
      document.getElementById('volt').innerText = parts[5];
      document.getElementById('ysft').innerText = parseFloat(parts[6]).toFixed(1) + "V";
      
      var voltDiv = parseFloat(parts[5]), yOffset = parseFloat(parts[6]), bias = parseFloat(parts[7]), adc = parts[8].split(',');
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      
      ctx.strokeStyle = '#1c2833'; ctx.lineWidth = 1; ctx.beginPath();
      for(var i=0; i<=640; i+=64) { ctx.moveTo(i,0); ctx.lineTo(i,480); }
      for(var j=0; j<=480; j+=60) { ctx.moveTo(0,j); ctx.lineTo(640,j); }
      ctx.stroke();

      ctx.strokeStyle = '#45a29e'; ctx.lineWidth = 1.5; ctx.beginPath(); 
      ctx.moveTo(320, 0); ctx.lineTo(320, 480); ctx.moveTo(0, 240); ctx.lineTo(640, 240); ctx.stroke();

      ctx.strokeStyle = '#ffff00'; ctx.lineWidth = 2.5;
      var maxVal = 0, minVal = 4095, maxIdx = 0, minIdx = 0;
      ctx.beginPath();
      for(var i=0; i<adc.length; i++) {
        var raw_val = parseInt(adc[i]);
        if(raw_val > maxVal) { maxVal = raw_val; maxIdx = i; }
        if(raw_val < minVal) { minVal = raw_val; minIdx = i; }
        var x = i * 2, v_signal = (raw_val / 4095.0) * 3.3 - bias, y = 240 - ((v_signal + yOffset) / voltDiv) * 60; 
        if(i==0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();

      var v_max_real = (maxVal / 4095.0) * 3.3, v_min_real = (minVal / 4095.0) * 3.3;
      var maxX = maxIdx * 2, maxY = 240 - (((v_max_real - bias) + yOffset) / voltDiv) * 60;
      var minX = minIdx * 2, minY = 240 - (((v_min_real - bias) + yOffset) / voltDiv) * 60;

      ctx.fillStyle = '#ff073a'; ctx.beginPath(); ctx.arc(maxX, maxY, 6, 0, Math.PI*2); ctx.fill();
      ctx.font = 'bold 16px monospace'; ctx.fillText(v_max_real.toFixed(2) + 'V', maxX + 10, maxY - 10);
      ctx.fillStyle = '#00e5ff'; ctx.beginPath(); ctx.arc(minX, minY, 6, 0, Math.PI*2); ctx.fill();
      ctx.fillText(v_min_real.toFixed(2) + 'V', minX + 10, minY + 20);
    };
  </script>
</body>
</html>
)rawliteral";

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_TEXT) {
      data[len] = 0;
      String cmd = (char*)data;
      if (cmd == "RUN_TOGGLE") isRunning = !isRunning;
      else if (cmd == "TIME_UP" && timeIndex < 7) timeIndex++;
      else if (cmd == "TIME_DOWN" && timeIndex > 0) timeIndex--;
      else if (cmd == "VOLT_UP" && voltIndex < 7) voltIndex++;
      else if (cmd == "VOLT_DOWN" && voltIndex > 0) voltIndex--;
      else if (cmd == "Y_UP") yOffset += 0.5;
      else if (cmd == "Y_DOWN") yOffset -= 0.5;
    }
  }
}

// ==========================================
// 4. HÀM HIỆU CHUẨN (AUTO ZERO)
// ==========================================
void autoCalibrate() {
  tft.fillRect(0, 0, 320, 240, BG_COLOR);
  tft.setTextColor(TFT_WHITE, BG_COLOR);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("AUTO ZERO CALIBRATING...", 160, 120, 2);
  delay(1500); 
  long sum = 0;
  for(int i = 0; i < 200; i++) {
    sum += analogRead(PIN_ADC);
    delay(5);
  }
  float avgADC = sum / 200.0;
  bias = (avgADC / 4095.0) * 3.3; 
  tft.fillScreen(BG_COLOR);
}

// ==========================================
// 5. TASK CORE 0: XỬ LÝ NÚT NHẤN, VẼ UI & GỬI WEB
// ==========================================
void TaskUI(void *pvParameters) {
  unsigned long lastWebSend = 0;
  while (true) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (digitalRead(BTN_HOLD) == LOW) { 
        isRunning = !isRunning; lastDebounceTime = millis(); 
      }
      else if (digitalRead(BTN_OK) == LOW && isRunning) { 
        currentMode = (SystemMode)((currentMode + 1) % 4); lastDebounceTime = millis(); 
      }
      else if (digitalRead(BTN_UP) == LOW && isRunning) {
        if (currentMode == MODE_TIME_DIV && timeIndex < 7) timeIndex++;
        if (currentMode == MODE_VOLT_DIV && voltIndex < 7) voltIndex++;
        if (currentMode == MODE_Y_SHIFT) yOffset += 0.5;
        if (currentMode == MODE_X_SHIFT && xOffset < 80) xOffset += 4;
        lastDebounceTime = millis();
      }
      else if (digitalRead(BTN_DOWN) == LOW && isRunning) {
        if (currentMode == MODE_TIME_DIV && timeIndex > 0) timeIndex--;
        if (currentMode == MODE_VOLT_DIV && voltIndex > 0) voltIndex--;
        if (currentMode == MODE_Y_SHIFT) yOffset -= 0.5;
        if (currentMode == MODE_X_SHIFT && xOffset > 0) xOffset -= 4;
        lastDebounceTime = millis();
      }
    }

    if (isRunning) {
      // 1. Vẽ Nền và Lưới
      graph.fillSprite(BG_COLOR);
      for (int x = 0; x <= 320; x += 32) graph.drawFastVLine(x, 0, 240, GRID_COLOR);
      for (int y = 0; y <= 240; y += 30) graph.drawFastHLine(0, y, 320, GRID_COLOR);
      graph.drawFastVLine(160, 0, 240, 0x4A69); 
      graph.drawFastHLine(0, 120, 320, 0x4A69); 
      
      // 2. Điểm xác định 0V bên trái trục hoành
      int zeroY = 120 - (int)((yOffset / voltDivs[voltIndex]) * 30); 
      if (zeroY >= 0 && zeroY <= 240) {
        graph.fillTriangle(0, zeroY, 8, zeroY - 5, 8, zeroY + 5, TFT_RED);
        graph.setTextColor(TFT_RED);
        graph.setTextDatum(ML_DATUM);
        graph.drawString("0V", 10, zeroY, 1); // Dùng font 1 nhỏ gọn
      }
      
      uint16_t actualMax = 0, actualMin = 4095;
      int maxX = 0, maxY = 0, minX = 0, minY = 0;
      float v_div_factor = voltDivs[voltIndex];

      // 3. Tính toán & Vẽ sóng
      for (int x = 1; x < SCREEN_WIDTH; x++) {
        uint16_t raw_current = adcBuffer[x + xOffset];
        uint16_t raw_prev = adcBuffer[x - 1 + xOffset];
        
        float v_current = (raw_current / 4095.0) * 3.3 - bias;
        float v_prev = (raw_prev / 4095.0) * 3.3 - bias;
        
        int y = 120 - (int)(((v_current + yOffset) / v_div_factor) * 30); 
        int prevY = 120 - (int)(((v_prev + yOffset) / v_div_factor) * 30);
        
        int drawY = constrain(y, 0, 240);
        int drawPrevY = constrain(prevY, 0, 240);
        
        graph.drawLine(x - 1, drawPrevY, x, drawY, WAVE_COLOR);
        
        if (raw_current > actualMax) { actualMax = raw_current; maxX = x; maxY = drawY; }
        if (raw_current < actualMin) { actualMin = raw_current; minX = x; minY = drawY; }
      }
      
      float v_max = (actualMax / 4095.0) * 3.3;
      float v_min = (actualMin / 4095.0) * 3.3;
      
      int crossCount = 0;
      int threshold = (actualMax + actualMin) / 2;
      for (int x = 1; x < SCREEN_WIDTH; x++) {
         if (adcBuffer[x-1 + xOffset] < threshold && adcBuffer[x + xOffset] >= threshold) crossCount++;
      }
      float screenTimeUs = SCREEN_WIDTH * timeDivs[timeIndex];
      int realFreq = (int)((crossCount / screenTimeUs) * 1000000.0);

      // 4. Vẽ Chấm Cực Trị và Giá trị điện áp kèm theo (Font 1)
      graph.fillCircle(maxX, maxY, 3, TFT_RED);
      graph.setTextColor(TFT_RED);
      graph.setTextDatum(BL_DATUM);
      graph.drawString(String(v_max, 2) + "V", maxX + 5, constrain(maxY - 5, 25, 240), 1);

      graph.fillCircle(minX, minY, 3, TFT_CYAN);
      graph.setTextColor(TFT_CYAN);
      graph.setTextDatum(TL_DATUM);
      graph.drawString(String(v_min, 2) + "V", minX + 5, constrain(minY + 5, 0, 215), 1);

      // ==========================================
      // 5. VẼ KHUNG OSD MỎNG 1 HÀNG BÊN TRÊN (TOP BAR)
      // ==========================================
      graph.fillRect(0, 0, 320, 18, TFT_BLACK); 
      graph.drawFastHLine(0, 17, 320, TFT_DARKGREY); 
      
      graph.setTextDatum(TL_DATUM);
      // V/d (Sáng đỏ nếu đang chọn)
      graph.setTextColor((currentMode == MODE_VOLT_DIV) ? TFT_RED : TFT_CYAN, TFT_BLACK); 
      graph.drawString(String(voltDivs[voltIndex], 1) + "V/d", 2, 1, 2);
      
      // T/d (Sáng đỏ nếu đang chọn)
      graph.setTextColor((currentMode == MODE_TIME_DIV) ? TFT_RED : TFT_CYAN, TFT_BLACK); 
      graph.drawString(String(timeDivs[timeIndex]) + "us", 60, 1, 2);
      
      // VPP và Freq
      graph.setTextColor(TFT_MAGENTA, TFT_BLACK); 
      graph.drawString("VPP:" + String(v_max - v_min, 1), 125, 1, 2);
      
      graph.setTextColor(TFT_WHITE, TFT_BLACK); 
      graph.drawString("F:" + String(realFreq), 205, 1, 2);
      
      // Chữ [ RUN ] căn sát mép phải
      graph.setTextDatum(TR_DATUM);
      graph.setTextColor(TFT_GREEN, TFT_BLACK); 
      graph.drawString("[RUN]", 318, 1, 2); 

      // ==========================================
      // 6. VẼ KHUNG OSD MỎNG 1 HÀNG BÊN DƯỚI (BOTTOM BAR)
      // ==========================================
      graph.fillRect(0, 222, 320, 18, TFT_BLACK); 
      graph.drawFastHLine(0, 222, 320, TFT_DARKGREY); 
      
      graph.setTextDatum(TL_DATUM);
      graph.setTextColor(TFT_WHITE, TFT_BLACK); 
      graph.drawString("M:" + String(v_max, 1), 2, 224, 2);
      graph.drawString("m:" + String(v_min, 1), 55, 224, 2);
      
      graph.setTextColor(TFT_YELLOW, TFT_BLACK); 
      graph.drawString("B:" + String(bias, 2), 110, 224, 2);
      
      uint16_t sftColor = (currentMode == MODE_Y_SHIFT || currentMode == MODE_X_SHIFT) ? TFT_RED : TFT_CYAN;
      graph.setTextColor(sftColor, TFT_BLACK); 
      graph.drawString("SFT:Y" + String(yOffset, 1) + " X" + String(xOffset), 175, 224, 2);

      // Xuất Sprite ra màn hình
      graph.pushSprite(0, 0);

      // Đóng gói gửi Web
      if (millis() - lastWebSend > 100) {
        String payload;
        payload.reserve(3000); 
        payload += String(v_max, 2) + "|" + String(v_min, 2) + "|" + String(v_max - v_min, 2) + "|" 
                + String(realFreq) + "|" + String(timeDivs[timeIndex]) + "|" + String(voltDivs[voltIndex], 1) 
                + "|" + String(yOffset, 2) + "|" + String(bias, 2) + "|";
        for (int i = 0; i < SCREEN_WIDTH; i++) {
          payload += String(adcBuffer[i + xOffset]);
          if (i < SCREEN_WIDTH - 1) payload += ",";
        }
        ws.textAll(payload);
        lastWebSend = millis();
      }
    } else {
      // Khi nhấn STOP, chỉ xóa và vẽ lại chữ [STOP] trên góc phải
      graph.fillRect(270, 0, 50, 18, TFT_BLACK); 
      graph.setTextDatum(TR_DATUM);
      graph.setTextColor(TFT_RED, TFT_BLACK); 
      graph.drawString("[STOP]", 318, 1, 2);
      graph.pushSprite(0, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(40)); 
  }
}

// ==========================================
// 6. TASK CORE 1: ĐỌC ADC TỐC ĐỘ CAO
// ==========================================
void TaskADC(void *pvParameters) {
  while (true) {
    if (isRunning) {
      int triggerTimeout = 0;
      int prevVal = analogRead(PIN_ADC);
      int triggerLevel = (int)((bias / 3.3) * 4095);
      
      while (triggerTimeout < 5000) {
        int currentVal = analogRead(PIN_ADC);
        if (prevVal < triggerLevel && currentVal >= triggerLevel) break; 
        prevVal = currentVal;
        triggerTimeout++;
      }
      
      for(int i = 0; i < ADC_BUF_LEN; i++) {
        adcBuffer[i] = analogRead(PIN_ADC);
        delayMicroseconds(timeDivs[timeIndex]); 
        if (i % 64 == 0) yield(); 
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}

// ==========================================
// 7. SETUP TỔNG
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000); 
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_HOLD, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);
  
  graph.setColorDepth(8);
  graph.createSprite(320, 240); 
  
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAP(ssid, password); 
  
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.begin();

  autoCalibrate();

  xTaskCreatePinnedToCore(TaskUI, "UI Task", 10000, NULL, 1, NULL, 0); 
  xTaskCreatePinnedToCore(TaskADC, "ADC Task", 10000, NULL, 2, NULL, 1); 
}
void loop() {
  ws.cleanupClients(); 
  vTaskDelay(pdMS_TO_TICKS(100)); 
}