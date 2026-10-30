#include "DHT11.h"
#include "delay.h"
static void DHT11_Start(void)
{
	DHT11_OutPut(0);
	Delay_ms(20);
	DHT11_OutPut(1);
	Delay_us(40);
}

static int DHT11_Check(void)
{
	uint8_t timer = 0;
	DHT11_OutPut(1);
	while(!DHT11_Input && timer < 100)
	{
		timer++;
		Delay_us(1);
	}
	if (timer >= 100)
		return -1;
	else
	{
		timer = 0;
		while(DHT11_Input && timer < 100)
		{
			timer++;
			Delay_us(1);
		}
		if (timer >= 100)
			return -1;
	}
	return 0;
}
static uint8_t DHT11_Read_Byte(void)
{
	uint8_t timer = 0;
	uint8_t data;
	
	for (int i = 0; i < 8; i++)
	{
    timer = 0;
		DHT11_OutPut(1);
		while(!DHT11_Input && timer < 100)
		{
			timer++;
			Delay_us(1);
		}
		data <<= 1;
		timer = 0;
		Delay_us(40);
		if (DHT11_Input == 1)
		{
			data |= 1;
			while(DHT11_Input && timer < 100)
			{
				timer++;
				Delay_us(1);
			}
		}
	}
	return data;
}

void DHT11_Read_Data(uint8_t *humi, uint8_t *temp)
{
	uint8_t data[5];
	DHT11_Start();
	DHT11_Check();
	for (int i = 0; i < 5; i++)
	{
		data[i] = DHT11_Read_Byte();
	}
	Delay_ms(1);
	if ((data[0] + data[1] + data[2] + data[3]) == data[4])
	{
		*humi = data[0];
		*temp = data[2];
	}
}

void DHT11_Init(void)
{
	DHT11_Start();
	if (DHT11_Check())
	{

	}
}



