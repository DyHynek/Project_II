#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <DFRobot_ENS160.h>
#include <Wifi.h>
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>


const char* ssid = "Tam Tuoi";
const char* password = "99999999";


unsigned long channelID = 2767141;
const char* apiKey = "2AWMAE61IHWY4F0M"; // Thay bằng Write API Key của bạn
WiFiClient  client;

// Khởi tạo cảm biến ENS160 với địa chỉ I2C là 0x53
DFRobot_ENS160_I2C ens160(&Wire, 0x53);  // Địa chỉ I2C mặc định của ENS160 là 0x53

// Khởi tạo cảm biến AHT21
Adafruit_AHTX0 aht;

//khởi tạo LCD1602
LiquidCrystal_I2C lcd(0x27,16,2); 




// Biến toàn cục lưu dữ liệu
float temperature = 0.0, humidity = 0.0;
int co2 = 0, tvoc = 0;
// Semaphore để đồng bộ dữ liệu
SemaphoreHandle_t dataSemaphore;

//hàm hiển thị
void taskDisplayLCD(void* parameter) {
  while(1){
    xSemaphoreTake(dataSemaphore, portMAX_DELAY);

    // Chuẩn bị dữ liệu cho LCD
    String line1 = " Temp: " + String(temperature) + "C  Humi: " + String(humidity) + "%  ";
    String line2 = " eCO2: " + String(co2) + "ppm  TVOC: " + String(tvoc) + "ppb  ";
    xSemaphoreGive(dataSemaphore);
    delay(300);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    // Nếu dữ liệu dài hơn 16 ký tự, thực hiện cuộn
    if (line1.length() > 16 || line2.length() > 16) {
      int maxLength = max(line1.length(), line2.length());
      for (int i = 0; i <= maxLength - 16; i++) {
        lcd.scrollDisplayLeft();
        delay(700); // Tốc độ cuộn
      }
      
    }
  }
}

void taskSendThingSpeak(void* parameter){
  while (1)
  {
    xSemaphoreTake(dataSemaphore, portMAX_DELAY);
    co2 = ens160.getECO2();
    tvoc = ens160.getTVOC();
    sensors_event_t humidityAHT, temperatureAHT;
    aht.getEvent(&humidityAHT, &temperatureAHT);
    temperature = temperatureAHT.temperature;
    humidity = humidityAHT.relative_humidity;
    xSemaphoreGive(dataSemaphore);

    Serial.print("ENS160 - CO2: ");
    Serial.print(co2);
    Serial.println(" ppm");
    Serial.print("ENS160 - TVOC: ");
    Serial.print(tvoc);
    Serial.println(" ppb");
    Serial.print("AHT20 - Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("AHT20 - Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Gửi dữ liệu lên ThingSpeak
    ThingSpeak.setField(1, co2);
    ThingSpeak.setField(2, tvoc);
    ThingSpeak.setField(3, temperature);
    ThingSpeak.setField(4, humidity);
    int response = ThingSpeak.writeFields(channelID, apiKey);
    if (response == 200) {
      Serial.println("Dữ liệu đã gửi lên ThingSpeak thành công!");
    } else {
      Serial.print("Lỗi gửi dữ liệu lên ThingSpeak: ");
      Serial.println(response);
    }
    delay(15000);
  }
  
}

void setup() {
  // Khởi tạo giao tiếp Serial
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Kết nối với ThingSpeak
  ThingSpeak.begin(client);



  // Khởi tạo giao tiếp I2C
  Wire.begin();


  //khởi tạo LCD
  lcd.init();       
  lcd.backlight(); //bật đèn nền
  Serial.println("Khởi tạo LCD thành công!");
  lcd.setCursor(0, 0);
  lcd.print("NGUYEN DUY HUY");
  lcd.setCursor(0, 1);
  lcd.print("20210442");
  delay(15000);


  // Khởi tạo cảm biến ENS160
  if (NO_ERR != ens160.begin()) {
    Serial.println("Không thể khởi tạo cảm biến ENS160!");
    while (1);
  }
  // Khởi tạo cảm biến AHT20
  if (!aht.begin()) {
    Serial.println("Không thể khởi tạo cảm biến AHT20!");
    while (1);
  }
  Serial.println("Cảm biến ENS160 và AHT20 đã được khởi tạo thành công!");

  dataSemaphore = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(taskDisplayLCD, "DisplayLCD", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskSendThingSpeak, "DisplayLCD", 2048, NULL, 1, NULL, 1);
  

}



void loop() {

}
