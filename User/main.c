/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   液晶显示BMP图片
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 iSO STM32 开发板 
  * 论坛    :http://www.chuxue123.com
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
	*/
#include "stm32f10x.h"
#include "bsp_usart1.h"
#include "bsp_sdfs_app.h"
#include "bsp_exti.h"
#include "bsp_led.h"
#include "ff.h"
#include "delay.h"
#include "bsp_sdio_sdcard.h"
#include "save.h"
#include "bsp_spi_flash.h"
#include "bsp_ov7725.h"
#include "zig.h"
#include "bsp_rtc.h"
#include "time2.h"

#define CLI()      __set_PRIMASK(1)		/* 关闭总中断 */  
#define SEI() __set_PRIMASK(0)				/* 开放总中断 */ 
/* 
** flash定义区
*/    
/*获取缓冲区的长度*/
typedef enum { FAILED = 0, PASSED = !FAILED} TestStatus;

#define TxBufferSize1   (countof(TxBuffer1) - 1)
#define RxBufferSize1   (countof(TxBuffer1) - 1)
#define countof(a)      (sizeof(a) / sizeof(*(a)))
#define  BufferSize (countof(Tx_Buffer))

#define  FLASH_WriteAddress     0x00010
#define  FLASH_ReadAddress      FLASH_WriteAddress
#define  FLASH_SectorToErase    FLASH_WriteAddress
//#define  sFLASH_ID              0xEF3015     //W25X16
//#define  sFLASH_ID              0xEF4015	 //W25Q16
#define  sFLASH_ID              0XEF4017    //W25Q64
/* 发送缓冲区初始化 */
//uint8_t Tx_Buffer[] = " 感谢您选用野火stm32开发板\r\n                http://firestm32.taobao.com";
//uint8_t Rx_Buffer[BufferSize];
uint8_t Tx_Buffer[1]={0};
uint8_t Rx_Buffer[1];

__IO uint32_t DeviceID = 0;
__IO uint32_t FlashID = 0;
__IO TestStatus TransferStatus1 = FAILED;

/*时间结构体*/
struct rtc_time systmtime;
uint8_t time_data[24];
uint16_t time_year,time_month,time_day,time_hour,time_minute,time_second,time_y;
void Time_data_set(void);
/*定时*/
volatile uint32_t timer;    //计时用
/*
 * 全局中断优先级的高低：按键 > sd卡 > 摄像头场中断   
 */
extern volatile uint8_t Ov7725_vsync ;
volatile uint8_t usart_flag;
volatile uint8_t screen_flag=0;
extern EXTI_InitTypeDef EXTI_InitStructure;
uint8_t dest_address[8]={0x00,0x13,0xA2,0x00,0x40,0xEA,0x13,0xE7};
uint8_t status_flag=0x30;

	  uint8_t file_name[30];   //0804 change in order to test
	  uint8_t save_name[30];



 int main(void)
{

		uint8_t send_name[30];
		uint8_t file_name[30];
	  uint8_t save_name[30];
    uint8_t num1 = 0;
//		uint8_t num0 = 0;
//		uint8_t packet_length;
		
   /* USART1 config */
		USART1_Config();
//	  printf("\r\n start \r\n");
		NVIC_Configuration1();   //串口中断等级为0,2
	  /*按键初始化*/
	  EXTI_PB6_Config();    //中断等级为0,1

		/* LED 初始化 */
		LED_GPIO_Config();
		LED1_OFF;
		LED2_OFF;
	
	/* 8M串行flash W25X64初始化 */
	  SPI_FLASH_Init();
		DeviceID = SPI_FLASH_ReadDeviceID();
		delay(200);
		FlashID = SPI_FLASH_ReadID();	
				
		if (FlashID == sFLASH_ID)  /* #define  sFLASH_ID  0XEF4017 */
		{	
			/**读取保存在flash中的num**/
			SPI_FLASH_BufferRead(Rx_Buffer, FLASH_ReadAddress, BufferSize);
			num1=Rx_Buffer[0];
		}

		/* 初始化sd卡文件系统，因为汉字的字库放在了sd卡里面 */
		Sd_fs_init();	  //中断等级为0,0
	  /*--------------------------------------------------------------------------------------------------------*/	
		/* ov7725 gpio 初始化 */
		Ov7725_GPIO_Config();
	
		/* ov7725 寄存器配置初始化 */
		while(Ov7725_Init() != SUCCESS);
			/* ov7725 场信号线初始化 */
		VSYNC_Init();	 //中断等级为0,3
		Ov7725_vsync = 0;
		
		/*RTC初始化*/
		RTC_NVIC_Config(); //中断等级为1,2
		/*定时初始化*/
		Tim2_Config( );
		TIM2_NVIC_Configuration();//0,4
//		packet_length=Tramsmit_packet(ask_buffer,ask,dest_address,3);   
		
		while(1)
		{
			  
        /* screen_flag 在按键中断函数里面置位 */
      if( screen_flag == 1 )
       { 	
				  								 
					Ov7725_vsync = 0;
					while(Ov7725_vsync !=2);
					num1++;	       
				 
					Time_Show(&systmtime);
				  time_year=systmtime.tm_year;
				  time_month=systmtime.tm_mon; 
				  time_day=systmtime.tm_mday;
				  time_hour=systmtime.tm_hour;
				  time_minute=systmtime.tm_min;
				  time_second=systmtime.tm_sec;			
					
				  sprintf((char *)file_name,"%0.2d%0.2d%0.2d%0.2d%0.2d%0.2d",time_year,time_month, time_day,time_hour,time_minute,time_second);    
			 
					sprintf((char*)save_name,"0:/%s.bmp",file_name);	
				 
//		       sprintf((char *)send_name,"/txt%d.bmp",num0);					
//				  sprintf((char*)save_name,"0:%s",send_name);
          FIFO_PREPARE;  			/*FIFO准备*/
				  FIFO_PREPARE;  			/*FIFO准备*/
 				  bmp_sd_save(320,240,save_name);//保存图像
 
			  	set_timestamp (	(char *)save_name, systmtime.tm_year,	systmtime.tm_mon,	systmtime.tm_mday,systmtime.tm_hour,systmtime.tm_min,systmtime.tm_sec);
					
					LED2_TOGGLE;				 
        /* 截图完毕LED2取反 */  
        
         screen_flag = 2;
       
				 	/**烧写flash**/
					SPI_FLASH_SectorErase(FLASH_SectorToErase);		
					Tx_Buffer[0]=num1;					
					SPI_FLASH_BufferWrite(Tx_Buffer, FLASH_WriteAddress, BufferSize);
				 	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
       }

			 
			if(screen_flag==2)    //2016.04.21为实验所添加部分
			{
				if(status_flag==0x30)
				{screen_flag=3;}				
			}
			
			if(screen_flag==3)
			{		
			
					bmp_sd_send(save_name,file_name);	   //发送图像
					screen_flag=1;
					delay(100);
					LED1_TOGGLE;	

			}
		}
}




	  

