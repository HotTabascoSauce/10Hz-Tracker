#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// LED和按键引脚定义
#define LED_PIN    42
#define BUTTON_PIN  0   // 连接按键的引脚
#define LED_COUNT  60
#define BRIGHTNESS 50

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

int currentColor = 0;         // 当前选中的颜色索引
bool lastButtonState = HIGH;  // 上次按键状态
bool currentButtonState = HIGH; // 当前按键状态
unsigned long lastDebounceTime = 0; // 上次检测到按键变化的时间
unsigned long debounceDelay = 2;   // 消抖延迟时间（毫秒）

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  Serial.begin(115200);
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  
  // 初始化按键引脚
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 使用内部上拉电阻
  
  // 初始显示无色
  //colorWipe(colors[3], 0);
}

void loop() {
  // 读取按键状态并进行消抖处理
  int reading = digitalRead(BUTTON_PIN);
  
  // 如果状态变化，重置去抖动计时器
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // 经过消抖时间后确认状态变化
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      // 检测到按键按下（下降沿）
      if (currentButtonState == LOW) {
        changeColor();
      }
    }
  }
  
  lastButtonState = reading;
}
void changeColor() {
  currentColor = (currentColor + 1) % 3; // 循环切换
  
  switch(currentColor) {
    case 0:
      colorWipe(strip.Color(255, 0, 0), 0); // 红色
      break;
    case 1:
      colorWipe(strip.Color(0, 255, 0), 0); // 绿色
      break;
    case 2:
      colorWipe(strip.Color(0, 0, 255), 0); // 蓝色
      break;
  }
}
// Fill strip pixels one after another with a color. 
// Modified to use the existing colorWipe function from original code
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}