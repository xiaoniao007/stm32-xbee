#include "save.h"
#include "ff.h"
#include "bsp_usart1.h"
#include "delay.h"
#include "bsp_ov7725.h"
#include "bsp_sccb.h"
#include "zig.h"
#include "bsp_led.h"
/* 如果不需要打印bmp相关的提示信息,将printf注释掉即可
 * 如要用printf()，需将串口驱动文件包含进来
 */
#define BMP_DEBUG_PRINTF(FORMAT,...)  printf(FORMAT,##__VA_ARGS__)
extern volatile uint8_t usart_flag;
extern volatile uint8_t Ov7725_vsync ;
extern uint8_t dest_address[8];

uint8_t pColorData[960];					/* 一行真彩色数据缓存 320 * 3 = 960 */
uint8_t bmp_data[120];						/*图片数据缓存区*/
uint8_t pack_data[200];						/*数据传输包*/
uint8_t pack_head[100];						/*数据头传输包*/
uint8_t pack_time[50];						/*文件名（时间）传输包*/

extern volatile uint32_t timer;
/*文件系统*/
FATFS bmpfs[2]; 
FIL bmpfsrc, bmpfdst; 
FRESULT bmpres;
UINT bw;

/* bmp  文件头 54个字节 */
unsigned char bmp_head[54] =
	{
		0x42, 0x4d, 0, 0, 0, 0, 
		0, 0, 0, 0, 54, 0, 
		0, 0, 40,0, 0, 0, 
		0, 0, 0, 0, 0, 0, 
		0, 0, 1, 0, 24, 0, 
		0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 
		0, 0, 0
	};
	
	/* 处理保存BMP文件的头信息 */
static void headerdeal(BITMAPFILEHEADER* pBmpHead,tagBITMAPINFOHEADER* pBmpInforHead)
{
		/* 文件大小 4个字节 */	
		bmp_head[2] = (unsigned char)((*pBmpHead).bfSize &0x000000ff);
	  bmp_head[3] = ((*pBmpHead).bfSize >> 8) & 0x000000ff;
	  bmp_head[4] = ((*pBmpHead).bfSize >> 16) & 0x000000ff;
	  bmp_head[5] = ((*pBmpHead).bfSize >> 24) & 0x000000ff;
	  /*实际位图数据的偏移字节数*/
		bmp_head[10] = (*pBmpHead).bfOffBits &0x000000ff;
	  bmp_head[11] = ((*pBmpHead).bfOffBits >> 8) & 0x000000ff;
	  bmp_head[12] = ((*pBmpHead).bfOffBits >> 16) & 0x000000ff;
	  bmp_head[13] = ((*pBmpHead).bfOffBits >> 24) & 0x000000ff;	
	
	  /*指定此结构体的长度*/
		bmp_head[14] = (*pBmpInforHead).biSize & 0x000000ff;
	  bmp_head[15] = ((*pBmpInforHead).biSize >> 8) &0x000000ff;
	  bmp_head[16] = ((*pBmpInforHead).biSize>> 16) &0x000000ff;
	  bmp_head[17] = ((*pBmpInforHead).biSize >> 24) &0x000000ff;
		/* 位图宽 4个字节 */
		bmp_head[18] = (*pBmpInforHead).biWidth & 0x000000ff;
	  bmp_head[19] = ((*pBmpInforHead).biWidth >> 8) &0x000000ff;
	  bmp_head[20] = ((*pBmpInforHead).biWidth >> 16) &0x000000ff;
	  bmp_head[21] = ((*pBmpInforHead).biWidth >> 24) &0x000000ff;
	  /* 位图高 4个字节 */
		bmp_head[22] = (*pBmpInforHead).biHeight &0x000000ff;
	  bmp_head[23] = ((*pBmpInforHead).biHeight >> 8) &0x000000ff;
	  bmp_head[24] = ((*pBmpInforHead).biHeight >> 16) &0x000000ff;
	  bmp_head[25] = ((*pBmpInforHead).biHeight >> 24) &0x000000ff;	
	  /* 平面数 */
		bmp_head[26] = (*pBmpInforHead).biPlanes &0x00ff;
	  bmp_head[27] = ((*pBmpInforHead).biPlanes >> 8) &0x00ff;
	  /* 采用颜色位数 */
		bmp_head[28] = (*pBmpInforHead).biBitCount &0x00ff;
	  bmp_head[29] = ((*pBmpInforHead).biBitCount >> 8) &0x00ff;	
	  /* 压缩方式*/
		bmp_head[30] = (*pBmpInforHead).biCompression &0x000000ff;
	  bmp_head[31] = ((*pBmpInforHead).biCompression >> 8) &0x000000ff;
	  bmp_head[32] = ((*pBmpInforHead).biCompression >> 16) &0x000000ff;
	  bmp_head[33] = ((*pBmpInforHead).biCompression >> 24) &0x000000ff;	
	  /*实际位图数据占用的字节数*/
		bmp_head[34] = (*pBmpInforHead).biSizeImage &0x000000ff;
	  bmp_head[35] = ((*pBmpInforHead).biSizeImage >> 8) &0x000000ff;
	  bmp_head[36] = ((*pBmpInforHead).biSizeImage >> 16) &0x000000ff;
	  bmp_head[37] = ((*pBmpInforHead).biSizeImage >> 24) &0x000000ff;	
		/*X方向分辨率	*/
		bmp_head[38] = (*pBmpInforHead).biXPelsPerMeter &0x000000ff;
	  bmp_head[39] = ((*pBmpInforHead).biXPelsPerMeter >> 8) &0x000000ff;
	  bmp_head[40] = ((*pBmpInforHead).biXPelsPerMeter >> 16) &0x000000ff;
	  bmp_head[41] = ((*pBmpInforHead).biXPelsPerMeter >> 24) &0x000000ff;	
		/*Y方向分辨率	*/
		bmp_head[42] = (*pBmpInforHead).biYPelsPerMeter &0x000000ff;
	  bmp_head[43] = ((*pBmpInforHead).biYPelsPerMeter >> 8) &0x000000ff;
	  bmp_head[44] = ((*pBmpInforHead).biYPelsPerMeter >> 16) &0x000000ff;
	  bmp_head[45] = ((*pBmpInforHead).biYPelsPerMeter >> 24) &0x000000ff;		
		/*使用的颜色数	*/
		bmp_head[46] = (*pBmpInforHead).biClrUsed &0x000000ff;
	  bmp_head[47] = ((*pBmpInforHead).biClrUsed >> 8) &0x000000ff;
	  bmp_head[48] = ((*pBmpInforHead).biClrUsed >> 16) &0x000000ff;
	  bmp_head[49] = ((*pBmpInforHead).biClrUsed >> 24) &0x000000ff;				
		/*重要颜色数	*/
		bmp_head[50] = (*pBmpInforHead).biClrImportant &0x000000ff;
	  bmp_head[51] = ((*pBmpInforHead).biClrImportant >> 8) &0x000000ff;
	  bmp_head[52] = ((*pBmpInforHead).biClrImportant >> 16) &0x000000ff;
	  bmp_head[53] = ((*pBmpInforHead).biClrImportant >> 24) &0x000000ff;			
}


/*
 * 发送bmp图片, 24位真彩色
 * 图片宽度和高度根据图片大小而定
 */
void bmp_sd_send(unsigned char *pic_name,unsigned char *file_name)
{
	int i, j;
	int width, height;
	int timer_length;  //转换成字符串后数据的偿长度
	BITMAPFILEHEADER bitHead;
	BITMAPINFOHEADER bitInfoHead;
	WORD fileType;
  uint16_t packet_length;
	unsigned int read_num;
	unsigned char time_write[20];  //定时写入
//	sprintf((char*)tmp_name,"0:/%s.bmp",pic_name);  
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);		/*先打开定时*/
	
	/* 发送文件名*/
		packet_length=Tramsmit_packet(pack_time,file_name,dest_address,14);
		for(i=0;i<packet_length;i++)	   
				{
					printf("%c",pack_time[i]);
				}
	/* 打开文件*/
	
	f_mount(0, &bmpfs[0]);
	bmpres = f_open( &bmpfsrc , (char *)pic_name, FA_OPEN_EXISTING | FA_READ);	
/*-------------------------------------------------------------------------------------------------------*/
	if(bmpres == FR_OK)
	{
		/* 读取文件头信息  两个字节*/         
		f_read(&bmpfsrc,&fileType,sizeof(WORD),&read_num);     

		/* 读取BMP文件头信息*/
		f_read(&bmpfsrc,&bitHead,sizeof(tagBITMAPFILEHEADER),&read_num);        
		/* 读取位图信息头信息 */
		f_read(&bmpfsrc,&bitInfoHead,sizeof(BITMAPINFOHEADER),&read_num);        
		headerdeal(&bitHead,&bitInfoHead);
		/* 发送包打包，返回数据包长度 */
		packet_length=Tramsmit_packet(pack_head,bmp_head,dest_address,54);
		
		/*发送头文件包 */
   for(i=0;i<packet_length;i++)	   
	  {
		   BMP_DEBUG_PRINTF("%c",pack_head[i]);
		}
		// while(!usart_flag);
 	while(usart_flag!=0x73);  //等待协调节点回复传输标志位,控制传输速度
		usart_flag=0;
	}    
	else
	{
		BMP_DEBUG_PRINTF("file open fail!\r\n");
		return;
	}    
/*-------------------------------------------------------------------------------------------------------*/
	width = bitInfoHead.biWidth;
	height = bitInfoHead.biHeight;

	/* 计算位图的实际宽度并确保它为32的倍数	*/
//	l_width = WIDTHBYTES(width* bitInfoHead.biBitCount);	
  width=width/8;
	height=height*8;
	/* 判断是否是24bit真彩色图 */

		for(i=0;i< height; i++)
		{	  
			/* 读取一行bmp的数据到数组bmp_data里面 */
			f_read(&bmpfsrc,bmp_data,width*3,&read_num);			
   
		  packet_length=Tramsmit_packet(pack_data,bmp_data,dest_address,120);   //打包成ZigBee发送数据帧
			for(j=0;j<packet_length;j++)
			{
				BMP_DEBUG_PRINTF("%c",pack_data[j]); //发送数据包
			}
		//	while(!usart_flag);
				while(usart_flag!=0x74);  //等待协调节点回复传输标志位，控制传输速度
			  usart_flag=0;
		}  
	f_close(&bmpfsrc); 
	/*定时写入*/  	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , DISABLE);		/*先关闭等待使用*/	
	timer_length=	sprintf((char*)time_write," \r\npicture%stime:%d ",pic_name,timer);
	bmpres = f_open( &bmpfdst , "timer.txt", FA_OPEN_EXISTING | FA_READ|FA_WRITE);	
	f_lseek(&bmpfdst,(*&bmpfdst).fsize); 
	f_write(&bmpfdst,time_write,timer_length,&bw);	
	f_close(&bmpfdst);
  timer=0;		
}

/*
 * 保存bmp图像
 * 
 */
void bmp_sd_save(unsigned short int Width,unsigned short int Height,unsigned char *filename)
{
		uint16_t i,j;
		uint16_t Camera_Data;		
		long file_size;     
		long width;
		long height;
		unsigned char r,g,b;	
		unsigned int mybw;
		
		/* 宽*高 +补充的字节 + 头部信息 */
		file_size = (long)Width * (long)Height * 3 + Height*(Width%4) + 54;		
			/* 文件大小 4个字节 */
		bmp_head[2] = (unsigned char)(file_size &0x000000ff);
		bmp_head[3] = (file_size >> 8) & 0x000000ff;
		bmp_head[4] = (file_size >> 16) & 0x000000ff;
		bmp_head[5] = (file_size >> 24) & 0x000000ff;
		
		/* 位图宽 4个字节 */
		width=Width;	
		bmp_head[18] = width & 0x000000ff;
		bmp_head[19] = (width >> 8) &0x000000ff;
		bmp_head[20] = (width >> 16) &0x000000ff;
		bmp_head[21] = (width >> 24) &0x000000ff;
		
		/* 位图高 4个字节 */
		height = Height;
		bmp_head[22] = height &0x000000ff;
		bmp_head[23] = (height >> 8) &0x000000ff;
		bmp_head[24] = (height >> 16) &0x000000ff;
		bmp_head[25] = (height >> 24) &0x000000ff;	
		
		

		/* 注册一个工作区，逻辑号为0 */
		f_mount(0, &bmpfs[0]);
		
		bmpres = f_open( &bmpfsrc , (char*)filename,  FA_OPEN_ALWAYS | FA_WRITE);
		
		if ( bmpres == FR_OK )
		{    
			/* 将预先定义好的bmp头部信息写进文件里面 */
				f_write(&bmpfsrc, bmp_head, sizeof(unsigned char)*54, &mybw);
		
			/* 下面是将指定窗口的数据读出来写到文件里面去 */
				for(i=0; i<240;i++)
					{
							for(j=0;j<320;j++)
							{
									READ_FIFO_PIXEL(Camera_Data);		/* 从FIFO读出一个rgb565像素到Camera_Data变量 */		
									r =  GETR_FROM_RGB16(Camera_Data);
									g =  GETG_FROM_RGB16(Camera_Data);
									b =  GETB_FROM_RGB16(Camera_Data);		

									bmpres = f_write(&bmpfsrc, &b,sizeof(unsigned char), &mybw);//保存到文件中
									bmpres = f_write(&bmpfsrc, &g,sizeof(unsigned char), &mybw);
									bmpres = f_write(&bmpfsrc, &r,sizeof(unsigned char), &mybw);						
							}			
				 }
		 }
		f_close(&bmpfsrc); 
}
