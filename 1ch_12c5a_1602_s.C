#include <STC12C5A60S2.H>
#include <intrins.h> 
#include "delay.h"
#include "ds1302.h"
#include "lcd1602.h"


#define uchar unsigned char
#define uint unsigned int
	

#define FOSC   11059200L
#define BAUD   9600



sbit  RE_DE = P0^4;	//max485���÷�ʽ�����ӣ���RE��DE��������һ��IO�ڿ���
sbit  beep = P3^6;  //������
sbit  led1 = P1^5;  //��ƣ���0-15%
sbit  led2 = P1^6;  //�̵ƣ�����16%-60%
sbit  led3 = P1^7;  //�Ƶƣ�ʪ61%-99%

/*****stc12c5a60s2��صļĴ���˵��*****/ 
//sfr P1ASF = 0x9D;     //P1��ģ��ת�����ܿ��ƼĴ��� 
//sfr ADC_CONTR = 0xBC; //ADת�����ƼĴ��� 
//sfr ADC_RES = 0xBD;  //ADת������Ĵ�����λ 
//sfr ADC_RESL = 0xBE; //ADת������Ĵ�����λ 
//sfr AUXR1 = 0xA2;    //ADת������洢��ʽ����λ  
/***P1ASF�Ĵ�����8λ����ӦP1��8���ߣ�����ָ���Ǹ�������ADC���� ******�ĸ�������ADC��Ӧ����Ӧ��λΪ1��ע�⣺����λѰַ******/ 
//sfr WDT_CONTR=0xC1;

#define ADC_POWER 0x80     //ADC��Դ�� 
#define ADC_SPEEDLL 0x00   //����Ϊ420�����ڣ�ADCһ�� 125kHz
#define ADC_SPEEDL 0x20    //280CLOCKS
#define ADC_SPEEDH 0x40    //140clocks
#define ADC_SPEEDHH 0x60   //70clocks
#define ADC_START 0x08     //ADC��������λ��Ϊ�� 
#define ADC_FLAG 0X10      //ADC������־λ  

uchar *p1,*p2,*p3,*p4;//����ָ�룬�ֱ��Ӧ���ڣ�ʱ�䣬оƬ��ʽ���ݣ����ù��λ��
uchar mdate[16];//������ʾ����"20xx-xx-xx Mon."
uchar mtime[9],ntime[6];//������ʾʱ��"xx:xx:xx"���ֱ�ʱ��"XX:XX"ĩλд0���ڽ����ַ������
uchar stime[2];//���ڱ�������ʱ��
uchar ttime[7];//={0x48,0x48,0x22,0x028,0x05,0x06,0x18};//���ڳ�ʼ��DS1302���Ͷ�ȡʱ��

sbit key1=P3^5;//ʱ�����������1����
sbit key2=P3^4;//2��
sbit key3=P3^3;//3��
sbit key4=P3^2;//4�˳�
uchar n,stat=0;//����״̬��־��0Ϊ������ʾ״̬
								//1-9Ϊ����״̬1-�ꡢ2-�¡�3-�ա�4-�ܡ�5-ʱ��6-�֡�7-�롢8-�ֱ�ʱ��9-�ֱ���
uint warn_l1=15;
uint warn_l2=60;


uchar isryear(uchar year1)
{
	int year,isrun;

	//�ж��Ƿ�����,1Ϊ���꣬0Ϊƽ��
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
	if(key1==0)//����
	{
		delayms(10);
		if(key1==0)
		{
			stat++;//���ñ�־��0--7֮��仯,0Ϊ������ʾ״̬��1--7�򿪹����˸����7��ʱ��ѡ�����ƶ�,8��9�ֱ�ʱ��
			if(stat>7)
				stat=1;
		}
		while(!key1);
	}
	if(key2==0)//ѡ��λ��+1
	{
		delayms(10);
		if(key2==0)
		{
			switch(stat)
			{
				case 1://��
				{
					tempt[6]++;
					if((tempt[6]&0x0f)==0x0a)//��10��1
					{
						tempt[6]+=0x10;
						tempt[6]=tempt[6]&0xf0;
						if(tempt[6]==0xa0)//��100��0
							tempt[6]=0x00;
					}
					//�ж�ƽ������2���Ƿ����
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))
						tempt[3]=0x28;
					break;
				}
				case 2://��
				{
					tempt[4]+=0x01;
					if((tempt[4]&0x0f)==0x0a)//��10��1
					{
						tempt[4]+=0x10;//ǰ4λ��λ
						tempt[4]=tempt[4]&0xf0;//��4λ��0
					}
					if(((tempt[4]&0x0f)==0x03)&((tempt[4]&0xf0)==0x10))//��13��1
						tempt[4]=0x01;
					//��������֮���������û�����
					isrun=isryear(tempt[6]);
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))//29����
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))//28����
						tempt[3]=0x28;
					else if(((tempt[4]==0x01)||(tempt[4]==0x03)||(tempt[4]==0x05)//31����
						||(tempt[4]==0x07)||(tempt[4]==0x08)||(tempt[4]==0x10)
						||(tempt[4]==0x12))&&(tempt[3]>0x31))
						tempt[3]=0x31;
					else if(((tempt[4]==0x04)||(tempt[4]==0x06)||(tempt[4]==0x09)//30����
						||(tempt[4]==0x11))&&(tempt[3]>0x30))
						tempt[3]=0x30;
					break;
				}
				case 3://��
				{
					tempt[3]+=0x01;
					if((tempt[3]&0x0f)==0x0a)//��10��1
					{
						tempt[3]+=0x10;
						tempt[3]=tempt[3]&0xf0;
					}
					//��������֮���������û�����
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
				case 4://����
				{
					tempt[5]+=0x01;
					if(tempt[5]>0x07)
						tempt[5]=0x01;
					break;
				}
				case 5://ʱ
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
				case 6://��
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
				case 7://��
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
//				case 8://�ֱ�ʱ
//				{
//					stime[1]+=0x01;
//					if((stime[1]&0x0f)==0x0a)
//					{
//						stime[1]+=0x10;
//						stime[1]=stime[1]&0xf0;
//					}
//					if(((stime[1]&0x0f)==0x04)&&((stime[1]&0xf0)==0x20))
//						stime[1]=0x00;
//		Ds1302Write(0x8e,0x00);//�ر�д����
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//��д����

//					break;
//				}
//				case 9://�ֱ���
//				{
//					stime[0]+=0x01;
//					if((stime[0]&0x0f)==0x0a)
//					{
//						stime[0]+=0x10;
//						stime[0]=stime[0]&0xf0;
//					}
//					if(((stime[0]&0x0f)==0x00)&&((stime[0]&0xf0)==0x60))
//						stime[0]=0x00;
//		Ds1302Write(0x8e,0x00);//�ر�д����
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//��д����
//					break;
//				}

			}
		}
		while(!key2);
	}
	if(key3==0)//ѡ��λ��-1
	{
		delayms(30);
		if(key3==0)
		{
			switch(stat)
			{
				case 1://��
				{
					if(tempt[6]==0x00)//��00��99
							tempt[6]=0x99;
					else if((tempt[6]&0x0f)==0x00)//��0��9
					{
						tempt[6]-=0x10;
						tempt[6]=tempt[6]|0x09;
					}
					else tempt[6]--;
					//��������֮���������û�����
					isrun=isryear(tempt[6]);
					//�ж�ƽ������2���Ƿ����
					if((tempt[4]==0x02)&&(tempt[3]>0x29)&&(isrun==1))
						tempt[3]=0x29;
					else if((tempt[4]==0x02)&&(tempt[3]>0x28)&&(isrun==0))
						tempt[3]=0x28;
					break;
				}
				case 2://��
				{
					if(((tempt[4]&0x0f)==0x01)&((tempt[4]&0xf0)==0x00))//��1��12
						tempt[4]=0x12;
					else if((tempt[4]&0x0f)==0x00)//10��9
					{
						tempt[4]=0x09;
					}
					else tempt[4]-=0x01;
					//��������֮���������û�����
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
				case 3://��
				{
					//��������֮���������û�����
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
					else if((tempt[3]&0x0f)==0x00)//��0��1��9
					{
						tempt[3]-=0x10;
						tempt[3]=tempt[3]|0x09;
					}
					else 	tempt[3]-=0x01;
					break;
				}
				case 4://����
				{
					tempt[5]-=0x01;
					if(tempt[5]==0x00)
						tempt[5]=0x07;
					break;
				}
				case 5://ʱ
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
				case 6://��
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
				case 7://��
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
//				case 8://�ֱ�ʱ
//				{
//					if(stime[1]==0x00)
//						stime[1]=0x23;
//					else if((stime[1]&0x0f)==0x00)
//					{
//						stime[1]-=0x10;
//						stime[1]=stime[1]|0x09;
//					}
//					else stime[1]-=0x01;
//		Ds1302Write(0x8e,0x00);//�ر�д����
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//��д����
//					break;
//				}
//				case 9://�ֱ���
//				{
//					if(stime[0]==0x00)
//						stime[0]=0x59;
//					else if((stime[0]&0x0f)==0x00)
//					{
//						stime[0]-=0x10;
//						stime[0]=stime[0]|0x09;
//					}
//					else stime[0]-=0x01;
//		Ds1302Write(0x8e,0x00);//�ر�д����
//					Ds1302Write(0xc0,stime[0]);
//					Ds1302Write(0xc2,stime[1]);
//		Ds1302Write(0x8e,0x80);//��д����
//					break;
//				}

			}
		}
		while(!key3);
	}
	if(key4==0)//����
	{
		delayms(10);
		if(key4==0)
		{
				stat=0;
		}
		while(!key4);
		//�˳����ö�ȡ�ֱ�ʱ��
//		stime[0]=Ds1302Read(0xc1);
//		stime[1]=Ds1302Read(0xc3);
	}
	if(stat>0)
	{
		Ds1302Write(0x8e,0x00);//�ر�д����
		for(n=0;n<7;n++)
		{
				 Ds1302Write(WRITE_RTC_ADDR[n],tempt[n]);
		}
		Ds1302Write(0x8e,0x80);//��д����
	}
}

void showtime()//��ʾ����ʱ��
{
	Ds1302readTime(p3);//��ȡʱ��
	dispros(p1,p2,p3);//ת��ʱ��
	Lcdwritedat(0x80,mdate);//��ʾ����
	Lcdwritedat(0x80+0x40,mtime);//��ʾʱ��
}




//***��nͨ��ADC��ʼ������***// 
void ADC_int() 
{ 
  P1ASF = 0X0F; //P1.0,P1.1,P1.2,P1.3����Ϊģ�⹦��ADʹ��
	//  ch &= 0x07;         //ȷ���ǵ�0~7ͨ��  
	AUXR1 |= 0x04;     //ת������洢��ʽ����2λ��ADC_RES����8λ��ADC_RESL  
//	AUXR1=0X00;        //����λ�����ADCRESL����λ�У��߰�λ�����ADCRES��
	ADC_RES = 0;  //���Ž���洢��  
	ADC_RESL = 0;  //���Ž���洢��

	ADC_CONTR =ADC_POWER|ADC_SPEEDHH;;  //ADת�����ƼĴ�����0���Ա�����  
//	ADC_CONTR =0xE0;
	delay(2);
}  

//***��nͨ��ADC��������***// 
uint ADC_GET(uchar ch) 
{  
	uint adc_data;  
	ch &= 0x07;   //ȷ���ǵ�0~7ͨ��  

	ADC_CONTR |= (ADC_POWER|ADC_SPEEDHH|ch|ADC_START);  //��ADת����Դ���趨ת���ٶȣ��趨ͨ���ţ�ADת����ʼ  
//	ADC_CONTR =0XE8+ch;
	_nop_();
	_nop_();
	_nop_();
	_nop_();//��ʱ4��ʱ����������

	while(!(ADC_CONTR & ADC_FLAG));
	ADC_CONTR &= ~ADC_FLAG;
//  adc_data=(ADC_RES)*4 + ADC_RESL;
	adc_data=(ADC_RES&0x03)*256 + ADC_RESL;
//	adc_data=ADC_RES;
	ADC_RES = 0;  //���Ž���洢��  
	ADC_RESL = 0;  //���Ž���洢��
	
	return adc_data;    //adc_data��ֵ��0~1023�� 
}  

uint shidu(uint x)
{
	float temp;
	uint y;
	temp=0.17128*(1024-x); //600*x/1024
	y=temp;//�Խ����������
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
		RE_DE=1;        //��485ģ�鷢��ʹ�ܣ��رս��չ���
		while (!TI);    //Wait for the previous data is sent
    TI = 0;         //Clear TI flag
    SBUF = dat;     //Send current data
		RE_DE=0;		   //�ر�485ģ�鷢��ʹ�ܣ��򿪽��չ���
}


void main()
{

	uint ad_0=0; 
	uint moi0=0;
	uchar iplc[]={3,6,9,14,0x40+1,0x40+4,0x40+7};//����ģʽ�¸���stat�жϹ����˸��λ��
	
	InitUart();      //���ڳ�ʼ��
	ADC_int( );      //ADC��ͨ��0��ʼ�� 
	lcd_init();
	delayms(100);      //�ϵ�����ʱ50ms�ȴ�����������ʱ5ms�ȴ������ȶ����
//	Ds1302Init();//��ʼ��ʱ��ģ�飬ֻ����оƬ��ʼ������һ�μ���
	
	p1=mdate;//���ں����ڼ�����ʾ
	p2=mtime;//����ʱ��ʱ�������ʾ
	p3=ttime;//�����ȡ��ʱ��
	
//  WDT_CONTR=0x3c;
	while(1)
	{
//   WDT_CONTR=0x3c;
		keyscan(p3);
		if((stat>0)&&(stat<8))//stat��1-7����ʱ��
			{
				showtime();//��ʾ����ʱ��				
				write_com(0x80+iplc[stat-1]);//�ƶ����
				write_com(0x0f);//�����˸
				keyscan(p3);
			}
		else if(stat==0)
			{
				showtime();//��ʾ����ʱ��
				write_com(0x0c);//�رչ����˸
				
				ad_0=ADC_GET(0);    //ADת������������ݴ����ad_0��    
				moi0=shidu(ad_0); //��ADֵת��Ϊʪ��
				write_com(0x80+0x40+12);  
				dis_2wei(moi0);
				warn(moi0);

				delayms(500);

		
				SendData(0xAA);
				SendData(0x55);
				SendData(moi0);		
				SendData(0x55);
				SendData(0xAA); 
			
				keyscan(p3);//ɨ�谴�������ݰ��������������ݣ���ȡ�����ù���λ��i				
			
			}
			
		

//    delayms(500);		

	}
}