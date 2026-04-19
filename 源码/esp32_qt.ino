#include <WiFi.h>


const char* ssid = "ONEADD";
const char* password = "147258369";
WiFiServer server(1234); // 端口号


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, 16, 17);  // 初始化Serial1
  Serial2.begin(9600, SERIAL_8N1, 18, 19);  // 初始化Serial2

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.println("IP地址: " + WiFi.localIP().toString());
  
  server.begin(); // 启动TCP服务器
}

void loop() { 

  WiFiClient client = server.available();
  if (client) {
    Serial.println("客户端已连接");

    while(Serial2.available()>0)
    {
      Serial2.read();
    }

    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');
        Serial.println("收到数据: " + data);
        Serial1.print(data);
        client.print("ESP32已收到: " + data + "\n"); // 回传数据
      }

       if(Serial1.available()>0)
       {
          String data = Serial1.readStringUntil('\n');
          Serial.println(data);
          client.print(data);
        }
        if(Serial2.available()>0)
       {
          String data = Serial2.readStringUntil('\n');
          Serial1.print(data);
          Serial.print("来自嵌入式:");
          Serial.println(data);
        }
    }

    client.stop();
    Serial.println("客户端断开");
  }
}







