#define BLYNK_TEMPLATE_ID "BLYNK-TEMPLATE-ID-HERE"
#define BLYNK_TEMPLATE_NAME "irigasi otomatis"
#define BLYNK_AUTH_TOKEN "BLYNK-AUTH-TOKEN-HERE"
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#define BLYNK_PRINT Serial

// nilai max dan min sensor
const int kering = 870;
const int basah = 343;

LiquidCrystal_I2C LCD(0x27, 20, 4);
DHT dht(14, DHT22);
BlynkTimer timer;
#define value A0
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "WIFI-NAME-HERE";
char pass[] = "YOUR-WIFI-PASSWORD-HERE";
WidgetLCD lcd(V1);
WidgetLCD lcdd(V5);
int pump = 12;
unsigned int detik, menit, jam;
unsigned int buttonState, modeAuto, settingKadarAir;
bool sedangMenyiram = false;

uint32_t start_sec;
uint32_t stop_sec;
String tz;

BLYNK_CONNECTED() {
  // Called when hardware is connected to Blynk.Cloud

  // get the latest value for V1
  Blynk.syncVirtual(V1);

  // Request Blynk server to re-send latest values for all pins
  Blynk.syncAll();
}

BLYNK_WRITE(V0) {
  buttonState = param.asInt();
  Serial.println(buttonState);
}

BLYNK_WRITE(V5) {
  modeAuto = param.asInt();
  Serial.println(modeAuto);
}

BLYNK_WRITE(V1) {
  settingKadarAir = param.asInt();
  Serial.println(settingKadarAir);
}
// timer
BLYNK_WRITE(V6) {
  // param[0] is the user time value selected in seconds.
  Serial.print("Start time in sec: ");
  start_sec = param[0].asInt();
  Serial.print("Waktu tersisa : ");
  Serial.println(start_sec);

  // param[1] is the stop time in seconds (when option enabled)
  Serial.print("Stop time in sec: ");
  stop_sec = param[1].asInt();
  Serial.println(stop_sec);

  // param[2] is the timezone (Ex. "America/New_York")
  Serial.print("Timezone: ");
  tz = param[2].asStr();
  Serial.println(tz);
}

void setup()
{

  delay(100);
  pinMode(pump, OUTPUT);
  pinMode(D6, OUTPUT);

  Serial.begin(115200);
  Serial.print("buttonState : ");
  Serial.println(buttonState);

  LCD.init();
  LCD.begin(20, 4);
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("__Irigasi Otomatis__");
  LCD.setCursor(0, 1); // baris pertama
  LCD.print("____dibuat oleh_____");
  LCD.setCursor(0, 2); // baris pertama
  LCD.print("Rahmatullah.R dan Team");
  LCD.setCursor(0, 3); // baris pertama
  LCD.print("_______Unhas_______");
  delay(3000);
  LCD.clear();

  dht.begin();

  timer.setInterval(1000L, Kadarair);
  timer.setInterval(1000L, DHT11sensor);
  timer.setInterval(1000L, waktu);
  Blynk.begin(auth, ssid, pass);
}

void waktu() {
  jam = (start_sec / 3600) % 24;     // Calculate hours (seconds divided by 3600, modulo 24)
  menit = (start_sec / 60) % 60;     // Calculate minutes (seconds divided by 60, modulo 60)
  detik = start_sec % 60;           // Calculate seconds (modulo 60)

  LCD.setCursor(0, 3);
  LCD.print("___Time: ");
  LCD.setCursor(8, 3);
  LCD.print(jam);
  LCD.print(" ");
  LCD.setCursor(10, 3);
  LCD.print(":");
  LCD.print(menit);
  LCD.print(" ");
  LCD.setCursor(13, 3);
  LCD.print(":");
  LCD.print(detik);
  LCD.print(" ___");
}


void Kadarair()
{
  // persen itu moisture
  int POT = analogRead(value);

  int persen = map(POT, basah, kering, 100, 0);
  // int persen = POT;

  // mengatasi kondisi persen diatas 100% dan dibawah 0%
  if (persen > 100)
  {
    persen = 100;
  }
  if (persen < 0)
  {
    persen = 0;
  }
  Serial.println(persen);
  Blynk.virtualWrite(V2, persen);
  // // debug kadar air
  // Serial.print("Kadar Air  : ");
  // Serial.print(persen);
  // Serial.println("%");

  lcdd.print(0, 0, "Irigasi Otomatis");
  lcdd.print(0, 1, "  Aulia Rahma  ");

  LCD.setCursor(0, 1); // baris pertama
  LCD.print("Ka:");
  LCD.print(persen, 1);
  LCD.print("%");
  LCD.print("  ");
  lcd.print(0, 0, "Kadar Air:");
  lcd.print(10, 0, persen);
  lcd.print(13, 0, "%  ");

  if (!modeAuto) {
    // mode manual
    if (buttonState && start_sec > 0) {
      sedangMenyiram = true;
      start_sec--;
      Serial.println(start_sec);
    }

    if (sedangMenyiram && start_sec ==  0) {
      sedangMenyiram = false;
      buttonState = 0;
    }


    digitalWrite(D6, buttonState);
    Blynk.virtualWrite(V0, buttonState);
    Blynk.virtualWrite(V6, start_sec, stop_sec, tz);
  }
  else {
    // mode otomatis
    if (persen < settingKadarAir)
    {
      digitalWrite(pump, HIGH);
      lcd.print(0, 1, "  Tanah kering  ");
      LCD.setCursor(10, 1);
      LCD.print("Pompa on  ");
    }
    else
    {
      digitalWrite(pump, LOW);
      lcd.print(0, 1, "  Tanah basah  ");
      LCD.setCursor(10, 1);
      LCD.print("Pompa off ");
    }
    Serial.print("settingKadarAir : ");
    Serial.println(settingKadarAir);
  }

  Blynk.setProperty(V0, "isDisabled", sedangMenyiram || modeAuto);
  Blynk.setProperty(V6, "isDisabled", modeAuto);
  Blynk.setProperty(V7, "isDisabled", sedangMenyiram || !modeAuto);
  Blynk.setProperty(V8, "isDisabled", !modeAuto);
  Blynk.setProperty(V1, "isDisabled", !modeAuto);
   // mencegah ganti mode saat timer manual nyala
  Blynk.setProperty(V5, "isDisabled", sedangMenyiram);
}

void DHT11sensor()
{
  float h = dht.readHumidity();
  float suhu = dht.readTemperature();
  Blynk.virtualWrite(V3, suhu);
  Blynk.virtualWrite(V4, h);
  // // debug suhu
  // Serial.print("Suhu : ");
  // Serial.print(suhu);
  LCD.setCursor(0, 0); // baris pertama
  LCD.print("__Irigasi Otomatis__");
  LCD.setCursor(0, 2); // baris pertama
  LCD.print("Te:");
  LCD.print(suhu, 1);
  LCD.print(".C");
  LCD.setCursor(11, 2); // baris pertama
  LCD.print("Rh:");
  LCD.print(h, 1);
  LCD.print("%");
}
void loop()
{ // Perulangan Program
  Blynk.run();
  timer.run();
}
