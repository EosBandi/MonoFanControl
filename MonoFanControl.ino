#include "pidController.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);


// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 6
#define TEMPERATURE_PRECISION 9


// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;


// Constants
const uint8_t cSdCardChipSelectPin = 10; // The pin where the SD card chip select is connected.
const uint16_t cUpdateDelay = 500; // The update every ~500ms.
const uint8_t cStartFanSpeed = 0x20; // The initial fan speed.
const float cControlTargetTemperature = 33.0f; // The maximum target temperature.

// Custom characters for the LCD display.
const uint8_t cFanCharacterMask[] PROGMEM = {
  0b01100,
  0b01100,
  0b00101,
  0b11011,
  0b11011,
  0b10100,
  0b00110,
  0b00110,
};
const char cFanCharacter = '\x05';
const uint8_t cTemp1CharacterMask[] PROGMEM = {
  0b01000,
  0b10100,
  0b10100,
  0b10100,
  0b10100,
  0b11101,
  0b11101,
  0b01000
};
const char cTemp1Character = '\x06';
const uint8_t cTemp2CharacterMask[] PROGMEM = {
  0b01000,
  0b10101,
  0b10101,
  0b10100,
  0b10100,
  0b11101,
  0b11101,
  0b01000
};
const char cTemp2Character = '\x07';

// The current fan speed.
uint8_t fanSpeed = cStartFanSpeed;

// The PID controller instance.
lr::PIDController pidController;

void setup()
{

        // set up the LCD's number of columns and rows:
        lcd.begin(8, 2);
        // Print a message to the LCD.
        
          // Add custom characters for the fan as bar display.
        uint8_t bars[] = { B10000, B11000, B11100, B11110, B11111 };
        uint8_t character[8];
        for (uint8_t i = 0; i < 5; ++i) {
            for (uint8_t j = 0; j < 8; ++j) character[j] = bars[i];
            lcd.createChar(i, character);
        }
        // Add custom characters from the masks.
        memcpy_P(character, cFanCharacterMask, 8);
        lcd.createChar(cFanCharacter, character);
        memcpy_P(character, cTemp1CharacterMask, 8);
        lcd.createChar(cTemp1Character, character);
        memcpy_P(character, cTemp2CharacterMask, 8);
        lcd.createChar(cTemp2Character, character);
        
        
        //updateTemperatureDisplay(11.1f, 22.2f);


        // Setup the PID controller.
        pidController.begin(8.0f, 1.0f, 0.2f);
        pidController.setOutputLimits(20.0f, 255.0f);
        pidController.setOutputReverse(true);
        pidController.setOutputDrift(-0.05f);
        pidController.setOutputValue(fanSpeed);
        pidController.setTargetValue(cControlTargetTemperature);


        Serial.begin(9600);
        Serial.println("Dallas Temperature IC Control Library Demo");
        


        pinMode(10, OUTPUT);
        analogWrite(10, 0x10);


        // Start up the library
        sensors.begin();

        // locate devices on the bus
        Serial.print("Locating devices...");
        Serial.print("Found ");
        Serial.print(sensors.getDeviceCount(), DEC);
        Serial.println(" devices.");

        // report parasite power requirements
        Serial.print("Parasite power is: ");
        if (sensors.isParasitePowerMode()) Serial.println("ON");
        else Serial.println("OFF");


        // Must be called before search()
        oneWire.reset_search();
        // assigns the first address found to insideThermometer
        if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");


        // show the addresses we found on the bus
        Serial.print("Device 0 Address: ");
        printAddress(insideThermometer);
        Serial.println();


}

// Add the main program code into the continuous loop() function
void loop()
{


    sensors.requestTemperatures();
    Serial.print("Temp:");
    float temperature1 = sensors.getTempC(insideThermometer);
    Serial.println(temperature1);

    updateTemperatureDisplay(temperature1,0);



    // Calculate the new fan speed using the PID controller.
    //const float maximumTemperature = max(temperature1, temperature2);
    fanSpeed = static_cast<uint8_t>(pidController.calculateOutput(temperature1));
    analogWrite(10, fanSpeed);

    Serial.print("Fan:");
    Serial.println(fanSpeed);



    // Refresh the display.
    updateFanSpeedDisplay();
    

    delay(cUpdateDelay);


}

/// Print a single bar character of the given value (0-5).
///
/// @param value The bar value to display (0-5).
///
void lcdPrintBar(const uint8_t value) {
    if (value == 0) {
        lcd.print(' ');
    }
    else {
        lcd.print(static_cast<char>(value - 1));
    }
}

/// Update the fan speed display.
///
void updateFanSpeedDisplay() {
    lcd.setCursor(0, 1);
    lcd.print(cFanCharacter);
    const uint8_t displayedValue = (fanSpeed >> 4);
    if (displayedValue > 10) {
        lcdPrintBar(5);
        lcdPrintBar(5);
        lcdPrintBar(displayedValue - 10);
    }
    else if (displayedValue > 5) {
        lcdPrintBar(5);
        lcdPrintBar(displayedValue - 5);
        lcdPrintBar(0);
    }
    else {
        lcdPrintBar(displayedValue);
        lcdPrintBar(0);
        lcdPrintBar(0);
    }
}



/// Update the temperature display
///
/// @param temperature1 The first temperature value in degree celsius.
/// @param temperature2 The second temperature value in degree celsius.
///
void updateTemperatureDisplay(float temperature1, float temperature2) {
    lcd.setCursor(0, 0);
    lcd.print(cTemp2Character);
    lcd.print(temperature1);
    lcd.print('\xdf');
    lcd.print(' ');
    //lcd.print(cTemp2Character);
    //lcd.print(static_cast<uint8_t>(round(temperature2)), DEC);
    //lcd.print('\xdf');
    //lcd.print('C');
}


/// Print a decimal number padded with an optional zero to the LCD.
///
/// @param value The value to display.
///
void lcdPrintPaddedDecimal(uint8_t value) {
    if (value < 10) {
        lcd.print('0');
    }
    lcd.print(value, DEC);
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        // zero pad the address if necessary
        if (deviceAddress[i] < 16) Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
    float tempC = sensors.getTempC(deviceAddress);
    if (tempC == DEVICE_DISCONNECTED_C)
    {
        Serial.println("Error: Could not read temperature data");
        return;
    }
    Serial.print("Temp C: ");
    Serial.print(tempC);
    Serial.print(" Temp F: ");
    Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
    Serial.print("Resolution: ");
    Serial.print(sensors.getResolution(deviceAddress));
    Serial.println();
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
    Serial.print("Device Address: ");
    printAddress(deviceAddress);
    Serial.print(" ");
    printTemperature(deviceAddress);
    Serial.println();
}
