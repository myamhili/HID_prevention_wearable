#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
const int B_CONST = 4275;
const float R0_CONST = 100000.0;
const float ADC_MAX = 4095.0;
#define RX_BUF_SIZE 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t adc_buf[4];
char tx_buf[100];
uint8_t rx_char;
uint8_t rx_buf[RX_BUF_SIZE];
volatile int rx_idx = 0;
volatile uint8_t command_ready_flag = 0;
uint32_t lastSendTime = 0;
const uint32_t sendInterval = 100;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void sendSensorData(void);
void parseCommand(char *cmd);
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
  (void)HAL_I2C_GetState(&hi2c1);

  lcd_init(&hi2c1);
  lcd_set_rgb(0, 100, 255);
  lcd_set_cursor(0, 0);
  lcd_send_string("System Online.");
  lcd_set_cursor(0, 1);
  lcd_send_string("Waiting for PC...");

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 4) != HAL_OK)
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
	if (now - lastSendTime >= sendInterval)
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
void sendSensorData(void)
{
	uint16_t temp_adc_val = adc_buf[0];
	uint16_t accel_x_val = adc_buf[1];
	uint16_t accel_y_val = adc_buf[2];
	uint16_t accel_z_val = adc_buf[3];

	float temp_C = -99.0;
	if (temp_adc_val > 0)
	{
		float R_thermistor = R0_CONST * (ADC_MAX / (float)temp_adc_val - 1.0);
		float log_R = log(R_thermistor / R0_CONST);
		float temp_K = 1.0 / (log_R / B_CONST + 1.0 / 298.15);
		temp_C = temp_K - 273.15;
	}

	/* Format the data string to match Arduino format: T:temp,X:x,Y:y,Z:z */
	int len = sprintf(tx_buf, "T:%.1f,X:%d,Y:%d,Z:%d\n",
	                    temp_C,
	                    (int)accel_x_val,
	                    (int)accel_y_val,
	                    (int)accel_z_val);
	HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, len, 100);
}

/**
 * @brief Processes a command string from the Python backend.
 * @param cmd The null-terminated command string to parse.
 * @details Supports the following commands:
 *   - RGB:r,g,b - Set LCD backlight color
 *   - L:line1|line2 - Display text on LCD (line2 optional)
 */
void parseCommand(char *cmd)
{
	/* Find the colon separator */
	char *commandValue = strchr(cmd, ':');
	if (commandValue == NULL)
	{
		HAL_UART_Transmit(&huart2, (uint8_t*)"ERR:Invalid format\n", 20, 100);
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
			lcd_set_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
			HAL_UART_Transmit(&huart2, (uint8_t*)"ACK:RGB\n", 9, 100);
		}
		else
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)"ERR:RGB parse failed\n", 22, 100);
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
			lcd_send_string(commandValue);
		}
		else
		{
			/* Two lines of text, split by '|' */
			*line2 = '\0'; /* Terminate the first line */
			char *line1 = commandValue;
			line2++; /* Move pointer to the start of the second line */

			lcd_set_cursor(0, 0);
			lcd_send_string(line1);
			lcd_set_cursor(0, 1);
			lcd_send_string(line2);
		}
		HAL_UART_Transmit(&huart2, (uint8_t*)"ACK:L\n", 7, 100);
	}
	else
	{
		HAL_UART_Transmit(&huart2, (uint8_t*)"ERR:Unknown command\n", 21, 100);
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
				rx_buf[rx_idx] = '\0';
				command_ready_flag = 1;
			}
		}
		else
		{
			if (rx_idx < RX_BUF_SIZE - 1)
			{
				rx_buf[rx_idx++] = rx_char;
			}
		}
		HAL_UART_Receive_IT(&huart2, &rx_char, 1);
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
	  HAL_Delay(50);
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
