/**
 * @file R4_Minima_Controller.ino
 * @author Deacon Sham
 * @brief Main firmware for the Delirium Prevention Wearable on an Arduino R4 Minima.
 * @version 2.1
 * @date 2025-11-15
 *
 *
 * @details This firmware performs the following tasks:
 * 1. Reads 4 analog sensors (Temp, Accel X/Y/Z) at 10Hz.
 * 2. Formats sensor data into a comma-separated string.
 * 3. Sends this data packet over Serial (9600 baud) to a Python backend.
 * 4. Listens for commands from the backend to control the LCD text and backlight colour.
 */

#include <Wire.h>
#include "rgb_lcd.h"
#include <math.h>

rgb_lcd lcd;

const int tempPin = A0;
const int accelXPin = A1;
const int accelYPin = A2;
const int accelZPin = A3;

const int B_CONST = 4275;
const float R0_CONST = 100000.0;
const float ADC_MAX = 4095.0;

// Timing and communication constants
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100;
const long BAUD_RATE = 9600;

// Buffer for incoming serial commands
const byte RX_BUFFER_SIZE = 100;
char rx_buffer[RX_BUFFER_SIZE];
byte rx_buffer_index = 0;

// Default LCD colors
const int LCD_R = 0, LCD_G = 100, LCD_B = 255;

/**
 * @brief Initializes all hardware, peripherals, and serial communication.
 * @details Runs once at startup. Sets ADC resolution, initializes Serial, and sets up the LCD.
 */
void setup() {
  Serial.begin(BAUD_RATE);
  Wire.begin();
  analogReadResolution(12);

  lcd.begin(16, 2);
  lcd.setRGB(LCD_R, LCD_G, LCD_B);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Online!");
  lcd.setCursor(0, 1);
  lcd.print("Waiting for PC...");
}

/**
 * @brief Main application loop.
 * @details This loop runs continuously. It uses a non-blocking delay
 * (millis()) to send sensor data at a fixed interval and
 * constantly checks for new commands from the PC.
 */
void loop() {
  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;
    sendSensorData();
  }
  
  checkSerialCommands();
}

/**
 * @brief Reads all 4 analog sensors, calculates temperature, formats, and
 * sends the data packet over Serial.
 * @note  The data format is "T:temp,X:x,Y:y,Z:z\n".
 */
void sendSensorData() {
  /* Read all 4 analog pins */
  int temp_adc_val = analogRead(tempPin);
  int accel_x_val  = analogRead(accelXPin);
  int accel_y_val  = analogRead(accelYPin);
  int accel_z_val  = analogRead(accelZPin);
  
  /* Calculate temperature */
  float temp_C = -99.0;
  if (temp_adc_val > 0) {
    float R_thermistor = R0_CONST * (ADC_MAX / (float)temp_adc_val - 1.0);
    float log_R = log(R_thermistor / R0_CONST);
    float temp_K = 1.0 / (log_R / B_CONST + 1.0 / 298.15);
    temp_C = temp_K - 273.15;
  }

  /* Format the data string using a memory-safe char buffer */
  char tx_buffer[64];
  snprintf(tx_buffer, sizeof(tx_buffer), "T:%.1f,X:%d,Y:%d,Z:%d", 
           temp_C, accel_x_val, accel_y_val, accel_z_val);

  /* Send the packet with a newline */
  Serial.println(tx_buffer);
}

/**
 * @brief Checks if a complete command string has arrived over Serial.
 * @details If data is available, it reads until a newline character
 * and then passes the command to parseCommand().
 */
void checkSerialCommands() {
  while (Serial.available() > 0) {
    char receivedChar = Serial.read();

    if (receivedChar == '\n' || receivedChar == '\r') {
      if (rx_buffer_index > 0) {
        rx_buffer[rx_buffer_index] = '\0'; // Null-terminate the string
        parseCommand(rx_buffer);
        rx_buffer_index = 0; // Reset for next command
      }
    } else if (rx_buffer_index < RX_BUFFER_SIZE - 1) {
      rx_buffer[rx_buffer_index++] = receivedChar;
    }
    // If buffer overflows, the command is ignored until the next newline.
  }
}

/**
 * @brief Processes a command string from the Python backend.
 * @param cmd The null-terminated command string to parse.
 */
void parseCommand(char* cmd) {
  char* commandValue = strchr(cmd, ':');
  if (commandValue == nullptr) {
    Serial.println("ERR:Invalid format");
    return;
  }
  
  *commandValue = '\0'; // Split the string into commandType and commandValue
  char* commandType = cmd;
  commandValue++; // Move pointer to the start of the value

  if (strcmp(commandType, "RGB") == 0) {
    int r, g, b;
    if (sscanf(commandValue, "%d,%d,%d", &r, &g, &b) == 3) {
      lcd.setRGB(r, g, b);
      Serial.println("ACK:RGB");
    } else {
      Serial.println("ERR:RGB parse failed");
    }
  } else if (strcmp(commandType, "L") == 0) {
    lcd.clear();
    char* line2 = strchr(commandValue, '|');
    
    if (line2 == nullptr) {
      // Only one line of text
      lcd.setCursor(0, 0);
      lcd.print(commandValue);
    } else {
      // Two lines of text, split by '|'
      *line2 = '\0'; // Terminate the first line
      char* line1 = commandValue;
      line2++; // Move pointer to the start of the second line
      
      lcd.setCursor(0, 0);
      lcd.print(line1);
      lcd.setCursor(0, 1);
      lcd.print(line2);
    }
    Serial.println("ACK:L");
  } else {
    Serial.println("ERR:Unknown command");
  }
}