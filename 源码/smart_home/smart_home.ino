#include <Adafruit_Fingerprint.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>
#include <Servo.h>
#include <U8g2lib.h>  
#include <Wire.h>
#include<stdio.h>


//甲烷传感器，
const int methanePin = A4;  //模拟读取
const int methanePin2 = 22;//数字读取  有没有泄漏以这个为主，0表示有
float R0 = 10.0;            // 默认基准电阻值
float ppm=0.0;


//雾化片
#define addWet 2
//风扇
#define fan1 3
#define fan2 4
//客厅灯
#define led_living 5
//客厅氛围灯
#define led_living_plus 6
//厨房报警器
#define  alarm 7
#define alarmPin  11 //检测按钮
//温湿度传感器
#define DHTPIN 8
#define DHTTYPE DHT11
//温湿度传感器
DHT dht(DHTPIN, DHTTYPE);
//指纹传感器 接上3.3v电压
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);//默认18 19串口通信
//nfc刷卡 接上3.3v
MFRC522 mfrc522(53, 9);  // 创建MFRC522实例

// 光感传感器配置
#define photosensitivePin A0
#define phTimeInterval 1000
uint32_t phTimes = 0;
uint16_t photosenVal = 0; //亮度越高值越大
uint8_t photoContent = 0;
bool lightStatus = false;

// RGB LED引脚和变量
#define redPin  A1 
#define greenPin  A2
#define bluePin  A3
int redValue = 0;
int greenValue = 0;
int blueValue = 0;//0  255 数字越高该颜色越高


// 门对象
Servo doorServo;
#define doorServoPin  13// 舵机控制的引脚

// 摄像头舵机
Servo buttomServo;
#define buttomServoPin  10
Servo topServo;
#define topServoPin  12 
int buttomServoAngle=90;
int topServoAngle=0;

//温湿度
float humidity =0.0;
float temp =0.0;

int sensor[70];//传感器状态数组 每10个为一个区域
int (*handle[70])(int);//传感器处理信号数组
const byte authorizedUID[4] = {0x43, 0x32, 0x22, 0xBA};//nfc用户登录密钥

//清空缓存区
void clearSerialBuffer() 
{
  while(Serial.available() > 0) 
  {
    Serial.read(); // 逐字节读取并丢弃
  }
  while(Serial2.available() > 0) 
  {
    Serial2.read(); // 逐字节读取并丢弃
  }
  while(Serial3.available() > 0) 
  {
    Serial3.read(); // 逐字节读取并丢弃
  }
}

//函数声明
int handle_1_addWet(int signal);
int handle_1_fan(int signal);
int handle_1_led(int signal);
int handle_1_DHT(int signal);
int handle_1_topServo(int signal) ;
int handle_1_buttomServo(int signal) ;
int handle_3_RGB(int signal);
int handle_7_Fingerprint(int signal);
int handle_7_nfc(int signal);
int handle_5_alarm(int signal);
int handle_5_methane(int signal);
int handle_6_door(int signal);
int handle_6_Photosensor(int signal);
int handle_7_app(int signal);



void initSensor();
void identityAccess();
void getSignal();
void handleSignal();
void initPin();



void setup() {
  Serial.begin(9600);//电脑串口，查看调试信息
  finger.begin(9600);//指纹模块串口 
  Serial2.begin(9600);//51串口，查看调试信息
  Serial3.begin(9600);//ESP32串口，查看调试信息


  initPin();
  initSensor();

  //认证登录，成功才可以运行程序
  identityAccess();

  handle_6_door(2);
  handle_7_app(1);
  clearSerialBuffer();
}

void loop() 
{
  //获得信号
  getSignal(); 

  //处理信号
  handleSignal();

  handle_5_methane(1);
  delay(100);
}

void identityAccess()
{
  while(true)
  {
    delay(100);
    if(handle_7_Fingerprint(1) == 0)
    {
      Serial.print("指纹");
      break;
    }

    delay(100);
    if(handle_7_nfc(1)==0)
    {
      Serial.print("NFC");
      break;
    }
  }

  Serial2.print("60 5");
  Serial3.print("60 5");
  handle_6_door(2);
  Serial.println("认证完成，开始执行程序");
}

void initPin()
{
  pinMode(addWet,OUTPUT); //加湿器
  digitalWrite(addWet,LOW);

  pinMode(fan1,OUTPUT); //风扇
  pinMode(fan2,OUTPUT); //风扇
  digitalWrite(fan2,LOW);
  digitalWrite(fan1,LOW);

  pinMode(led_living,OUTPUT); //客厅灯
  digitalWrite(led_living,LOW);
  
  pinMode(led_living_plus,OUTPUT); //客厅氛围灯
  digitalWrite(led_living_plus,LOW);

  pinMode(alarm,OUTPUT); //报警器
  digitalWrite(alarm,HIGH);

  pinMode(photosensitivePin, INPUT);//光敏电阻
  
  pinMode(methanePin2,INPUT);//甲烷传感器
  pinMode(alarmPin,INPUT);//甲烷传感器对应按钮
}

void initSensor()
{
  for(int i=0; i<70; i++)
  {
    sensor[i]=0;
    handle[i]=nullptr;//空指针代表不存在当前传感器
  }

  //每个传感器传不同的函数指针
  handle[0]=handle_1_addWet;
  handle[1]=handle_1_fan;
  handle[2]=handle_1_led;
  handle[3]=handle_1_led_plus;
  handle[4]=handle_1_DHT;
  handle[5]=handle_1_buttomServo;
  handle[6]=handle_1_topServo;
  handle[21] = handle_3_RGB; 
  handle[40]= handle_5_methane;
  handle[41]=handle_5_alarm;
  handle[50] =handle_6_door;
  handle[51] =handle_6_Photosensor;
  handle[60]=handle_7_Fingerprint;
  handle[61]=handle_7_nfc;
  handle[69]=handle_7_app;
 

  dht.begin();  //初始化温湿度
  delay(2000);
  handle_1_DHT(1); //初始化温湿度
  SPI.begin();          // 初始化SPI总线
  mfrc522.PCD_Init();   // 初始化MFRC522
  handle_6_Photosensor(1); //初始化光感
  handle_3_RGB(2);
  doorServo.attach(doorServoPin); 
  doorServo.write(180);

  buttomServo.attach(buttomServoPin); 
  buttomServo.write(90);
  topServo.attach(topServoPin); 
  topServo.write(0);

  handle_6_door(1);
  // 自动校准（需在干净空气中运行）
  Serial.println("校准中...（保持传感器在干净空气）");
  float sum = 0;
  for(int i=0; i<100; i++) {
    float voltage = analogRead(methanePin) * (5.0/1023.0);
    sum += (5.0 - voltage)/voltage * 10.0; // 计算临时Rs值
    delay(10);
  }
  R0 = sum/100.0;  // 计算基准电阻
  Serial.print("校准完成 R0=");
  Serial.println(R0);


  Serial.println("传感器初始化完成！");

  char num_str[10];
  snprintf(num_str, sizeof(num_str), "%d", R0);

}

//获得信号
void getSignal()
{
  //从串口读取
  if(Serial.available()>=3)
  {
    int sensor_id = Serial.parseInt(); 
    int sensor_information = Serial.parseInt(); 
    sensor[sensor_id]=sensor_information;
    Serial.print(F("Received: ID="));
    Serial.print(sensor_id);
    Serial.print(F(" CMD="));
    Serial.println(sensor_information);
  }

  //从51获取
  if(Serial2.available()>=3)
  {
    int sensor_id = Serial2.parseInt(); 
    int sensor_information = Serial2.parseInt(); 
    sensor[sensor_id]=sensor_information;
    Serial.print(F("Received-51: ID="));
    Serial.print(sensor_id);
    Serial.print(F(" CMD="));
    Serial.println(sensor_information);
  }

  //从ESP32获取
  if(Serial3.available()>=3)
  {
    int sensor_id = Serial3.parseInt(); 
    int sensor_information = Serial3.parseInt(); 
    sensor[sensor_id]=sensor_information;

    Serial.print(F("Received-ESP32: ID="));
    Serial.print(sensor_id);
    Serial.print(F(" CMD="));
    Serial.println(sensor_information);
  }
}

//处理信号
void handleSignal()
{
  if(digitalRead(methanePin2)==0)
  {
    Serial.println("煤气泄漏");
    handle_5_alarm(1);
    handle_6_door(1);
    handle_1_fan(2);
    handle_1_addWet(1);
    Serial2.print("40 3");
    Serial3.print("40 3");
    //气体泄漏
    while(true)      
    {
      //按钮检测
      if(digitalRead(alarmPin)==HIGH)
      {
        handle_5_alarm(2);
        handle_1_addWet(2);
        handle_1_fan(1);
        handle_6_door(2);
        Serial.println("请用户及时处理泄漏");
        break;
      }
    }
  }

  for(int i=0; i<70; i++)
  {
    if(handle[i]!=nullptr)
    {
      handle[i](sensor[i]);
    }
    //处理完之后清空旧信号
    sensor[i]=0;
  }

}


int handle_7_app(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  if(signal ==1)
  {
    handle_1_DHT(1);
    handle_6_Photosensor(1);
    handle_5_methane(1);

    Serial3.print("69 ");
    Serial3.print(temp);
    Serial3.print(" ");
    Serial3.print(humidity);
    Serial3.print(" ");
    Serial3.print(photosenVal);
    Serial3.print(" ");
    Serial3.println(ppm);  
    Serial3.print(" ");

    Serial.print("数据刷新完毕");
  }
  delay(100);
  Serial3.print("60 5");
  return 0;
}



int handle_7_nfc(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  if(signal ==1)
  {
    Serial.println("NFC模块认证登录开始.....");
    Serial.println("等待NFC验证...");
    // 检测到卡片时继续
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return 1; // 没有卡片时退出
    }
    // 读取卡片信息
    if (!mfrc522.PICC_ReadCardSerial()) {
      return 1; // 读取失败时退出
    }
    // 显示原始UID
    Serial.print("检测到卡片 UID:");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    // 验证UID是否匹配
    bool isAuthorized = true;
    // 首先检查UID长度
    if (mfrc522.uid.size != 4) {
      isAuthorized = false;
    } else {
      // 逐字节比对
      for (byte i = 0; i < 4; i++) {
        if (mfrc522.uid.uidByte[i] != authorizedUID[i]) {
          isAuthorized = false;
          break;
        }
      }
    }
    // 输出验证结果
    if (isAuthorized) 
    {
      Serial.println("验证通过：合法卡片！");
      return 0;
    } else 
    {
      Serial.println("验证失败：非法卡片！");
      return 1;
    }
   // 停止读卡
    mfrc522.PICC_HaltA();
  }

  return 0;
}


//函数实现
int handle_7_Fingerprint(int signal)
{
  if(signal==0)
  {
    //可加可不加
    //Serial.println("指纹模块正常，无操作");
  }
  else if(signal==1)
  {

    //验证指纹
    Serial.println("指纹模块认证登录开始.....");
    Serial.println("等待指纹验证...");
    for (u8 i = 0; i < 15; i++) 
    {
      u8 ensure = finger.getImage();
      if(ensure == FINGERPRINT_OK) 
      {
        ensure = finger.image2Tz();
        if (ensure == FINGERPRINT_OK) 
        {
          ensure = finger.fingerFastSearch();
          if (ensure == FINGERPRINT_OK) 
          {
            Serial.print("验证成功！ID:");
            Serial.print(finger.fingerID);
            Serial.print(" 置信度:");
            Serial.println(finger.confidence);
            return 0;
          }
        }
      }
    }
    Serial.println("验证失败");
    return 1;
  }
  else if(signal==2)
  {
    handle_7_Fingerprint(4);
    u8 i, ensure, processnum = 0;
    u8 ID_NUM = 0;
    while (1) {
      switch (processnum) {
        case 0:
          Serial.println("请按手指...");
          ensure = finger.getImage();
          if (ensure == FINGERPRINT_OK) {
            ensure = finger.image2Tz(1);
            if (ensure == FINGERPRINT_OK) {
              Serial.println("指纹识别成功");
              processnum = 1;
            }
          }
          delay(1000);
          break;

        case 1:
          Serial.println("请再次按压同一手指...");
          ensure = finger.getImage();
          if (ensure == FINGERPRINT_OK) {
            ensure = finger.image2Tz(2);
            if (ensure == FINGERPRINT_OK) {
              Serial.println("二次验证成功");
              processnum = 2;
            }
          }
          delay(1000);
          break;

        case 2:
          Serial.println("正在创建模板...");
          ensure = finger.createModel();
          if (ensure == FINGERPRINT_OK) {
            Serial.println("模板创建成功，请输入ID（1-99）:");
            clearSerialBuffer();
            while (!Serial.available());
          
            //解决非堵塞读取不到数字问题
            while(ID_NUM==0)
            {
              ID_NUM = Serial.parseInt();
            }

            Serial.print("设置ID为：");
            Serial.println(ID_NUM);
            
            ensure = finger.storeModel(ID_NUM);
            if (ensure == FINGERPRINT_OK) {
              Serial.println("指纹保存成功！");
              return;
            }
          }
          break;
      }
      delay(500);
  }
  }
  else if(signal==3)
  {
    //删除指纹
    Serial.println("请输入要删除的ID（1-99）:");
    handle_7_Fingerprint(4);//打印信息
    clearSerialBuffer();
    while (!Serial.available());

    //解决非堵塞读取不到数字问题
    u8 ID_NUM=0;
    while(ID_NUM==0)
    {
      ID_NUM = Serial.parseInt();
    }
    Serial.print("正在删除ID:");
    Serial.println(ID_NUM);
    
    u8 ensure = finger.deleteModel(ID_NUM);
    if (ensure == FINGERPRINT_OK) 
    {
      Serial.println("删除成功");
    } 
    else 
    {
      Serial.println("删除失败");
      return 2;
    }
  }
  else if(signal ==4)
  {
    Serial.println("\n===== 已注册指纹 =====");
    // 获取模板数量
    finger.getTemplateCount();
    Serial.print("总数: ");
    Serial.println(finger.templateCount);
    // 获取指纹ID列表
    Serial.println("ID列表:");
    uint8_t id_list[100];  // 最大支持100个ID
    uint8_t count = 0;
    
    for (uint16_t id = 0; id < 100; id++) 
    {
      if (finger.loadModel(id) == FINGERPRINT_OK) 
      {
        id_list[count++] = id;
        Serial.print(id);
        Serial.print(" ");   
        // 每行显示10个ID
        if (count % 10 == 0) Serial.println();
      }
    }
    if (count == 0) Serial.println("无注册指纹");
    else Serial.println("\n======================");
  }
  return 0;
}

//加湿器模块
int handle_1_addWet(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  else if(signal==1)
  {
    digitalWrite(addWet,HIGH);
    Serial.println("加湿器已经打开");
  }
  else if(signal==2)
  {
    digitalWrite(addWet,LOW);
    Serial.println("加湿器已经关闭");
  }
  else
  {
    return 1;
  }

  return 0;
}


//风扇模块
int handle_1_fan(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  else if(signal==1)
  {
    digitalWrite(fan1,LOW);
    digitalWrite(fan2,LOW);
    Serial.println("风扇已经关闭");
  }
  else if(signal==2)
  {
    digitalWrite(fan1,HIGH);
    digitalWrite(fan2,LOW);
    Serial.println("风扇打开");
  }
  else
  {
    return 1;
  }

  return 0;
}


//客厅灯模块
int handle_1_led(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  else if(signal==1)
  {
    digitalWrite(led_living,HIGH);
    Serial.println("客厅灯已经打开");
  }
  else if(signal==2)
  {
    digitalWrite(led_living,LOW);
    Serial.println("客厅灯已经关闭");
  }
  else
  {
    return 1;
  }

  return 0;
}

//客厅氛围灯模块
int handle_1_led_plus(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  else if(signal==1)
  {
    digitalWrite(led_living_plus,HIGH);
    Serial.println("客厅氛围灯已经打开");
  }
  else if(signal==2)
  {
    digitalWrite(led_living_plus,LOW);
    Serial.println("客厅氛围灯已经关闭");
  }
  else
  {
    return 1;
  }

  return 0;
}


int handle_5_alarm(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  else if(signal==1)
  {
    Serial.println("报警器已经打开");
    digitalWrite(alarm,LOW);
  }
  else if(signal==2)
  {
    Serial.println("报警器已经关闭");
    digitalWrite(alarm,HIGH);
  }
  else
  {
    return 1;
  }

  return 0;
}

int handle_1_DHT(int signal)
{
  if(signal==0)
  {
    return 0;
  }
  else if(signal==1)
  {
    delay(2000);
    humidity = dht.readHumidity();    // 读取湿度(%RH)
    temp = dht.readTemperature();     // 读取温度(℃)
    Serial.println("温湿度数据更新完毕");
  }
  else if(signal==2)
  {
    // 格式化输出数据
    Serial.print("湿度: ");
    Serial.print(humidity);
    Serial.print("%\t温度: ");
    Serial.print(temp);
    Serial.println("℃\t");
    Serial.println("温湿度数据打印完毕");
  }
  else
  {
    return 1;
  }

  return 0;
}


int handle_6_Photosensor(int signal) 
{
  if(signal == 1)
  {
    static uint32_t lastRead = 0;
    if ((uint32_t)(millis() - lastRead) >= phTimeInterval) {
      photosenVal = analogRead(photosensitivePin);
      photosenVal = constrain(photosenVal, 10, 1024);
      photoContent = map(photosenVal, 10, 1024, 100, 0);
      lightStatus = (photoContent > 50);
      lastRead = millis();
    }
  }
  else if (signal == 2) {
    Serial.print(F("[20] Light: ")); // F()节省RAM
    Serial.print(photoContent);
    Serial.print(F("% (Raw: "));
    Serial.print(photosenVal);
    Serial.println(lightStatus ? " Bright" : " Dark");
  }
  else
    return 1;
  return 0;
}


int handle_3_RGB(int signal) {
  int mysignal=signal & 0xff;
  switch(mysignal) {
    case 0:  // 无操作
      break;
    case 1:  // 设置颜色值
      //从串口读取
      // 数值约束
      redValue = signal>>24 & 0xff;
      greenValue = signal>>16 & 0xff;
      blueValue = signal>>8 & 0xff;
      Serial.print("设置颜色：R=");
      Serial.print(redValue);
      Serial.print(" G=");
      Serial.print(greenValue);
      Serial.print(" B=");
      Serial.println(blueValue);

      analogWrite(redPin, redValue);
      analogWrite(greenPin, greenValue);
      analogWrite(bluePin, blueValue);
      Serial.println("RGB已点亮");
      break;
      
    case 2:  // 点亮LED
      analogWrite(redPin, 0);
      analogWrite(greenPin, 0);
      analogWrite(bluePin, 0);
      Serial.println("RGB已关闭");
      break;
      
    case 3:  // 打印当前值
      Serial.print("当前颜色：R=");
      Serial.print(redValue);
      Serial.print(" G=");
      Serial.print(greenValue);
      Serial.print(" B=");
      Serial.println(blueValue);
      break;

    case 4:  
      redValue=180;
      greenValue=100;
      blueValue=20;
      analogWrite(redPin, redValue);
      analogWrite(greenPin, greenValue);
      analogWrite(bluePin, blueValue);
      Serial.println("暗色模式已打开");
      break;

    case 5: 
      redValue=255;
      greenValue=100;
      blueValue=0;
      analogWrite(redPin, redValue);
      analogWrite(greenPin, greenValue);
      analogWrite(bluePin, blueValue);
      Serial.println("暖色模式已打开");
      break;

    case 6: 
      redValue=255;
      greenValue=255;
      blueValue=255;
      analogWrite(redPin, redValue);
      analogWrite(greenPin, greenValue);
      analogWrite(bluePin, blueValue);
      Serial.println("白光模式已打开");
      break;
    default:
      Serial.println("无效信号");
      return -1;
  }
  return 0;
}



int handle_6_door(int signal) 
{
  switch (signal) {
    case 0:  // 无操作
      break;

    case 1:  
      doorServo.write(180);
      Serial.println("门关闭");
      break;

    case 2:  
      doorServo.write(90);
      Serial.println("门打开");
      break;
    default:
      Serial.println("无效信号");
      return -1;
  }
  return 0;
}



int handle_5_methane(int signal) 
{
  if(signal==0)
  {
    return  0;
  }
  else if(signal==1)
  {
    // 读取当前浓度
    float voltage = analogRead(methanePin) * (5.0/1023.0);
    float Rs = (5.0 - voltage)/voltage * 10.0;
    ppm = 1016.0 * pow(Rs/R0, -2.95);  // 甲烷换算公式

    delay(2000);  // 每2秒更新
  }
  else if(signal==2)
  {
    // 输出浓度值
    Serial.print("甲烷浓度: ");
    Serial.print(ppm);
    Serial.println(" ppm");
  }
  return 0;
}

int handle_1_buttomServo(int signal) 
{
  if(signal==0)
  {
    return  0;
  }
  else if(signal==1)
  {

    buttomServoAngle+=10;
    if(buttomServoAngle>=180)
      return 1;
    buttomServo.write(buttomServoAngle);
    Serial.println("向左转");
  }
  else if(signal==2)
  {
    if(buttomServoAngle<=0)
      return 1;
    buttomServoAngle-=10;
    buttomServo.write(buttomServoAngle);
     Serial.println("向右转");
  }
  else if(signal==3)
  {
    buttomServoAngle=90;
    buttomServo.write(buttomServoAngle);
    Serial.println("底部舵机复位");
    delay(100);
  }
  return 0;
}




int handle_1_topServo(int signal) 
{
  if(signal==0)
  {
    return  0;
  }
  else if(signal==1)
  {
    topServoAngle+=10;
    if(topServoAngle>=90)
      topServoAngle=90;
    topServo.write(topServoAngle);
    Serial.println("向下转");
  }
  else if(signal==2)
  {
    topServoAngle-=10;
    if(topServoAngle<=0)
      topServoAngle=0;
    topServo.write(topServoAngle);
    Serial.println("向上转");
  }
  else if(signal==3)
  {
    topServoAngle=0;
    topServo.write(topServoAngle);
    Serial.println("上部舵机复位");
    delay(100);
  }
  return 0;
}








