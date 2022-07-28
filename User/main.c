#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "CountSensor.h"
#include "Encoder.h"

int main()
{
	static int num;
	OLED_Init();
	encoder_Init();
	OLED_ShowString(1, 1, "Count:");
	while(1)
	{
		OLED_ShowSignedNum(1, 7, get_EncoderCount(), 5);
	}
	return 0;
}
