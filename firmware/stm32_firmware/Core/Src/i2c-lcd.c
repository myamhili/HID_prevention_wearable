/**
  ******************************************************************************
  * @file           : i2c-lcd.c
  * @brief          : I2C LCD driver
  ******************************************************************************
  */
#include "i2c-lcd.h"
#include "main.h"

/* Private variables ---------------------------------------------------------*/
static I2C_HandleTypeDef *g_hi2c;

/* Private defines -----------------------------------------------------------*/
#define LCD_ADDRESS (0x3E << 1) // 7-bit 0x3E
#define RGB_ADDRESS (0x62 << 1) // 7-bit 0x62
#define I2C_TIMEOUT 100

// LCD Commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20
#define LCD_SETDDRAMADDR 0x80
#define LCD_DISPLAYON 0x04
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00

// RGB Backlight Registers
#define REG_MODE1 0x00
#define REG_MODE2 0x01
#define REG_OUTPUT 0x08
#define REG_RED 0x04
#define REG_GREEN 0x03
#define REG_BLUE 0x02

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef set_rgb_register(uint8_t reg, uint8_t value);


/* Public functions ----------------------------------------------------------*/

/**
 * @brief Write a single command to the LCD.
 */
HAL_StatusTypeDef lcd_send_cmd(char cmd)
{
    uint8_t data_t[2];
    data_t[0] = 0x80; // Co=1, RS=0 (Command mode)
    data_t[1] = cmd;
    return HAL_I2C_Master_Transmit(g_hi2c, LCD_ADDRESS, data_t, 2, I2C_TIMEOUT);
}

/**
 * @brief Write a single data character to the LCD.
 */
HAL_StatusTypeDef lcd_send_data(char data)
{
    uint8_t data_t[2];
    data_t[0] = 0x40; // Co=0, RS=1 (Data mode)
    data_t[1] = data;
    return HAL_I2C_Master_Transmit(g_hi2c, LCD_ADDRESS, data_t, 2, I2C_TIMEOUT);
}

/**
 * @brief Initialize the LCD display.
 */
HAL_StatusTypeDef lcd_init(I2C_HandleTypeDef *hi2c)
{
    g_hi2c = hi2c;
    HAL_StatusTypeDef status = HAL_OK;

    // Wait for the LCD to power up
    HAL_Delay(50);

    // Standard HD44780 init sequence
    // If any of these fail, we stop and return the error
    status = lcd_send_cmd(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS);
    if(status != HAL_OK) return status;
    HAL_Delay(5);

    status = lcd_send_cmd(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
    if(status != HAL_OK) return status;
    HAL_Delay(5);

    status = lcd_send_cmd(LCD_CLEARDISPLAY);
    if(status != HAL_OK) return status;
    HAL_Delay(5);

    status = lcd_send_cmd(LCD_ENTRYMODESET | 0x02); // 0x02 = Entry Left

    return status;
}

void lcd_send_string(char *str)
{
    while (*str)
    {
        // Stop sending if one of the characters fails
        if(lcd_send_data(*str++) != HAL_OK) break;
    }
}

void lcd_clear(void)
{
    lcd_send_cmd(LCD_CLEARDISPLAY);
    HAL_Delay(2);
}

void lcd_set_cursor(int col, int row)
{
    int row_offsets[] = { 0x00, 0x40 };
    if(row > 1) row = 1; // Clamp to max 2 rows
    lcd_send_cmd(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

/**
 * @brief Set the backlight color.
 */
HAL_StatusTypeDef lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    HAL_StatusTypeDef status;

    status = set_rgb_register(REG_MODE1, 0x00);
    if(status != HAL_OK) return status;

    status = set_rgb_register(REG_OUTPUT, 0xAA); // Set all outputs to ON (PWM-controlled)
    if(status != HAL_OK) return status;

    status = set_rgb_register(REG_MODE2, 0x00);
    if(status != HAL_OK) return status;

    status = set_rgb_register(REG_RED, r);
    if(status != HAL_OK) return status;

    status = set_rgb_register(REG_GREEN, g);
    if(status != HAL_OK) return status;

    status = set_rgb_register(REG_BLUE, b);
    return status;
}

/**
 * @brief Write a byte to an RGB register. (Internal function)
 */
static HAL_StatusTypeDef set_rgb_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return HAL_I2C_Master_Transmit(g_hi2c, RGB_ADDRESS, data, 2, I2C_TIMEOUT);
}
