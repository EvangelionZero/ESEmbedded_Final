#include <stdint.h>
#include <stdio.h>
#include "reg.h"
#include "blink.h"
#include "usart.h"
#include "asm_func.h"

#define TASK_NUM 4
#define PSTACK_SIZE_WORDS 1024 //user stack size = 4 kB

static uint32_t *psp_array[TASK_NUM];

void setup_systick(uint32_t ticks);

void init_task(unsigned int task_id, uint32_t *task_addr, uint32_t *psp_init)
{
	*(psp_init - 1) = *(psp_init - 1)|0x01000000;	  //xPSR (bit 24, T bit, has to be 1 in Thumb state)
	*(psp_init - 1) = *(psp_init - 1)&0x01000000;
	*(psp_init - 2) = (uint32_t) task_addr;  //Return Address is being initialized to the task entry
	psp_array[task_id] = psp_init - 16;//initialize psp_array (stack frame: 8 + r4 ~ r11: 8)
}

void task0(void)
{
	blink(LED_RED); //should not return
	//printf("task0 debug /n/n");
	
}

void task1(void)
{
	blink(LED_GREEN); //should not return
	//printf("task1 debug /n/n");
}

void task2(void)
{
	blink(LED_ORANGE); //should not return
	//printf("task2 debug /n/n");
}

void task3(void)
{
	static int fib[3]={1,1,2};
	static int i=0;
	while(1){
	//printf("[Task3] fibonacci sequence, Start in unprivileged thread mode.\r\n\n");
		if(fib[2]==2)
			printf("%d\r\n%d\r\n%d\r\n",fib[0],fib[1],fib[2]);
		while(fib[2]<10000){
			blink_count(LED_BLUE,1);
			fib[0] = fib[1];
			fib[1] = fib[2];
			fib[2] = fib[0] + fib[1];
			while(i<500000){
				i++;
			}
			i=0;
			printf("%d\r\n",fib[2]);
		}
		printf("\r\n\n");
		fib[0] = 1;
		fib[1] = 1;
		fib[2] = 2;
		}
}

int main(void)
{
	init_usart1();

	uint32_t user_stacks[TASK_NUM][PSTACK_SIZE_WORDS];

	//init user tasks
	init_task(0, (uint32_t *)task0 ,user_stacks[0]+1024);
	init_task(1, (uint32_t *) task1 ,user_stacks[1]+1024);
	init_task(2, (uint32_t *) task2 ,user_stacks[2]+1024);
	init_task(3, (uint32_t *) task3 ,user_stacks[3]+1024);

	printf("[Kernel] Start in privileged thread mode.\r\n\n");

	printf("[Kernel] Setting systick...\r\n\n");
	setup_systick(168e6 / 8 / 50); //200 ms

	//start user task
	printf("[Kernel] Switch to unprivileged thread mode & start user task0 with psp.\r\n\n");
	start_user((uint32_t *)task0, user_stacks[0]);

	while (1) //should not go here
		;
}

void setup_systick(uint32_t ticks)
{
	// set reload value
	WRITE_BITS(SYST_BASE + SYST_RVR_OFFSET, SYST_RELOAD_23_BIT, SYST_RELOAD_0_BIT, ticks - 1);

	// uses external reference clock
	CLEAR_BIT(SYST_BASE + SYST_CSR_OFFSET, SYST_CLKSOURCE_BIT);

	// enable systick exception
	SET_BIT(SYST_BASE + SYST_CSR_OFFSET, SYST_TICKINT_BIT);

	// enable systick
	SET_BIT(SYST_BASE + SYST_CSR_OFFSET, SYST_ENABLE_BIT);
}

uint32_t *sw_task(uint32_t *psp)
{
	static unsigned int curr_task_id = 0;//最一開始的task是task0

	psp_array[curr_task_id] = psp; //save current psp

	if (++curr_task_id > TASK_NUM - 1) //get next task id
		curr_task_id = 0;

	return  psp_array[curr_task_id];//return next psp
}