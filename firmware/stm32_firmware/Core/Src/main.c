/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "i2c-lcd.h"
#include "usart.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define B_CONST 4275.0f
#define R0_CONST 100000.0f
#define ADC_MAX 4095.0f
#define RX_BUF_SIZE 100
#define SENSOR_CHANNEL_COUNT 4U
#define SENSOR_SEND_INTERVAL_MS 100U
#define UART_TIMEOUT_MS 100U
#define LCD_COLUMNS 16U
#define ERROR_BLINK_DELAY_CYCLES 500000UL
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t adc_buf[SENSOR_CHANNEL_COUNT];
char tx_buf[100];
uint8_t rx_char;
uint8_t rx_buf[RX_BUF_SIZE];
volatile int rx_idx = 0;
volatile uint8_t command_ready_flag = 0;
volatile uint8_t rx_overflow_flag = 0;
uint32_t lastSendTime = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void sendSensorData(void);
static void parseCommand(char *cmd);
static void sendUartString(const char *message);
static uint8_t clampRgbValue(int value);
static void showStartupScreen(void);
static void lcdSendLine(const char *text);
static void busyWait(volatile uint32_t cycles);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  if (lcd_init(&hi2c1) != HAL_OK)
  {
	  Error_Handler();
  }
  showStartupScreen();

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, SENSOR_CHANNEL_COUNT) != HAL_OK)
  {
	  Error_Handler();
  }

  if (HAL_UART_Receive_IT(&huart2, &rx_char, 1) != HAL_OK)
  {
	  Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	uint32_t now = HAL_GetTick();
	if (now - lastSendTime >= SENSOR_SEND_INTERVAL_MS)
	{
		lastSendTime = now;
		sendSensorData();
	}

	if (command_ready_flag)
	{
		parseCommand((char*)rx_buf);
		rx_idx = 0;
		memset(rx_buf, 0, RX_BUF_SIZE);
		command_ready_flag = 0;
	}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void sendSensorData(void)
{
	uint16_t temp_adc_val = adc_buf[0];
	uint16_t accel_x_val = adc_buf[1];
	uint16_t accel_y_val = adc_buf[2];
	uint16_t accel_z_val = adc_buf[3];

	float temp_C = -99.0;
	if (temp_adc_val > 0)
	{
		float R_thermistor = R0_CONST * (ADC_MAX / (float)temp_adc_val - 1.0f);
		float log_R = logf(R_thermistor / R0_CONST);
		float temp_K = 1.0f / (log_R / B_CONST + 1.0f / 298.15f);
		temp_C = temp_K - 273.15f;
	}

	/* Format the data string to match Arduino format: T:temp,X:x,Y:y,Z:z */
	int len = snprintf(tx_buf, sizeof(tx_buf), "T:%.1f,X:%u,Y:%u,Z:%u\n",
	                    temp_C,
	                    (unsigned int)accel_x_val,
	                    (unsigned int)accel_y_val,
	                    (unsigned int)accel_z_val);
	if (len > 0 && len < (int)sizeof(tx_buf))
	{
		HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, (uint16_t)len, UART_TIMEOUT_MS);
	}
}

/**
 * @brief Processes a command string from the Python backend.
 * @param cmd The null-terminated command string to parse.
 * @details Supports the following commands:
 *   - RGB:r,g,b - Set LCD backlight color
 *   - L:line1|line2 - Display text on LCD (line2 optional)
 */
static void parseCommand(char *cmd)
{
	/* Find the colon separator */
	char *commandValue = strchr(cmd, ':');
	if (commandValue == NULL)
	{
		sendUartString("ERR:Invalid format\n");
		return;
	}

	/* Split the string into commandType and commandValue */
	*commandValue = '\0';
	char *commandType = cmd;
	commandValue++; /* Move pointer to the start of the value */

	/* Handle RGB command */
	if (strcmp(commandType, "RGB") == 0)
	{
		int r, g, b;
		if (sscanf(commandValue, "%d,%d,%d", &r, &g, &b) == 3)
		{
			if (lcd_set_rgb(clampRgbValue(r), clampRgbValue(g), clampRgbValue(b)) == HAL_OK)
			{
				sendUartString("ACK:RGB\n");
			}
			else
			{
				sendUartString("ERR:RGB write failed\n");
			}
		}
		else
		{
			sendUartString("ERR:RGB parse failed\n");
		}
	}
	/* Handle LCD text display command */
	else if (strcmp(commandType, "L") == 0)
	{
		lcd_clear();
		char *line2 = strchr(commandValue, '|');

		if (line2 == NULL)
		{
			/* Only one line of text */
			lcd_set_cursor(0, 0);
			lcdSendLine(commandValue);
		}
		else
		{
			/* Two lines of text, split by '|' */
			*line2 = '\0'; /* Terminate the first line */
			char *line1 = commandValue;
			line2++; /* Move pointer to the start of the second line */

			lcd_set_cursor(0, 0);
			lcdSendLine(line1);
			lcd_set_cursor(0, 1);
			lcdSendLine(line2);
		}
		sendUartString("ACK:L\n");
	}
	else
	{
		sendUartString("ERR:Unknown command\n");
	}
}

static void sendUartString(const char *message)
{
	HAL_UART_Transmit(&huart2, (uint8_t*)message, (uint16_t)strlen(message), UART_TIMEOUT_MS);
}

static uint8_t clampRgbValue(int value)
{
	if (value < 0)
	{
		return 0;
	}
	if (value > 255)
	{
		return 255;
	}
	return (uint8_t)value;
}

static void showStartupScreen(void)
{
	(void)lcd_set_rgb(0, 100, 255);
	lcd_clear();
	lcd_set_cursor(0, 0);
	lcdSendLine("System Online!");
	lcd_set_cursor(0, 1);
	lcdSendLine("Waiting for PC...");
}

static void lcdSendLine(const char *text)
{
	uint8_t chars_written = 0;
	while (*text != '\0' && chars_written < LCD_COLUMNS)
	{
		if (lcd_send_data(*text++) != HAL_OK)
		{
			break;
		}
		chars_written++;
	}
}

static void busyWait(volatile uint32_t cycles)
{
	while (cycles-- > 0U)
	{
		__NOP();
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		if (rx_char == '\n' || rx_char == '\r')
		{
			if (rx_idx > 0)
			{
				if (!rx_overflow_flag)
				{
					rx_buf[rx_idx] = '\0';
					command_ready_flag = 1;
				}
				else
				{
					rx_overflow_flag = 0;
					rx_idx = 0;
				}
			}
		}
		else
		{
			if (!command_ready_flag && !rx_overflow_flag && rx_idx < RX_BUF_SIZE - 1)
			{
				rx_buf[rx_idx++] = rx_char;
			}
			else if (!command_ready_flag)
			{
				rx_overflow_flag = 1;
			}
		}
		if (HAL_UART_Receive_IT(&huart2, &rx_char, 1) != HAL_OK)
		{
			Error_Handler();
		}
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	  busyWait(ERROR_BLINK_DELAY_CYCLES);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
