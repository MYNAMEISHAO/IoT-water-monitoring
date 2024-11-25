#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // Sử dụng thư viện LiquidCrystal_I2C của John Rickman
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

// Khai báo kết nối màn hình LCD I2C
#define LCD_ADDRESS 0x27  // Địa chỉ I2C của màn hình LCD
#define LCD_COLUMNS 16    // Số cột của màn hình LCD
#define LCD_ROWS 2        // Số hàng của màn hình LCD

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS); // Khai báo đối tượng LCD

// Cấu hình chân cảm biến
const int TDS_PIN = 36;             // Chân kết nối với TDS sensor
const int ONE_WIRE_BUS = 14;        // Chân kết nối với DS18B20 sensor

// Biến để lưu giá trị ADC (Analog-to-Digital Converter) và điện áp
int adcValue = 0;
float voltage = 0.0;

// Hằng số điện áp tham chiếu của ESP32
const float referenceVoltage = 3.3;

// Hằng số để chuyển đổi từ giá trị ADC sang giá trị TDS (ppm - parts per million)
const float TDS_FACTOR = 0.5;

// Thiết lập kết nối với DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Thông tin kết nối Firebase
#define FIREBASE_HOST "https://water-monitoring-111ad-default-rtdb.asia-southeast1.firebasedatabase.app" 	
#define FIREBASE_AUTH "wr3yqB8ANVgVGyK1moYPLkciWoruXV48xIj5INx1"  
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
FirebaseJson json;

// Thông tin Wi-Fi
const char* WIFI_SSID = "Kim Anh";  
const char* WIFI_PASSWORD ="12345678";  

void setup() {
  // Khởi tạo giao tiếp Serial để hiển thị kết quả
  Serial.begin(115200);
  
  // Khởi tạo DS18B20
  sensors.begin();

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối WiFi thành công");

  // Kết nối Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  // Khởi tạo màn hình LCD qua I2C
  lcd.init();
  lcd.backlight();  // Bật đèn nền LCD
  lcd.clear();      // Xóa nội dung màn hình
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();
}

void updateFirebase(float tdsValue, float temperature) {
  // Đường dẫn đến Firebase nơi dữ liệu sẽ được lưu trữ
  String path = "/sensorData";

  // Tạo một đối tượng JSON để chứa dữ liệu
  FirebaseJson json;
  json.set("TDS", tdsValue);
  json.set("Temperature", temperature);

  // Gửi dữ liệu JSON đến Firebase
  if (Firebase.setJSON(firebaseData, path, json)) {
    Serial.println("Đã cập nhật dữ liệu lên Firebase thành công");
  } else {
    Serial.print("Lỗi khi cập nhật lên Firebase: ");
    Serial.println(firebaseData.errorReason());
  }
}

void loop() {
  // Đọc giá trị từ TDS sensor (từ 0 đến 4095 với độ phân giải 12-bit ADC)
  adcValue = analogRead(TDS_PIN);
  
  // Chuyển đổi giá trị ADC sang điện áp
  voltage = adcValue * (referenceVoltage / 4095.0);
  
  // Tính toán giá trị TDS (ppm)
  float tdsValue = (voltage / referenceVoltage) * 1000 * TDS_FACTOR;

  // Đọc nhiệt độ từ cảm biến DS18B20
  sensors.requestTemperatures(); // Gửi yêu cầu đọc nhiệt độ
  float temperature = sensors.getTempCByIndex(0); // Lấy nhiệt độ từ cảm biến (°C)

  // In kết quả ra Serial Monitor
  Serial.print("ADC Value: ");
  Serial.print(adcValue);
  Serial.print("\tVoltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V\tTDS: ");
  Serial.print(tdsValue, 2);
  Serial.print(" ppm\tTemperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  // Hiển thị kết quả trên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TDS: ");
  lcd.print(tdsValue);
  lcd.print(" ppm");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  // Cập nhật dữ liệu lên Firebase
  updateFirebase(tdsValue, temperature);

  // Dừng trong 1 giây trước khi lặp lại
  delay(1000);
}
