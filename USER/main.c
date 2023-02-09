#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "string.h"
#include "sdram.h"
#include "malloc.h"
#include "w25qxx.h"
#include "ff.h"
#include "exfuns.h"
#include "string.h"
#include "sdio_sdcard.h"
#include "fontupd.h"
#include "text.h"
#include "touch.h"
#include "pyinput.h"
#include "rs485.h"


#define SENSOR_NUM 4


u16 kbdxsize;	//�������
u16 kbdysize;	//�����߶�
u8 r=0;	
u8 m=0;
u8 key1;
u8 request_humidity=30;

float humidity[SENSOR_NUM];
float temperature[SENSOR_NUM];
u8 rs485buf_send_humid[8]={0x01,0x03,0x02,0x03,0x00,0x78,0xB4,0x50};//��ȡʪ����������
u8 rs485buf_read_humid[245];//ʪ�Ȼ�����
u8 rs485buf_send_temperature[8]={0x01,0x03,0x00,0x03,0x00,0x79,0x74,0x28};//�����¶���������
u8 rs485buf_read_temperature[247];//�¶Ȼ�����

//���ؼ��̽���
//x,y:������ʼ����
void py_load_ui(u16 x,u16 y)
{
	u16 i;
	POINT_COLOR=RED;
	LCD_DrawRectangle(x,y,x+kbdxsize,y+kbdysize);		
  	LCD_DrawRectangle(x,y+kbdysize+40,x+kbdxsize,y+2*kbdysize+40);		
	LCD_DrawRectangle(x,y+2*kbdysize+80,x+kbdxsize,y+3*kbdysize+80);
	LCD_DrawRectangle(x,y+3*kbdysize+120,x+kbdxsize,y+4*kbdysize+120);
	//LCD_DrawRectangle(x+kbdxsize,y,x+kbdxsize*2,y+kbdysize*3);						   
	//LCD_DrawRectangle(x,y+kbdysize,x+kbdxsize*3,y+kbdysize*2);
	POINT_COLOR=BLUE;
	//for(i=0;i<9;i++)
	//{
	//	Show_Str_Mid(x+(i%3)*kbdxsize,y+4+kbdysize*(i/3),(u8*)kbd_tbl[i],16,kbdxsize);		
	//	Show_Str_Mid(x+(i%3)*kbdxsize,y+kbdysize/2+kbdysize*(i/3),(u8*)kbs_tbl[i],16,kbdxsize);		
	//}  		 					   
}
//����״̬����
//x,y:��������
//key:��ֵ��0~8��
//sta:״̬��0���ɿ���1�����£�
void py_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx;
	if(keyx>3)return;
	if(sta)LCD_Fill(x+1,y+i*(kbdysize+40)+1,x+kbdxsize-1,y+i*(kbdysize+40)+kbdysize-1,GREEN);
	else LCD_Fill(x+1,y+i*(kbdysize+40)+1,x+kbdxsize-1,y+i*(kbdysize+40)+kbdysize-1,WHITE); 
	Show_Str(224,164,32,32,"��",32,0);
	Show_Str(224,322,32,32,"��",32,0);
	Show_Str(224,480,32,32,"��",32,0);
	Show_Str(224,638,32,32,"��",32,0);
	if(sta)
	{
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
		switch(keyx)
		{
			case 0:HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_SET);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);break;
			case 1:HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);break;
			case 2:HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_SET);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);break;
			default:break;
		}
	}

}
//�õ�������������
//x,y:��������
//����ֵ��������ֵ��1~9��Ч��0,��Ч��
u8 py_get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x = 0;//0,û���κΰ������£�1~9��1~9�Ű�������
	u8 key=0;
	
	tp_dev.scan(0); 		 
	if(tp_dev.sta&TP_PRES_DOWN)			//������������
	{	
		for(i=0;i<4;i++)
		{
			if(tp_dev.x[0]<(x+kbdxsize)&&tp_dev.x[0]>(x)&&tp_dev.y[0]<(y+i*(kbdysize+40)+kbdysize)&&tp_dev.y[0]>(y+i*(kbdysize+40)))
				key=i+1;	  		   
			if(key)
			{	   
				if(key_x!=key)
				{
					py_key_staset(x,y,key_x-1,0);
					key_x=key;
					py_key_staset(x,y,key_x-1,1);
				}
				break;
			}
		}  
	}
	return key; 
}
							   
void PB_Init()
{
	GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOB_CLK_ENABLE();           //����GPIOBʱ��
	
    GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15; //PB
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //�������
    GPIO_Initure.Pull=GPIO_PULLUP;          //����
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //����
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
}

/**
  * @brief  �շ�һ��ʪ������
  * @retval None
  */
void CheckHumidity()
{
	RS485_Send_Data(rs485buf_send_humid,8);//����ʪ�ȶ�ȡ����
	delay_ms(534);//�ȴ��շ�����Ӧ
	RS485_Receive_Data(rs485buf_read_humid,&key1);//�ռ�ʪ������
	r=0;
	for(m=3;m<3+(SENSOR_NUM-1)*2+1;m=m+2)
	{
		humidity[r] = (rs485buf_read_humid[m]*16*16+rs485buf_read_humid[m+1])/10;
		Show_Str(150,730,128,32,"��ǰʪ�ȣ�",16,0);
		LCD_ShowxNum_DEC(300+r*30,730,humidity[r],2,16,0X80);
		r++;
	}
}

void CheckTemperature()
{
	RS485_Send_Data(rs485buf_send_temperature,8);//����ʪ�ȶ�ȡ����
	delay_ms(547);//�ȴ��շ�����Ӧ
	RS485_Receive_Data(rs485buf_read_temperature,&key1);//�ռ�ʪ������
	r=0;
	for(m=5;m<5+(SENSOR_NUM-1)*2+1;m=m+2)
	{
		if((rs485buf_read_temperature[m]!=0xFE) && (rs485buf_read_temperature[m+1])!=0x0C)
			temperature[r] = (rs485buf_read_temperature[m]*16*16+rs485buf_read_temperature[m+1])/10;
			Show_Str(150,760,128,32,"��ǰ�¶ȣ�",16,0);
			LCD_ShowxNum_DEC(300+r*30,760,temperature[r],2,16,0X80);
		r++;
	}
}

/**
  * @brief  �ж��Ƿ�ﵽ���Ҫ��Ĭ�Ϻ��Ҫ����ʪ�ȵ���40%��
  * @retval None
  */
void ShutDown()
{
	int sum;
	int average_humidity=sum/SENSOR_NUM;
	
	for(r=0;r<SENSOR_NUM;r++)
	{	sum+=humidity[r];
	}

	if(average_humidity<request_humidity)
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
	}
	
}
int main(void)
{	
    u8 i=0;	    	  	    
	u8 result_num;
	u8 cur_index;
	u8 key;

	u8 inputstr[7];		            //�������6���ַ�+������
	u8 inputlen;		            //���볤��
	u8 j=0,t=0;
	u8 cnt=0;


	u32 count = 0;
	u8 count2 = 0;


    
    HAL_Init();                     //��ʼ��HAL��   
    Stm32_Clock_Init(360,25,2,8);   //����ʱ��,180Mhz
    delay_init(180);                //��ʼ����ʱ����
    uart_init(9600);              //��ʼ��USART
    LED_Init();                     //��ʼ��LED 
    KEY_Init();                     //��ʼ������
    SDRAM_Init();                   //SDRAM��ʼ��
    LCD_Init();                     //LCD��ʼ��
	PB_Init();
	W25QXX_Init();				    //��ʼ��W25Q256
    tp_dev.init();				    //��ʼ��������
    my_mem_init(SRAMIN);            //��ʼ���ڲ��ڴ��
    my_mem_init(SRAMEX);            //��ʼ���ⲿSDRAM�ڴ��
    my_mem_init(SRAMCCM);           //��ʼ���ڲ�CCM�ڴ��
    RS485_Init(9600);
	SDRAM_Init();

	  
    
	POINT_COLOR=RED;      
 	while(font_init()) 				//����ֿ�
	{	    
		LCD_ShowString(60,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(60,50,240,66,WHITE);//�����ʾ	     
	}
	Show_Str(120,35,300,32,"���ܺ�ɻ��������",32,1);
	Show_Str(120,90,128,32,"��ǰ�趨ʪ��:",16,1);
	LCD_ShowxNum_DEC(250,90,request_humidity,2,16,0X80);
	Show_Str(120,90,32,32,"%",16,1);
	kbdxsize=240;kbdysize=120;
	py_load_ui(120,120);
	
	Show_Str(224,164,32,32,"��",32,0);
	Show_Str(224,322,32,32,"��",32,0);
	Show_Str(224,480,32,32,"��",32,0);
	Show_Str(224,638,32,32,"��",32,0);

	memset(inputstr,0,7);	//ȫ������
	inputlen=0;				//���볤��Ϊ0
	result_num=0;			//��ƥ��������
	cur_index=0;

	//������δ�������Ҫ��ѭ�����룬Ҳ���ǹ���״̬����
	
	while(1)
	{
		i++;
		delay_ms(10);
		key=py_get_keynum(120,120);    //��ô�ʱ������Ϣ
		if(count!=0 && count==500)
		{
			CheckHumidity();
			CheckTemperature();

			ShutDown();
			count = 0;
		}
		if(i==30)
		{
			i=0;
			LED0=!LED0;
		}
		count++;   //ÿѭ��һ�λ��������+1��ÿ�ﵽ1000������Լ10�룩�շ�һ����ʪ������
	}     							    						
}
