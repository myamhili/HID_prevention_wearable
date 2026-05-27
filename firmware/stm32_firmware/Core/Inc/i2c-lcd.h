/**
  ******************************************************************************
  * @file           : i2c-lcd.h
  * @brief          : I2C LCD driver
  ******************************************************************************
  */
#ifndef INC_I2C_LCD_H_
#define INC_I2C_LCD_H_

#include "stm32f4xx_hal.h"

/**
  * @brief  Initializes the LCD and RGB controller.
  * @param  hi2c: Pointer to an I2C_HandleTypeDef structure.
  * @retval HAL_StatusTypeDef (HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT)
  */
HAL_StatusTypeDef lcd_init(I2C_HandleTypeDef *hi2c);

/**
  * @brief  Sets the RGB backlight color.
  * @param  r, g, b: Red, Green, Blue values (0-255)
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
  * @brief  Sends a command byte to the LCD.
  * @param  cmd: The command byte to send.
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef lcd_send_cmd(char cmd);

/**
  * @brief  Sends a data character to the LCD.
  * @param  data: The character to display.
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef lcd_send_data(char data);

/**
  * @brief  Sends a string to the LCD.
  * @param  str: Pointer to the string to display.
  * @retval None
  */
void lcd_send_string(char *str);

/**
  * @brief  Clears the LCD display.
  * @retval None
  */
void lcd_clear(void);

/**
  * @brief  Sets the cursor position.
  * @param  col: Column (0-15)
  * @param  row: Row (0-1)
  * @retval None
  */
void lcd_set_cursor(int col, int row);

#endif /* INC_I2C_LCD_H_ */
