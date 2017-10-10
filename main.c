#include "main.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/debug_usart.h"
#include "lib/debug_hw.h"
#include "lib/ONE_hw.h"
#include "lib/led_pwm.h"
#include "usbd/usbd_main.h"

// private fn prototypes
static void SystemClock_Config(void);
static void Error_Handler(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);

uint32_t kStartExecutionAddress = 0x08010000;

typedef void (*pFunc)(void);
void JumpTo(uint32_t address) {
	// deinit USB here!!!

	// Deinit open drivers
	Debug_HW_Deinit();
	Debug_USART_Deinit();
	ONE_HW_Deinit();

	HAL_DeInit();

    // Reinitialize the Stack pointer
    __set_MSP(*(__IO uint32_t*) address);
    // jump to application address
    ((pFunc) (*(__IO uint32_t*) (address + 4)))();
    
    while(1){}
}

// exported fns
uint32_t main(void)
{
	// Configure low-level
	MPU_Config();
	CPU_CACHE_Enable();
	HAL_Init(); // Flash ART accelerator on ITCM interface?!
	SystemClock_Config();

	// HW initialization
	Debug_HW_Init();
    Debug_USART_Init();
	Debug_USART_printf("dfu bootload\n\r");
	ONE_HW_Init();

	uint8_t tmp = 0;
	ONE_getstates( &tmp );
	if( tmp != 1 ){
		JumpTo(kStartExecutionAddress);
	}
	uint8_t arm = 0;
	while( tmp <= 1 
		&& arm == 0
		 ){
		PWM_set_level( MOTOR_L, 1.0 );

		// init USB & check if present
		usbd_main();

		//run_bootloader();
		
		PWM_step();
		// exit with keypress (should protect against this when programming)
		if( ONE_getstates( &tmp )
		 && tmp ){
			JumpTo(kStartExecutionAddress);
		}
	}



	

	JumpTo(kStartExecutionAddress);

	while(1){}
	return 0;
}

// LOW LEVEL SYS INIT
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef ret = HAL_OK;
	
	// Enable Power Control clock
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	// Enable HSE Oscillator and activate PLL with HSE as source
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 432;  // for 216MHz use 432 !!!!!!!!!!
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;
	RCC_OscInitStruct.PLL.PLLR = 7;  

	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if(ret != HAL_OK) { Error_Handler(); }

	// Activate the OverDrive to reach the 216 MHz Frequency
	ret = HAL_PWREx_EnableOverDrive();
	if(ret != HAL_OK) { Error_Handler(); }

	// Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
	if(ret != HAL_OK) { Error_Handler(); }
}

static void Error_Handler(void)
{
	while(1) {
		;;
	}
}

static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;
	
	// Disable the MPU
	HAL_MPU_Disable();

	// Configure the MPU attributes as WT for SRAM
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = 0x20020000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	// Enable the MPU
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void CPU_CACHE_Enable(void)
{
	// Enable I-Cache
	SCB_EnableICache();

	// Enable D-Cache
	SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT
// Reports the name of the source file and the source line number
void assert_failed(uint8_t* file, uint32_t line)
{ 
	// printf the failure: "error in %file at line %line"
	// Infinite loop (just freezing now)
	while (1) {
		;;
	}
}
#endif
