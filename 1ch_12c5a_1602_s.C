#include <STC12C5A60S2.H>
#include <intrins.h> 
#include "delay.h"
#include "ds1302.h"
#include "lcd1602.h"


#define uchar unsigned char
#define uint unsigned int
	

#define FOSC   11059200L
#define BAUD   9600



sbit  RE_DE = P0^4;	//max485采用方式二连接，即RE和DE相连接用一个IO口控制
sbit  beep = P3^6;  //蜂鸣器
sbit  led1 = P1^5;  //红灯，干0-15%
sbit  led2 = P1^6;  //绿灯，适中16%-60%
sbit  led3 = P1^7;  //黄灯，湿61%-99%

/*****stc12c5a60s2相关的寄存器说明*****/ 
//sfr P1ASF = 0x9D;     //P1口模数转换功能控制寄存器 
//sfr ADC_CONTR = 0xBC; //AD转换控制寄存器 
//sfr ADC_RES = 0xBD;  //AD转换结果寄存器高位 
//sfr ADC_RESL = 0xBE; //AD转换结果寄存器低位 
//sfr AUXR1 = 0xA2;    //AD转换结果存储方式控制位  
/***P1ASF寄存器：8位，对应P1口8根线，用于指定那根线用作ADC功能 ******哪根线用作ADC就应置相应的位为1，注意：不能位寻址******/ 
//sfr WDT_CONTR=0xC1;

#define ADC_POWER 0x80     //ADC电源开 
#define ADC_SPEEDLL 0x00   //设置为420个周期，ADC一次 125kHz
#define ADC_SPEEDL 0x20    //280CLOCKS
#define ADC_SPEEDH 0x40    //140clocks
#define ADC_SPEEDHH 0x60   //70clocks
#define ADC_START 0x08     //ADC启动控制位设为开 
#define ADC_FLAG 0X10      //ADC结束标志位  

uchar *p1,*p2,*p3,*p4;//定义指针，分别对应日期，时间，芯片格式数据，设置光标位置
uchar mdate[16];//用于显示日期"20xx-xx-xx Mon."
uchar mtime[9],ntime[6];//用于显示时间"xx:xx:xx"和闹表时间"XX:XX"末位写0用于结束字符串输出
uchar stime[2];//用于保存闹钟时间
uchar ttime[7];//={0x48,0x48,0x22,0x028,0x05,0x06,0x18};//用于初始化DS1302，和读取时间

sbit key1=P3^5;//时间调整按键，1设置
sbit key2=P3^4;//2加
sbit key3=P3^3;//3减
sbit key4=P3^2;//4退出
uchar n,stat=0;//设置状态标志，0为正常显示状态
								//1-9为设置状态1-年、2-月、3-日、4-周、5-时、6-分、7-秒、8-闹表时、9-闹表分
uint warn_l1=15;
uint warn_l2=60;


uchar isryear(uchar year1)
{
	int year,isrun;

	//判断是否闰年,1为闰年，0为平年
	year=2000+year1/16*10+year1%16;
	if(year%4==0)
	{
		if(year%100!=0)
		{
			isrun=1;
		}
		else if(year%400)
		{
			isrun=1;
		}
		else
		{
			isrun=0;
		}
	 } 
	else
	{
	 	isrun=0;
	 }
	return isrun;
}

void keyscan(uchar *tempt)
{
	uchar isrun;
	if(key1==0)//设置
	{
		delayms(10);
		if(key1==0)
		{
			stat++;//设置标志在0--7之间变化,0为正常显示状态，1--7打开光标闪烁并在7个时间选项中移动,8、9闹表时间
			if(stat>7)
				stat=1;
		}
		while(!key1);
	}
	if(key2==0)//选中位置+1
	{
		delayms(10);
		if(key2==0)
		{
			switch(stat)
			{
				case 1://年
				{
					tempt[6]++;
					if((tempt[6]&0x0f)==0x0a)//逢10进1
					{
						tempt[6]+=0x10;
						tempt[6]=tempt[6]&0xf0;
						if(tempt[6]==0xa0)//逢100变0
							tempt[6]=0x00;
					}
					//判断平年闰年2月是否溢出
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))
						tempt[3]=0x28;
					break;
				}
				case 2://月
				{
					tempt[4]+=0x01;
					if((tempt[4]&0x0f)==0x0a)//逢10进1
					{
						tempt[4]+=0x10;//前4位进位
						tempt[4]=tempt[4]&0xf0;//后4位置0
					}
					if(((tempt[4]&0x0f)==0x03)&((tempt[4]&0xf0)==0x10))//逢13变1
						tempt[4]=0x01;
					//检查调整月之后的日期有没有溢出
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))//29天月
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))//28天月
						tempt[3]=0x28;
					else if(((tempt[4]==0x01)||(tempt[4]==0x03)||(tempt[4]==0x05)//31天月
						||(tempt[4]==0x07)||(tempt[4]==0x08)||(tempt[4]==0x10)
						||(tempt[4]==0x12))&&(tempt[3]>0x31))
						tempt[3]=0x31;
					else if(((tempt[4]==0x04)||(tempt[4]==0x06)||(tempt[4]==0x09)//30天月
						||(tempt[4]==0x11))&&(tempt[3]>0x30))
						tempt[3]=0x30;
					break;
				}
				case 3://日
				{
					tempt[3]+=0x01;
					if((tempt[3]&0x0f)==0x0a)//逢10进1
					{
						tempt[3]+=0x10;
						tempt[3]=tempt[3]&0xf0;
					}
					//检查调整月之后的日期有没有溢出
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))
						tempt[3]=0x01;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))
						tempt[3]=0x01;
					if(((tempt[4]==0x01)||(tempt[4]==0x03)||(tempt[4]==0x05)
						||(tempt[4]==0x07)||(tempt[4]==0x08)||(tempt[4]==0x10)
						||(tempt[4]==0x12))&&(tempt[3]==0x32))
						tempt[3]=0x01;
					if(((tempt[4]==0x04)||(tempt[4]==0x06)||(tempt[4]==0x09)
						||(tempt[4]==0x11))&&(tempt[3]==0x31))
						tempt[3]=0x01;
					break;
				}
				case 4://星期
				{
					tempt[5]+=0x01;
					if(tempt[5]>0x07)
						tempt[5]=0x01;
					break;
				}
				case 5://时
				{
					tempt[2]+=0x01;
					if((tempt[2]&0x0f)==0x0a)
					{
						tempt[2]+=0x10;
						tempt[2]=tempt[2]&0xf0;
					}
					if(((tempt[2]&0x0f)==0x04)&((tempt[2]&0xf0)==0x20))
						tempt[2]=0x00;
					break;
				}
				case 6://分
				{
					tempt[1]+=0x01;
					if((tempt[1]&0x0f)==0x0a)
					{
						tempt[1]+=0x10;
						tempt[1]=tempt[1]&0xf0;
					}
					if(tempt[1]==0x60)
						tempt[1]=0x00;
					break;
				}
				case 7://秒
				{
					tempt[0]+=0x01;
					if((tempt[0]&0x0f)==0x0a)
					{
						tempt[0]+=0x10;
						tempt[0]=tempt[0]&0xf0;
					}
					if(tempt[0]==0x60)
						tempt[0]=0x00;
					break;
				}
//				case 8://闹表时
//				{
//					stime[1]+=0x01;
//					if((stime[1]&0x0f)==0x0a)
//					{
//						stime[1]+=0x10;
//						stime[1]=stime[1]&0xf0;
//					}
//					if(((stime[1]&0x0f)==0x04)&&((stime[1]&0xf0)==0x20))
//						stime[1]=0x00;
//		Ds1302Write(0x8e,0x00);//关闭写保护
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//打开写保护

//					break;
//				}
//				case 9://闹表分
//				{
//					stime[0]+=0x01;
//					if((stime[0]&0x0f)==0x0a)
//					{
//						stime[0]+=0x10;
//						stime[0]=stime[0]&0xf0;
//					}
//					if(((stime[0]&0x0f)==0x00)&&((stime[0]&0xf0)==0x60))
//						stime[0]=0x00;
//		Ds1302Write(0x8e,0x00);//关闭写保护
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//打开写保护
//					break;
//				}

			}
		}
		while(!key2);
	}
	if(key3==0)//选中位置-1
	{
		delayms(30);
		if(key3==0)
		{
			switch(stat)
			{
				case 1://年
				{
					if(tempt[6]==0x00)//逢00变99
							tempt[6]=0x99;
					else if((tempt[6]&0x0f)==0x00)//逢0进9
					{
						tempt[6]-=0x10;
						tempt[6]=tempt[6]|0x09;
					}
					else tempt[6]--;
					//检查调整月之后的日期有没有溢出
					isrun=isryear(tempt[6]);
					//判断平年闰年2月是否溢出
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))
						tempt[3]=0x28;
					break;
				}
				case 2://月
				{
					if(((tempt[4]&0x0f)==0x01)&((tempt[4]&0xf0)==0x00))//逢1变12
						tempt[4]=0x12;
					else if((tempt[4]&0x0f)==0x00)//10变9
					{
						tempt[4]=0x09;
					}
					else tempt[4]-=0x01;
					//检查调整月之后的日期有没有溢出
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))
						tempt[3]=0x28;
					else if(((tempt[4]==0x01)||(tempt[4]==0x03)||(tempt[4]==0x05)
						||(tempt[4]==0x07)||(tempt[4]==0x08)||(tempt[4]==0x10)
						||(tempt[4]==0x12))&&(tempt[3]>0x31))
						tempt[3]=0x31;
					else if(((tempt[4]==0x04)||(tempt[4]==0x06)||(tempt[4]==0x09)
						||(tempt[4]==0x11))&&(tempt[3]>0x30))
						tempt[3]=0x30;
					break;
				}
				case 3://日
				{
					//检查调整月之后的日期有没有溢出
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]==0x01)&&(isrun==1))
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]==0x01)&&(isrun==0))
						tempt[3]=0x28;
					else if(((tempt[4]==0x01)||(tempt[4]==0x03)||(tempt[4]==0x05)
						||(tempt[4]==0x07)||(tempt[4]==0x08)||(tempt[4]==0x10)
						||(tempt[4]==0x12))&&(tempt[3]==0x01))
						tempt[3]=0x31;
					else if(((tempt[4]==0x04)||(tempt[4]==0x06)||(tempt[4]==0x09)
						||(tempt[4]==0x11))&&(tempt[3]==0x01))
						tempt[3]=0x30;
					else if((tempt[3]&0x0f)==0x00)//逢0借1变9
					{
						tempt[3]-=0x10;
						tempt[3]=tempt[3]|0x09;
					}
					else 	tempt[3]-=0x01;
					break;
				}
				case 4://星期
				{
					tempt[5]-=0x01;
					if(tempt[5]==0x00)
						tempt[5]=0x07;
					break;
				}
				case 5://时
				{
					if(tempt[2]==0x00)
						tempt[2]=0x23;
					else if((tempt[2]&0x0f)==0x00)
					{
						tempt[2]-=0x10;
						tempt[2]=tempt[2]|0x09;
					}
					else tempt[2]-=0x01;
					break;
				}
				case 6://分
				{
					if(tempt[1]==0x00)
					{
						tempt[1]=0x59;
					}
					else if((tempt[1]&0x0f)==0x00)
					{
						tempt[1]-=0x10;
						tempt[1]=tempt[1]|0x09;
					}
					else tempt[1]-=0x01;
					break;
				}
				case 7://秒
				{
					if(tempt[0]==0x00)
						tempt[0]=0x59;
					else if((tempt[0]&0x0f)==0x00)
					{
						tempt[0]-=0x10;
						tempt[0]=tempt[0]|0x09;
					}
					else tempt[0]-=0x01;
					break;
				}
//				case 8://闹表时
//				{
//					if(stime[1]==0x00)
//						stime[1]=0x23;
//					else if((stime[1]&0x0f)==0x00)
//					{
//						stime[1]-=0x10;
//						stime[1]=stime[1]|0x09;
//					}
//					else stime[1]-=0x01;
//		Ds1302Write(0x8e,0x00);//关闭写保护
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//打开写保护
//					break;
//				}
//				case 9://闹表分
//				{
//					if(stime[0]==0x00)
//						stime[0]=0x59;
//					else if((stime[0]&0x0f)==0x00)
//					{
//						stime[0]-=0x10;
//						stime[0]=stime[0]|0x09;
//					}
//					else stime[0]-=0x01;
//		Ds1302Write(0x8e,0x00);//关闭写保护
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//打开写保护
//					break;
//				}

			}
		}
		while(!key3);
	}
	if(key4==0)//设置
	{
		delayms(10);
		if(key4==0)
		{
				stat=0;
		}
		while(!key4);
		//退出设置读取闹表时间
//		stime[0]=Ds1302Read(0xc1);
//		stime[1]=Ds1302Read(0xc3);
	}
	if(stat>0)
	{
		Ds1302Write(0x8e,0x00);//关闭写保护
		for(n=0;n<7;n++)
		{
				 Ds1302Write(WRITE_RTC_ADDR[n],tempt[n]);
		}
		Ds1302Write(0x8e,0x80);//打开写保护
	}
}

void showtime()//显示正常时间
{
	Ds1302readTime(p3);//读取时间
	dispros(p1,p2,p3);//转换时间
	Lcdwritedat(0x80,mdate);//显示日期
	Lcdwritedat(0x80+0x40,mtime);//显示时间
}




//***第n通道ADC初始化函数***// 
void ADC_int() 
{ 
  P1ASF = 0X0F; //P1.0,P1.1,P1.2,P1.3口作为模拟功能AD使用
	//  ch &= 0x07;         //确保是第0~7通道  
	AUXR1 |= 0x04;     //转换结果存储方式：高2位放ADC_RES，低8位放ADC_RESL  
//	AUXR1=0X00;        //低两位存放在ADCRESL低两位中，高八位存放在ADCRES中
	ADC_RES = 0;  //清存放结果存储器  
	ADC_RESL = 0;  //清存放结果存储器

	ADC_CONTR =ADC_POWER|ADC_SPEEDHH;;  //AD转换控制寄存器清0，以便重置  
//	ADC_CONTR =0xE0;
	delay(2);
}  

//***第n通道ADC采样函数***// 
uint ADC_GET(uchar ch) 
{  
	uint adc_data;  
	ch &= 0x07;   //确保是第0~7通道  

	ADC_CONTR |= (ADC_POWER|ADC_SPEEDHH|ch|ADC_START);  //打开AD转换电源，设定转换速度，设定通道号，AD转换开始  
//	ADC_CONTR =0XE8+ch;
	_nop_();
	_nop_();
	_nop_();
	_nop_();//延时4个时钟周期左右

	while(!(ADC_CONTR & ADC_FLAG));
	ADC_CONTR &= ~ADC_FLAG;
//  adc_data=(ADC_RES)*4 + ADC_RESL;
	adc_data=(ADC_RES&0x03)*256 + ADC_RESL;
//	adc_data=ADC_RES;
	ADC_RES = 0;  //清存放结果存储器  
	ADC_RESL = 0;  //清存放结果存储器
	
	return adc_data;    //adc_data的值（0~1023） 
}  

uint shidu(uint x)
{
	float temp;
	uint y;
	temp=0.17128*(1024-x); //600*x/1024
	y=temp;//对结果四舍五入
	if(temp-y>=0.5)
	{
		y+=1;
	}
	if(y>99)
	{
		y=99;
	}
	return y;
}

void didi()
{
	beep=0;
	delayms(100);
	beep=1;
	delayms(100);
	beep=0;
	delayms(100);
	beep=1;
	delayms(100);	
}
void warn(uint m)
{
	if((m>=0)&&(m<=warn_l1))
	{	
		didi();
		led1=0;
		led2=1;
		led3=1;
	}
	else if((m>warn_l1)&&(m<=warn_l2))
	{	
		led1=1;
		led2=0;
		led3=1;
	}	
	else
	{
		led1=1;
		led2=1;
		led3=0;	
	}

}

/*----------------------------
Initial UART
----------------------------*/
void InitUart()
{
    SCON = 0x5a;                    //8 bit data ,no parity bit
    TMOD = 0x20;                    //T1 as 8-bit auto reload
    TH1 = TL1 = -(FOSC/12/32/BAUD); //Set Uart baudrate
    TR1 = 1;                        //T1 start running
}

/*----------------------------
Send one byte data to PC
Input: dat (UART data)
Output:-
----------------------------*/
void SendData(uchar dat)
{
		RE_DE=1;        //打开485模块发射使能，关闭接收功能
		while (!TI);    //Wait for the previous data is sent
    TI = 0;         //Clear TI flag
    SBUF = dat;     //Send current data
		RE_DE=0;		   //关闭485模块发射使能，打开接收功能
}


void main()
{

	uint ad_0=0; 
	uint moi0=0;
	uchar iplc[]={3,6,9,14,0x40+1,0x40+4,0x40+7};//设置模式下根据stat判断光标闪烁的位置
	
	InitUart();      //串口初始化
	ADC_int( );      //ADC的通道0初始化 
	lcd_init();
	delayms(100);      //上电先延时50ms等待测量，再延时5ms等待建立稳定输出
//	Ds1302Init();//初始化时间模块，只在新芯片初始化运行一次即可
	
	p1=mdate;//日期和星期几的显示
	p2=mtime;//保存时间时分秒的显示
	p3=ttime;//保存读取的时间
	
//  WDT_CONTR=0x3c;
	while(1)
	{
//   WDT_CONTR=0x3c;
		keyscan(p3);
		if((stat>0)&&(stat<8))//stat从1-7设置时间
			{
				showtime();//显示正常时间				
				write_com(0x80+iplc[stat-1]);//移动光标
				write_com(0x0f);//光标闪烁
				keyscan(p3);
			}
		else if(stat==0)
			{
				showtime();//显示正常时间
				write_com(0x0c);//关闭光标闪烁
				
				ad_0=ADC_GET(0);    //AD转换处理后的数据存放于ad_0中    
				moi0=shidu(ad_0); //把AD值转换为湿度
				write_com(0x80+0x40+12);  
				dis_2wei(moi0);
				warn(moi0);

				delayms(500);

		
				SendData(0xAA);
				SendData(0x55);
				SendData(moi0);		
				SendData(0x55);
				SendData(0xAA); 
			
				keyscan(p3);//扫描按键，根据按键进行设置数据，并取得设置光标的位置i				
			
			}
			
		

//    delayms(500);		

	}
}
