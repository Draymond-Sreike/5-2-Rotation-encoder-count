#include "stm32f10x.h"                  // Device header

static int count;

void encoder_Init()
{
	/*****GPIOB的中断选择配置*******/
	//以下代码配置B0&B1作为中断信号的输入口，即使得与B0&B1连接的硬件作为中断源
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIOB_InitStructure;
	GPIOB_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIOB_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIOB_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIOB_InitStructure);
	
	/*****AFIO的中断选择配置*******/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	//这个函数虽然写着EXTL，但实际上是配置AFIO，然后去跟EXTL产生联系
	//这个函数完成了中断源的选取，即中断输入口的选择，选择了GPIOB0&1口作为中断源输入
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

	/*****EXTI的中断选择配置*******/
	EXTI_InitTypeDef EXTI_InitStructure;
	///因为选择了B0&1作为中断源，所以相应的EXTI_Line选择0&1
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	//事件响应/中断响应，选择中断响应
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	//选择中断为下降沿触发
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStructure);
	
	/*****NVIC的中断选择配置*******/
	//设置抢占和响应的优先级分组为组2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	//因为EXTI是0线过来的，所以选择EXTI0_IRQn
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//优先级组2中抢占和响应优先级的数值可选均为0~3
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	//至此完成B0口中断源与CPU的联系
	
	//因为EXTI是0线过来的，所以选择EXTI1_IRQn
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//优先级组2中抢占和响应优先级的数值可选均为0~3,此处响应优先级选为2，比EXTI线0低一级，但也是任意设置的，无需关注
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStructure);
	//至此完成B1口中断源与CPU的联系
	//注意上述代码对同一个结构体进行了二次利用，这种是可行的
}

int get_EncoderCount()
{
	return count;
}


//反转开始时B会有一个离开下降沿，即B1（B）口会接收到一个离开下降沿，而后A会产生一个接入下降沿，标志旋转到位
//此时读取B0（A）口的下降沿再读取B1口是否处于低电平状态
//若是正转，B0口会先出现下降沿，反转中断函数被执行，由于此时B1（B）还未旋入，没有下降，仍然处于高电平，此时无法满足反转中断函数要求
//	反转中断函数的功能无法执行
//	待到B1（B）旋入时，产生下降沿后，执行后一个的正转中断函数，此时B0口早已经处于低电平，符合正转中断函数的执行要求，其功能可被执行
//	以上说明正转时，其实两个中断都会被执行，只是正转时，反转函数的要求无法满足，其功能无法实现
//若是反转，B1（B）口会先出现下降沿，此时正转中断函数执行，此时由于B0（A）还未旋入，没有下降，仍然处于高电平，此时无法满足正转中断函数要求
//  正转中断函数的功能无法执行
//	待到B0（A）旋入时，产生下降沿后，执行后一个的反转中断函数，此时B1口早已经处于低电平，符合反转中断函数的执行要求，其功能可被执行
//	同理说明反转时，其实两个中断都会被执行，只是反转时，正转函数的要求无法满足，其功能无法实现

/**************反转中断函数******************/
void EXTI0_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line0) == SET)
	{
		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
		{
			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
			{
				count --;
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line0);
	}

//	if((EXTI_GetITStatus(EXTI_Line0)) == SET)
//	{
//		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
//			count--;
//		EXTI_ClearITPendingBit(EXTI_Line0);
//	}
}

/**************正转中断函数******************/
void EXTI1_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line1) == SET)
	{
		/*如果出现数据乱跳的现象，可再次判断引脚电平，以避免抖动*/
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
		{
			if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0)
			{
				count ++;
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line1);

//	if((EXTI_GetITStatus(EXTI_Line1)) == SET)
//	{
//		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
//			count++;
//		EXTI_ClearITPendingBit(EXTI_Line1);
//	}
	}
}
