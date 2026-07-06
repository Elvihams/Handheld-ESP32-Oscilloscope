#include <SPI.h>
#include <TFT_eSPI.h> 

TFT_eSPI tft = TFT_eSPI();

// --- CẤU HÌNH CHÂN NÚT NHẤN & ADC ---
#define ADC_PIN 33
#define BTN_UP 32
#define BTN_DOWN 25
#define BTN_OK 26
#define BTN_HOLD 27 

#define BTN_PRESSED LOW 

// --- QUY HOẠCH KHÔNG GIAN MÀN HÌNH ---
#define DISP_WIDTH 320
#define DISP_HEIGHT 240
#define TOP_BAR_H 16      // Thanh trạng thái trên
#define BOT_BAR_H 20      // Thanh menu dưới
#define GRAPH_TOP 16      
#define GRAPH_BOT 220     
#define Y_CENTER 118      

#define MAX_ADC 4095

uint16_t waveBuffer[DISP_WIDTH];
uint8_t oldYBuffer[DISP_WIDTH]; 

// Lưu tọa độ cũ để xóa nét chống nháy
int oldMaxY = -1;
int oldMinY = -1;

bool isHolding = false;
int zeroOffset = 0;       

// --- BIẾN ĐIỀU KHIỂN MENU ---
int menuFocus = 0;        // 0: Nguồn, 1: Tốc độ, 2: Zoom, 3: Y-Pos
int signalSource = 0;     // 0: EXT, 1: SINE, 2: SQUR, 3: TRIA
int timeBase = 50;      
float vScale = 1.0;     
int yOffset = 0;        
int sampleOffset = 0;   

void drawGrid();
void drawTopBar(float vPp);
void drawBottomMenu();

void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_HOLD, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT);

  tft.init();
  tft.setRotation(1); 
  tft.fillScreen(TFT_BLACK);
  
  // --- HIỆU CHUẨN AUTO-ZERO ---
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(60, 110);
  tft.print("CALIBRATING AUTO-ZERO...");

  long sum = 0;
  for(int i = 0; i < 200; i++) {
    sum += analogRead(ADC_PIN);
    delay(2);
  }
  zeroOffset = 2048 - (sum / 200); 
  
  for(int i=0; i<DISP_WIDTH; i++) oldYBuffer[i] = Y_CENTER;
  
  tft.fillScreen(TFT_BLACK); 
  drawBottomMenu(); // Vẽ Menu đáy lần đầu
}

void loop() {
  // 1. --- XỬ LÝ NÚT NHẤN MENU ---
  if (digitalRead(BTN_HOLD) == BTN_PRESSED) {
    isHolding = !isHolding;
    delay(300); 
  }
  
  bool menuChanged = false;
  if (digitalRead(BTN_OK) == BTN_PRESSED) {
    menuFocus = (menuFocus + 1) % 4; 
    menuChanged = true;
    delay(250);
  }
  
  if (digitalRead(BTN_UP) == BTN_PRESSED) {
    if (menuFocus == 0) { signalSource = (signalSource + 1) % 4; } 
    else if (menuFocus == 1) { timeBase += 10; }
    else if (menuFocus == 2) { vScale += 0.5; if(vScale > 5.0) vScale = 5.0; } 
    else if (menuFocus == 3) { yOffset -= 10; } 
    menuChanged = true;
    delay(100);
  }
  
  if (digitalRead(BTN_DOWN) == BTN_PRESSED) {
    if (menuFocus == 0) { signalSource--; if(signalSource < 0) signalSource = 3; }
    else if (menuFocus == 1) { timeBase -= 10; if(timeBase < 10) timeBase = 10; }
    else if (menuFocus == 2) { vScale -= 0.5; if(vScale < 0.5) vScale = 0.5; } 
    else if (menuFocus == 3) { yOffset += 10; } 
    menuChanged = true;
    delay(100);
  }

  if(menuChanged) drawBottomMenu(); // Chỉ update UI đáy khi có bấm nút

  // 2. --- XỬ LÝ SÓNG ---
  if (!isHolding) {
    int maxADC = 0;
    int minADC = 4095;

    for (int i = 0; i < DISP_WIDTH; i++) {
      int rawVal = 0;
      if (signalSource == 0) {
        rawVal = analogRead(ADC_PIN) + zeroOffset; 
      } 
      else if (signalSource == 1) { rawVal = 2048 + 1400 * sin((i + sampleOffset) * 0.08); } 
      else if (signalSource == 2) { rawVal = ((i + sampleOffset) % 60 < 30) ? 3500 : 600; } 
      else if (signalSource == 3) {
        int mod = (i + sampleOffset) % 60;
        if (mod < 30) rawVal = 600 + mod * 96; else rawVal = 3500 - (mod - 30) * 96;
      }
      
      waveBuffer[i] = constrain(rawVal, 0, MAX_ADC);
      
      // Thu thập Max/Min ADC
      if(waveBuffer[i] > maxADC) maxADC = waveBuffer[i];
      if(waveBuffer[i] < minADC) minADC = waveBuffer[i];

      if (signalSource == 0) delayMicroseconds(timeBase); 
    }

    if (signalSource != 0) {
      sampleOffset += 3;
      if(sampleOffset > 600) sampleOffset = 0;
    }

    // TÍNH TOÁN ĐIỆN ÁP THỰC TẾ
    float vMax = ((maxADC - 2048) / 2048.0) * 1.65;
    float vMin = ((minADC - 2048) / 2048.0) * 1.65;
    float vPp = vMax - vMin;

    // 3. --- ĐỒ HỌA (XÓA CŨ -> VẼ MỚI) ---
    // Xóa chữ text tag cũ của Max/Min (Dùng ô đen che lại)
    if(oldMaxY != -1) tft.fillRect(250, oldMaxY - 10, 70, 10, TFT_BLACK);
    if(oldMinY != -1) tft.fillRect(250, oldMinY + 2, 70, 10, TFT_BLACK);

    // Xóa vạch Cursor cũ và Sóng cũ
    if (oldMaxY != -1) tft.drawFastHLine(0, oldMaxY, DISP_WIDTH, TFT_BLACK);
    if (oldMinY != -1) tft.drawFastHLine(0, oldMinY, DISP_WIDTH, TFT_BLACK);
    for (int i = 0; i < DISP_WIDTH - 1; i++) {
      tft.drawLine(i, oldYBuffer[i], i + 1, oldYBuffer[i+1], TFT_BLACK);
    }

    // Vẽ lại khung lưới
    drawGrid(); 
    drawTopBar(vPp);

    // Vẽ sóng mới
    for (int i = 0; i < DISP_WIDTH - 1; i++) {
      int rawY1 = map(waveBuffer[i], 0, MAX_ADC, GRAPH_BOT, GRAPH_TOP);
      int rawY2 = map(waveBuffer[i+1], 0, MAX_ADC, GRAPH_BOT, GRAPH_TOP);
      
      int y1 = constrain(Y_CENTER + (rawY1 - Y_CENTER) * vScale + yOffset, GRAPH_TOP, GRAPH_BOT);
      int y2 = constrain(Y_CENTER + (rawY2 - Y_CENTER) * vScale + yOffset, GRAPH_TOP, GRAPH_BOT);

      tft.drawLine(i, y1, i + 1, y2, TFT_YELLOW); 
      oldYBuffer[i] = y1; 
    }

    // Tính tọa độ Y của vạch Max/Min
    int rawMaxY = map(maxADC, 0, MAX_ADC, GRAPH_BOT, GRAPH_TOP);
    int rawMinY = map(minADC, 0, MAX_ADC, GRAPH_BOT, GRAPH_TOP);
    int cursorMaxY = constrain(Y_CENTER + (rawMaxY - Y_CENTER) * vScale + yOffset, GRAPH_TOP, GRAPH_BOT);
    int cursorMinY = constrain(Y_CENTER + (rawMinY - Y_CENTER) * vScale + yOffset, GRAPH_TOP, GRAPH_BOT);

    // VẼ VẠCH CỰC TRỊ VÀ NHÃN ĐIỆN ÁP (NGAY TẠI CHỖ)
    tft.setTextSize(1);
    
    // Vạch MAX (Màu Hồng giống CH3)
    tft.drawFastHLine(0, cursorMaxY, DISP_WIDTH, TFT_MAGENTA);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.setCursor(250, cursorMaxY - 10); // Hiển thị nhãn ngay trên vạch
    tft.print(vMax, 2); tft.print("V");

    // Vạch MIN (Màu Xanh lợt giống CH2)
    tft.drawFastHLine(0, cursorMinY, DISP_WIDTH, TFT_CYAN);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(250, cursorMinY + 2); // Hiển thị nhãn ngay dưới vạch
    tft.print(vMin, 2); tft.print("V");

    oldMaxY = cursorMaxY;
    oldMinY = cursorMinY;
  }
}

// --- VẼ LƯỚI TỌA ĐỘ VÀ ĐIỂM 0V CỐ ĐỊNH TỪ BÊN TRÁI ---
void drawGrid() {
  // Lưới nét đứt mô phỏng (khoảng cách thưa để gọn mắt)
  for (int i = 0; i < DISP_WIDTH; i += 40) {
    for (int j = GRAPH_TOP; j < GRAPH_BOT; j += 5) { tft.drawPixel(i, j, TFT_DARKGREY); }
  }
  for (int j = GRAPH_TOP; j < GRAPH_BOT; j += 40) {
    for (int i = 0; i < DISP_WIDTH; i += 5) { tft.drawPixel(i, j, TFT_DARKGREY); }
  }
  
  // Trục hoành chính (0V Reference) dịch chuyển theo yOffset
  int zeroLineY = Y_CENTER + yOffset;
  if(zeroLineY >= GRAPH_TOP && zeroLineY <= GRAPH_BOT) {
    tft.drawFastHLine(0, zeroLineY, DISP_WIDTH, TFT_GREEN);
    
    // NHÃN "0V" CHỐT CỨNG MÉP TRÁI
    tft.setTextSize(1);
    tft.setTextColor(TFT_BLACK, TFT_GREEN); // Chữ đen nền xanh lá nổi bật
    tft.setCursor(0, zeroLineY - 8);
    tft.print(" 0V ");
  }
  tft.drawFastVLine(DISP_WIDTH/2, GRAPH_TOP, GRAPH_BOT - GRAPH_TOP, TFT_GREEN);
}

// --- THANH TRẠNG THÁI TRÊN CÙNG ---
void drawTopBar(float vPp) {
  tft.fillRect(0, 0, DISP_WIDTH, GRAPH_TOP, TFT_BLACK);
  tft.setTextSize(1);
  
  if(isHolding) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 4); tft.print("STOP");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(5, 4); tft.print("RUN");
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(60, 4); tft.print("CALIB: "); tft.print(zeroOffset);
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(240, 4); tft.print("VPP: "); tft.print(vPp, 2); tft.print("V");
}

// --- THANH MENU ĐÁY (STYLE OSCILLOSCOPE CHUYÊN NGHIỆP) ---
void drawBottomMenu() {
  tft.fillRect(0, GRAPH_BOT, DISP_WIDTH, BOT_BAR_H, TFT_BLACK);
  tft.setTextSize(1);
  
  // Kênh 1: SRC (Màu Vàng)
  if(menuFocus == 0) tft.setTextColor(TFT_BLACK, TFT_YELLOW); else tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(5, GRAPH_BOT + 6);
  if(signalSource == 0) tft.print(" CH1:EXT ");
  else if(signalSource == 1) tft.print(" CH1:SIN ");
  else if(signalSource == 2) tft.print(" CH1:SQU ");
  else if(signalSource == 3) tft.print(" CH1:TRI ");

  // Kênh 2: TDIV (Màu Xanh Cyan)
  if(menuFocus == 1) tft.setTextColor(TFT_BLACK, TFT_CYAN); else tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(85, GRAPH_BOT + 6);
  tft.print(" T:"); tft.print(timeBase * 40 / 1000.0, 1); tft.print("ms ");

  // Kênh 3: ZOOM (Màu Tím Magenta)
  if(menuFocus == 2) tft.setTextColor(TFT_BLACK, TFT_MAGENTA); else tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.setCursor(170, GRAPH_BOT + 6);
  tft.print(" ZOOM:x"); tft.print(vScale, 1); tft.print(" ");

  // Kênh 4: YPOS (Màu Trắng)
  if(menuFocus == 3) tft.setTextColor(TFT_BLACK, TFT_WHITE); else tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(250, GRAPH_BOT + 6);
  tft.print(" YPOS:"); tft.print(-yOffset); tft.print(" ");
}