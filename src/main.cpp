// 引入头文件
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// 宏定义GPIO引脚
#define FRQ_CNT_GPIO_CHN0 2
#define FRQ_CNT_GPIO_CHN1 12
#define FRQ_CNT_GPIO_CHN2 14
#define FRQ_CNT_GPIO_CHN3 13

// 宏定义频率波动阈值（待确定）
#define FREQUENCY_THRESHOLD_CHN0 200
#define FREQUENCY_THRESHOLD_CHN1 250
#define FREQUENCY_THRESHOLD_CHN2 200
#define FREQUENCY_THRESHOLD_CHN3 800

// 宏定义标准差阈值（待确定）
#define SD_THRESHOLD 150

// 定义是否已经读取过初始频率
bool hasReadInitialFrequency = false;  

// 定义全局变量
double BASIC_FREQUENCY_CHN0 = 0;
double BASIC_FREQUENCY_CHN1 = 0;
double BASIC_FREQUENCY_CHN2 = 0;
double BASIC_FREQUENCY_CHN3 = 0;

double BASIC_FREQUENCY_SUPINE_CHN0;
double BASIC_FREQUENCY_SUPINE_CHN1; 
double BASIC_FREQUENCY_SUPINE_CHN2;
double BASIC_FREQUENCY_SUPINE_CHN3; 

double FrqValue_CHN0;
double FrqValue_CHN1;
double FrqValue_CHN2;
double FrqValue_CHN3;

double TimeInterval;

unsigned int CountingValue_CHN0;
unsigned int CountingValue_CHN1;
unsigned int CountingValue_CHN2;
unsigned int CountingValue_CHN3;

unsigned long lastCorrectionTime = 0;       

double calibration_factors[4] = {1, 1, 1, 1};

// 声明OLED显示屏对象
Adafruit_SSD1306 display(128, 64, &Wire);

// 封装显示函数
void displayText(String text, int time_remaining){
  Serial.println(text);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(text);
  display.println("sleep");
  display.println("Time remaining: " + String(time_remaining) + "s");
  display.display();
}

// 每隔1秒计算一次频率
void TaskFrqMeter(void *ptParam) {
  while(1) {
    FrqValue_CHN0 = (double)CountingValue_CHN0 / TimeInterval / 2;
    FrqValue_CHN1 = (double)CountingValue_CHN1 / TimeInterval / 2;
    FrqValue_CHN2 = (double)CountingValue_CHN2 / TimeInterval / 2;
    FrqValue_CHN3 = (double)CountingValue_CHN3 / TimeInterval / 2;
    CountingValue_CHN0 = 0;
    CountingValue_CHN1 = 0;
    CountingValue_CHN2 = 0;
    CountingValue_CHN3 = 0;
    // 转换成ms为单位
    vTaskDelay((unsigned int)(1000* TimeInterval));
    }
}


// 每秒输出一次频率
void TaskUART0(void *ptParam) {
  Serial.begin(115200);

  while(1) {      
    Serial.println("Channel 0: " + String(FrqValue_CHN0) + " Hz");
    Serial.println("Channel 1: " + String(FrqValue_CHN1) + " Hz");
    Serial.println("Channel 2: " + String(FrqValue_CHN2) + " Hz");
    Serial.println("Channel 3: " + String(FrqValue_CHN3) + " Hz");

    vTaskDelay(1000);
  }
}


// 中断服务函数
void PulseCountingChn0(void) {
  CountingValue_CHN0 = CountingValue_CHN0 + 1;
}

void PulseCountingChn1(void) {
  CountingValue_CHN1 = CountingValue_CHN1 + 1;
}

void PulseCountingChn2(void) {
  CountingValue_CHN2 = CountingValue_CHN2 + 1;
}

void PulseCountingChn3(void) {
  CountingValue_CHN3 = CountingValue_CHN3 + 1;
}



void setup() {
  // 初始化GPIO
  TimeInterval = 1;
  FrqValue_CHN0 = 0;
  FrqValue_CHN1 = 0;
  FrqValue_CHN2 = 0;
  FrqValue_CHN3 = 0;
  CountingValue_CHN0 = 0;
  CountingValue_CHN1 = 0;
  CountingValue_CHN2 = 0;
  CountingValue_CHN3 = 0;
  
  pinMode(FRQ_CNT_GPIO_CHN0, INPUT);    
  pinMode(FRQ_CNT_GPIO_CHN1, INPUT);
  pinMode(FRQ_CNT_GPIO_CHN2, INPUT);
  pinMode(FRQ_CNT_GPIO_CHN3, INPUT);  
  
  attachInterrupt(FRQ_CNT_GPIO_CHN0, PulseCountingChn0, CHANGE);
  attachInterrupt(FRQ_CNT_GPIO_CHN1, PulseCountingChn1, CHANGE);
  attachInterrupt(FRQ_CNT_GPIO_CHN2, PulseCountingChn2, CHANGE);
  attachInterrupt(FRQ_CNT_GPIO_CHN3, PulseCountingChn3, CHANGE);

  xTaskCreatePinnedToCore(TaskFrqMeter, "Task FrqMeter", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskUART0, "Task UART0", 2048, NULL, 1, NULL, 1);

  //初始化OLED显示屏
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
}


void loop(){
  if (!hasReadInitialFrequency && millis() >= 15000)  // 如果还没有读取过初始频率，并且已经过去15s
  {
    BASIC_FREQUENCY_CHN0 = FrqValue_CHN0;
    BASIC_FREQUENCY_CHN1 = FrqValue_CHN1;
    BASIC_FREQUENCY_CHN2 = FrqValue_CHN2;
    BASIC_FREQUENCY_CHN3 = FrqValue_CHN3;
    BASIC_FREQUENCY_SUPINE_CHN0 = BASIC_FREQUENCY_CHN0 - 50;
    BASIC_FREQUENCY_SUPINE_CHN1 = BASIC_FREQUENCY_CHN1 - 50;
    BASIC_FREQUENCY_SUPINE_CHN2 = BASIC_FREQUENCY_CHN2 - 50;
    BASIC_FREQUENCY_SUPINE_CHN3 = BASIC_FREQUENCY_CHN3 - 50;
    hasReadInitialFrequency = true;  // 标记已经读取过初始频率
  }

  unsigned long currentTime = millis();
  if (currentTime - lastCorrectionTime >= 60000) { // 检查是否已经过了30s
    // 进行基准电容值修正
    BASIC_FREQUENCY_CHN0 = FrqValue_CHN0;
    BASIC_FREQUENCY_CHN1 = FrqValue_CHN1;
    BASIC_FREQUENCY_CHN2 = FrqValue_CHN2;
    BASIC_FREQUENCY_CHN3 = FrqValue_CHN3;
    BASIC_FREQUENCY_SUPINE_CHN0 = BASIC_FREQUENCY_CHN0 - 50;
    BASIC_FREQUENCY_SUPINE_CHN1 = BASIC_FREQUENCY_CHN1 - 50;
    BASIC_FREQUENCY_SUPINE_CHN2 = BASIC_FREQUENCY_CHN2 - 50;
    BASIC_FREQUENCY_SUPINE_CHN3 = BASIC_FREQUENCY_CHN3 - 50;

    lastCorrectionTime = currentTime; // 更新上次修正的时间
  }

  // 计算距离下次修正的剩余时间（秒）
  int remainingTime = 60 - (currentTime - lastCorrectionTime) / 1000;
  
  //屏幕设置
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  //用指针将四个频率值存入数组
  double frequencies[4] = {FrqValue_CHN0, FrqValue_CHN1, FrqValue_CHN2, FrqValue_CHN3};

  //计算四个频率值的偏差
  double basic_supine_frequency[4] = {BASIC_FREQUENCY_SUPINE_CHN0, BASIC_FREQUENCY_SUPINE_CHN1, BASIC_FREQUENCY_SUPINE_CHN2, BASIC_FREQUENCY_SUPINE_CHN3};
  double sd = 0;
  for (int i = 0; i < 4; i++){
    sd += pow(frequencies[i] - basic_supine_frequency[i], 2);
  }
  sd = sqrt(sd / 4);

  //输出频率值和标准差
  Serial.println("SD: " + String(sd) + "\n");
  Serial.println("Basic frequency of D2: " + String(BASIC_FREQUENCY_CHN0) + " Hz");
  Serial.println("Basic frequency of D12: " + String(BASIC_FREQUENCY_CHN1) + " Hz");
  Serial.println("Basic frequency of D14: " + String(BASIC_FREQUENCY_CHN2) + " Hz");
  Serial.println("Basic frequency of D13: " + String(BASIC_FREQUENCY_CHN3) + " Hz");
  Serial.println("D2: " + String(BASIC_FREQUENCY_CHN0 - frequencies[0]));
  Serial.println("D12: " + String(BASIC_FREQUENCY_CHN1 - frequencies[1]));
  Serial.println("D14: " + String(BASIC_FREQUENCY_CHN2 - frequencies[2]));
  Serial.println("D13: " + String(BASIC_FREQUENCY_CHN3 - frequencies[3]));
 
  //定义仰睡状态
  if (sd < SD_THRESHOLD) {
    displayText("Supine", remainingTime);
  } 
  else {
    //定义右侧睡眠状态和左侧睡眠状态
    if((BASIC_FREQUENCY_CHN1 - frequencies[1]) > FREQUENCY_THRESHOLD_CHN1 || (BASIC_FREQUENCY_CHN3 - frequencies[3]) > FREQUENCY_THRESHOLD_CHN3){
      displayText("right side", remainingTime);
    }
    else if((BASIC_FREQUENCY_CHN0 - frequencies[0]) > FREQUENCY_THRESHOLD_CHN0 || (BASIC_FREQUENCY_CHN2 - frequencies[2]) > FREQUENCY_THRESHOLD_CHN2){
      displayText("left side", remainingTime);
    }
    else{
      displayText("Supine", remainingTime);
    }
  }
  delay(1000);
}  
