/**********************************************************************
  Filename    : ESP32S3_lab_6a
  Target      : ESP32-S3-WROOM
  Description : input from thermistor, control servo
  Author      : Martin Harnevie
  Logbook     :
  20261101 000 - set it up
  **********************************************************************/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t   KPinLedInternal =           0x02; // internal LED pin
const uint8_t   KPinLedExternal =           0x0e; // 14; // external LED pin
const uint8_t   KPinAdcPhotoResistor =      0x01; // photo resistor
const uint8_t   KPinI2cScl =                0x0d; // 13; // SCL
const uint8_t   KPinI2cSda =                0x0c; // 12; // SDA

static String inputString = "";       // a String to hold incoming data
bool inputStringCompletedB = false;   // whether the string is complete or not

LiquidCrystal_I2C lcd(0x27,16,2); // 16 columns, 2 rows

uint16_t valueAdcPhotoResistor;
const uint8_t KPhotoArraySize = 5;
bool showAsciiB = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // initialise I2C
  Wire.begin(KPinI2cSda,KPinI2cScl);

  // initialise LCD 16×2
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ljus:");
  lcd.setCursor(7,0);
  lcd.print("snart");
  lcd.setCursor(0,1);
  lcd.print("Medel:");
  lcd.setCursor(7,1);
  lcd.print("snart");
  delay(2000);
  lcd.setCursor(7,0);
  lcd.print("      ");
  lcd.setCursor(7,1);
  lcd.print("      ");

  // reserve 200 bytes for inputString - prevent String from fragmenting the heap
  inputString.reserve(200);
 

  // initialise ADC photo resistor
  pinMode( KPinAdcPhotoResistor , INPUT );
  analogSetAttenuation(ADC_11db); // 11dB covers the 0-3.3V range

  // initialise external LED
  pinMode( KPinLedExternal, OUTPUT );

  // initialise heartbeat
  pinMode( KPinLedInternal, OUTPUT );
}

void loop() {

  readAndPrintPhotoResistor(1000);

  turnOffAndOnExternalLed(12000); // 12 seconds
  
  handleSerialInput(222);

  doHeartBeat();

}

void turnOffAndOnExternalLed(uint32_t aInterval) {
  uint32_t timeCurrent = millis();
  static uint32_t timePrevious = timeCurrent;
  if (timeCurrent - timePrevious < aInterval) return; else timePrevious = timeCurrent;

  static bool ledOnB = false;
  if(ledOnB) {
    digitalWrite(KPinLedExternal, LOW);
    ledOnB = false;
  } else {
    digitalWrite(KPinLedExternal, HIGH);
    ledOnB = true;
  }
}

void readAndPrintPhotoResistor(uint32_t aInterval) {
  uint32_t timeCurrent = millis();
  static uint32_t timePrevious = timeCurrent;
  if (timeCurrent - timePrevious < aInterval) return; else timePrevious = timeCurrent;

  uint16_t average = floatingAverage(valueAdcPhotoResistor);
  valueAdcPhotoResistor = 4095 - analogRead(KPinAdcPhotoResistor);

  Serial.print("value: ");
  Serial.print(valueAdcPhotoResistor);
  Serial.print("\taverage: ");
  Serial.print(average);
  Serial.print("\tdifference: ");
  Serial.print((int) valueAdcPhotoResistor-average);
  Serial.println();

  lcd.setCursor(7,0);
  lcd.print("    ");
  lcd.setCursor(7,0);
  lcd.print(valueAdcPhotoResistor);
  lcd.setCursor(7,1);
  lcd.print("    ");
  lcd.setCursor(7,1);
  lcd.print(average);

  static uint8_t ascii = 0x00;

  if (showAsciiB) {
    // just for testing out the available characters of the LCD
    lcd.setCursor(15,0);
    lcd.print(char(ascii));
    ascii++;
    if(ascii == 0xFF) ascii = 0x00;
  } else {
    ascii = 0xa0;
    lcd.setCursor(15,0);
    lcd.print(char(0x20));
  }
}

uint16_t floatingAverage(uint16_t aValue) {
  static uint16_t valueArray[KPhotoArraySize];
  static uint8_t index = 0;
  uint16_t sum;
  valueArray[index] = aValue;
  index++;
  if (index >= KPhotoArraySize) index = 0;
  sum = 0;
  for (uint8_t i=0 ; i<KPhotoArraySize ; i++ ) {
    sum = sum + valueArray[i];  
  }
  return(sum / KPhotoArraySize );
}

void doHeartBeat() {
  uint32_t currentTime = millis();
  static uint32_t previousTime = currentTime;
  static bool ledOnB = false;
  const uint32_t KHeartBeatBlink = 48;
  const uint32_t KHeartBeatStandard = 1000;

  // once per interval
  if(!ledOnB && (currentTime - previousTime >= KHeartBeatStandard)) {
    digitalWrite(KPinLedInternal, HIGH);
    previousTime = currentTime;
    ledOnB = true;
  }
  
  // off after heartbeat
  if(ledOnB && (currentTime - previousTime >= KHeartBeatBlink)) {
    digitalWrite(KPinLedInternal, LOW);
    ledOnB = false;
  }
}

void handleSerialInput(uint32_t aInterval) {
  uint32_t timeCurrent = millis();
  static uint32_t timePrevious = timeCurrent;
  if (timeCurrent - timePrevious < aInterval) return; else timePrevious = timeCurrent;

  if (Serial.available()) {
    inputString = Serial.readStringUntil('\n');

    // trim() removes leading and trailing whitespace characters, including \r, \n, tabs, etc.
    inputString.trim();
    
    if (inputString.length() > 0) {
      inputStringCompletedB = true;
    }
  }

  if (inputStringCompletedB) {
    Serial.printf("----- Entry: %s\r\n", inputString.c_str());
    
    if (inputString == "on") {
      showAsciiB = true;
    } else if (inputString == "off") {
      showAsciiB = false;
    } else {
      // do nothing
    }
  }
  inputString = "";
  inputStringCompletedB = false;
}
