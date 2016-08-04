/**
  ******************************************************************************
  * @file    bsp_exti.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   I/O线中断应用bsp
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 iSO STM32 开发板 
  * 论坛    :http://www.chuxue123.com
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "bsp_exti.h"
#include "bsp_usart1.h"
EXTI_InitTypeDef EXTI_InitStructure;
 /**
  * @brief  配置嵌套向量中断控制器NVIC
  * @param  无
  * @retval 无
  */
static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  
  /* Configure one bit for preemption priority */
//  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  /* 配置P[A|B|C|D|E]0为中断源 */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

 /**
  * @brief  配置 PB0 为线中断口，并设置中断优先级
  * @param  无
  * @retval 无
  */
void EXTI_PB6_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
                                                                                        

	/* config the extiline(PB6) clock and AFIO clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);
												
	/* config the NVIC(PB6) */
	NVIC_Configuration();
	/* EXTI line gpio config(PB6) */	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;       
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	 // 上拉输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* EXTI line(PB0) mode config */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6); 
  EXTI_InitStructure.EXTI_Line = EXTI_Line6;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //下降沿中断
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure); 
}
/*********************************************END OF FILE**********************/
