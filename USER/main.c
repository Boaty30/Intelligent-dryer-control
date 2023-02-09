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


u16 kbdxsize;	//按键宽度
u16 kbdysize;	//按键高度
u8 r=0;	
u8 m=0;
u8 key1;
u8 request_humidity=30;

float humidity[SENSOR_NUM];
float temperature[SENSOR_NUM];
u8 rs485buf_send_humid[8]={0x01,0x03,0x02,0x03,0x00,0x78,0xB4,0x50};//读取湿度请求命令
u8 rs485buf_read_humid[245];//湿度缓存器
u8 rs485buf_send_temperature[8]={0x01,0x03,0x00,0x03,0x00,0x79,0x74,0x28};//请求温度请求命令
u8 rs485buf_read_temperature[247];//温度缓存器

//加载键盘界面
//x,y:界面起始坐标
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
//按键状态设置
//x,y:键盘坐标
//key:键值（0~8）
//sta:状态，0，松开；1，按下；
void py_key_staset(u16 x,u16 y,u8 keyx,u8 sta)
{		  
	u16 i=keyx;
	if(keyx>3)return;
	if(sta)LCD_Fill(x+1,y+i*(kbdysize+40)+1,x+kbdxsize-1,y+i*(kbdysize+40)+kbdysize-1,GREEN);
	else LCD_Fill(x+1,y+i*(kbdysize+40)+1,x+kbdxsize-1,y+i*(kbdysize+40)+kbdysize-1,WHITE); 
	Show_Str(224,164,32,32,"低",32,0);
	Show_Str(224,322,32,32,"中",32,0);
	Show_Str(224,480,32,32,"高",32,0);
	Show_Str(224,638,32,32,"关",32,0);
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
//得到触摸屏的输入
//x,y:键盘坐标
//返回值：按键键值（1~9有效；0,无效）
u8 py_get_keynum(u16 x,u16 y)
{
	u16 i,j;
	static u8 key_x = 0;//0,没有任何按键按下；1~9，1~9号按键按下
	u8 key=0;
	
	tp_dev.scan(0); 		 
	if(tp_dev.sta&TP_PRES_DOWN)			//触摸屏被按下
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
    __HAL_RCC_GPIOB_CLK_ENABLE();           //开启GPIOB时钟
	
    GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15; //PB
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_15,GPIO_PIN_RESET);
}

/**
  * @brief  收发一次湿度数据
  * @retval None
  */
void CheckHumidity()
{
	RS485_Send_Data(rs485buf_send_humid,8);//发送湿度读取请求
	delay_ms(534);//等待收发器响应
	RS485_Receive_Data(rs485buf_read_humid,&key1);//收集湿度数据
	r=0;
	for(m=3;m<3+(SENSOR_NUM-1)*2+1;m=m+2)
	{
		humidity[r] = (rs485buf_read_humid[m]*16*16+rs485buf_read_humid[m+1])/10;
		Show_Str(150,730,128,32,"当前湿度：",16,0);
		LCD_ShowxNum_DEC(300+r*30,730,humidity[r],2,16,0X80);
		r++;
	}
}

void CheckTemperature()
{
	RS485_Send_Data(rs485buf_send_temperature,8);//发送湿度读取请求
	delay_ms(547);//等待收发器响应
	RS485_Receive_Data(rs485buf_read_temperature,&key1);//收集湿度数据
	r=0;
	for(m=5;m<5+(SENSOR_NUM-1)*2+1;m=m+2)
	{
		if((rs485buf_read_temperature[m]!=0xFE) && (rs485buf_read_temperature[m+1])!=0x0C)
			temperature[r] = (rs485buf_read_temperature[m]*16*16+rs485buf_read_temperature[m+1])/10;
			Show_Str(150,760,128,32,"当前温度：",16,0);
			LCD_ShowxNum_DEC(300+r*30,760,temperature[r],2,16,0X80);
		r++;
	}
}

/**
  * @brief  判断是否达到烘干要求。默认烘干要求是湿度低于40%。
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

	u8 inputstr[7];		            //最大输入6个字符+结束符
	u8 inputlen;		            //输入长度
	u8 j=0,t=0;
	u8 cnt=0;


	u32 count = 0;
	u8 count2 = 0;


    
    HAL_Init();                     //初始化HAL库   
    Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz
    delay_init(180);                //初始化延时函数
    uart_init(9600);              //初始化USART
    LED_Init();                     //初始化LED 
    KEY_Init();                     //初始化按键
    SDRAM_Init();                   //SDRAM初始化
    LCD_Init();                     //LCD初始化
	PB_Init();
	W25QXX_Init();				    //初始化W25Q256
    tp_dev.init();				    //初始化触摸屏
    my_mem_init(SRAMIN);            //初始化内部内存池
    my_mem_init(SRAMEX);            //初始化外部SDRAM内存池
    my_mem_init(SRAMCCM);           //初始化内部CCM内存池
    RS485_Init(9600);
	SDRAM_Init();

	  
    
	POINT_COLOR=RED;      
 	while(font_init()) 				//检查字库
	{	    
		LCD_ShowString(60,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(60,50,240,66,WHITE);//清除显示	     
	}
	Show_Str(120,35,300,32,"智能烘干机控制面板",32,1);
	Show_Str(120,90,128,32,"当前设定湿度:",16,1);
	LCD_ShowxNum_DEC(250,90,request_humidity,2,16,0X80);
	Show_Str(120,90,32,32,"%",16,1);
	kbdxsize=240;kbdysize=120;
	py_load_ui(120,120);
	
	Show_Str(224,164,32,32,"低",32,0);
	Show_Str(224,322,32,32,"中",32,0);
	Show_Str(224,480,32,32,"高",32,0);
	Show_Str(224,638,32,32,"关",32,0);

	memset(inputstr,0,7);	//全部清零
	inputlen=0;				//输入长度为0
	result_num=0;			//总匹配数清零
	cur_index=0;

	//下面这段代码是主要的循环代码，也就是工作状态程序
	
	while(1)
	{
		i++;
		delay_ms(10);
		key=py_get_keynum(120,120);    //获得此时触摸信息
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
		count++;   //每循环一次会给计数器+1，每达到1000（即大约10秒）收发一次温湿度数据
	}     							    						
}
