#include <stdint.h>
#include "FreeRTOS.h"

typedef enum {
	IDLE = 1,
	ACTIVE
} task_status;

typedef struct {
	volatile uint32_t sp;
	void (*handler)(void *args);
	void *args;
	volatile task_status status;
} task_header;

static struct {
	task_header tasks[OS_CONFIG_MAX_TASKS];
	volatile uint32_t current_task;
	uint32_t size;
} task_dir;

typedef struct {
	uint32_t time;
	uint32_t taskid;
	uint8_t isset;
} request_t;

volatile task_header *current_task;
volatile task_header *next_task;
volatile uint32_t tickcounter = 0;
volatile uint32_t taskStart = 0;
volatile uint32_t notify_scheduler = 0;
volatile uint8_t activeTasks[OS_CONFIG_MAX_TASKS] = {0};
volatile uint8_t notifyArray[OS_CONFIG_MAX_TASKS] = {0};
volatile request_t requestArray[OS_CONFIG_MAX_TASKS];
volatile uint8_t resurfacedFromDelay[OS_CONFIG_MAX_TASKS];
volatile uint32_t interruptTime = UINT32_MAX;
volatile uint8_t incritical = 0;
static uint32_t init_task_stack[1024];
volatile uint32_t didinit = 0;

static void task0_handler(void* bullshit);

static void task_finished(void)
{
	/* This function is called when some task handler returns. */
	volatile uint32_t i = 0;
	while (1)
		i++;
}

void trigger_switch(void) {
	notify_scheduler = 1;
}

volatile uint32_t get_trigger() {
	return notify_scheduler;
}

void printActiveTasks() {
	for(int i = 0; i < task_dir.size; i++) {
		Serial.print("Task ");
		Serial.print(i);
		Serial.print(" has the active status: ");
		Serial.print(activeTasks[i]);
		Serial.print("\n");
	}
}

static __inline__ void * get_pc(void)  {
    void *pc;
    asm("mov %0, pc" : "=r"(pc));
    return pc;
}

static __inline__ void * get_lr(void)  {
    void *lr;
    asm("mov %0, lr" : "=r"(lr));
    return lr;
}

void HardFault_Handler(void)
{
    Serial.println("hard fault...");
    __DSB();
    volatile uint32_t fault = *(uint32_t *) 0xE000ED28;
    __DSB();
    Serial.println(fault);
    __DSB();
    Serial.println("and this is the actual cfsr");
    Serial.println(SCB->CFSR);
    uint32_t newval = *(uint32_t*)get_pc();
    Serial.println(newval);
    newval = *(uint32_t*)get_lr();
    Serial.println(newval);
    while(1);
}

void set_task(uint32_t taskid, uint8_t ta) {
	activeTasks[taskid] = ta;
}

void begin_critical() {
	incritical = 1;
}

void end_critical() {
	incritical = 0;
}

bool initialize_task(void (*handler)(void* args), void* args, uint32_t *p_stack, uint32_t stack_size)
{
	if (task_dir.size >= OS_CONFIG_MAX_TASKS-1)
		return false;

	task_header *p_task = &task_dir.tasks[task_dir.size];
	p_task->handler = handler;
	p_task->args = args;
	p_task->sp = (uint32_t)(p_stack+stack_size-17);
	p_task->status = IDLE;
	p_stack[stack_size-1] = 0x01000000;
	p_stack[stack_size-2] = (uint32_t) &rpi_init_trampoline;
	p_stack[stack_size-3] = (uint32_t) &task_finished;
	p_stack[stack_size-12] = (uint32_t) handler;
	p_stack[stack_size-11] = (uint32_t) args;
	p_stack[stack_size-17] = 0xFFFFFFFD;
	activeTasks[task_dir.size] = 1;
	task_dir.size++;
	return true;
}

void setTask(void (*code)(void* args), void* args, uint32_t taskpriority, uint32_t *xStack, uint32_t stack_size){
	if (didinit == 0) {
        memset(&task_dir, 0, sizeof(task_dir));
		// the idle spin task
		initialize_task(&task0_handler, NULL, init_task_stack, 1024);
		didinit = 1;
	}

	initialize_task(code, args, xStack, stack_size);
}

void delayTask(uint32_t delay_ticks, uint32_t taskid) {
	incritical = 1;
	if (activeTasks[taskid] == 1) {
		activeTasks[taskid] = 0;
		for (int i = 0; i <OS_CONFIG_MAX_TASKS; i++) {
			if (requestArray[i].isset == 0) {
				requestArray[i].taskid = taskid;
				requestArray[i].time = tickcounter + delay_ticks;
				if (interruptTime == 0) {
					interruptTime = requestArray[i].time;
				} else if (requestArray[i].time < interruptTime) {
					interruptTime = requestArray[i].time;
				}
				requestArray[i].isset = 1;
				notify_scheduler = 1;
				incritical = 0;
				while(notify_scheduler == 1);
				return;
			}
		}
	}
}

uint32_t get_tick_counter() {
	return tickcounter;
}

void delayTaskUntil(uint32_t* lasttime, uint32_t delay_ticks, uint32_t taskid) {
	incritical = 1;
	if (activeTasks[taskid] == 1) {
		activeTasks[taskid] = 0;
		for (int i = 0; i <OS_CONFIG_MAX_TASKS; i++) {
			if (requestArray[i].isset == 0) {
				requestArray[i].taskid = taskid;
				requestArray[i].time = *lasttime + delay_ticks;
				*lasttime = requestArray[i].time;
				if (interruptTime == 0) {
					interruptTime = requestArray[i].time;
				} else if (requestArray[i].time < interruptTime) {
					interruptTime = requestArray[i].time;
				}
				requestArray[i].isset = 1;
				notify_scheduler = 1;
				incritical = 0;
				while(notify_scheduler == 1);
				return;
			}
		}
	}
}

uint32_t xTaskGetTickCount() {
    return get_tick_counter();
}

void yieldFromISR(){
	// __ISB();
	if (incritical == 0) {
		if (notify_scheduler == 1) {
			notify_scheduler = 0;
			
			/* Select next task: */
			uint32_t newtask = OS_CONFIG_MAX_TASKS-1;
			while (activeTasks[newtask] == 0) {
				newtask--;
			}

			current_task = &task_dir.tasks[task_dir.current_task];
			current_task->status = IDLE;
			
			task_dir.current_task = newtask;

			next_task = &task_dir.tasks[task_dir.current_task];
			next_task->status = ACTIVE;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	} else {
		if (notify_scheduler == 1) {
			notify_scheduler = 0;
			uint32_t newtask = task_dir.current_task;
			current_task = &task_dir.tasks[task_dir.current_task];
			current_task->status = IDLE;
			
			task_dir.current_task = newtask;

			next_task = &task_dir.tasks[task_dir.current_task];
			next_task->status = ACTIVE;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
}


void startTask() {
	NVIC_SetPriority(PendSV_IRQn, 0xff); /* Lowest possible priority */
	Serial.println("os start");
	Serial.println(task_dir.size);
	uint32_t curctrl = __get_CONTROL();
	Serial.println("control");
	Serial.println(curctrl);
	task_dir.current_task = task_dir.size-1;
	taskStart = 1;
	/* Start the first task: */
	current_task = &task_dir.tasks[task_dir.current_task];
    current_task->status = ACTIVE;
	__set_PSP(current_task->sp+68); /* Set PSP to the top of task's stack */
	__set_CONTROL(0x02); /* Switch to PSP, privilleged mode */
	__ISB(); /* Exec. ISB after changing CONTROL (recommended) */

	current_task->handler(current_task->args);
	while (1);
}

extern "C" {
	int sysTickHook(){
		if (taskStart == 1) {
			tickcounter++; 
		}

		if (incritical == 0) {
		if (tickcounter >= interruptTime) {
			uint32_t newInterruptTime = UINT32_MAX;
			for (int i = 0; i < OS_CONFIG_MAX_TASKS; i++) {
				if (requestArray[i].isset == 1) {
					if (requestArray[i].time == interruptTime) {
						activeTasks[requestArray[i].taskid] = 1;
						resurfacedFromDelay[requestArray[i].taskid] = 1;
						if (requestArray[i].taskid > task_dir.current_task) {
							notify_scheduler = 1;
						}
						requestArray[i].isset = 0;
					} else {
						if (requestArray[i].time < newInterruptTime) {
							newInterruptTime = requestArray[i].time;
						}
					}
				}
			}
			interruptTime = newInterruptTime;
		}
		if (notify_scheduler == 1) {
			notify_scheduler = 0;
			
			uint32_t newtask = OS_CONFIG_MAX_TASKS-1;
			while (activeTasks[newtask] == 0) {
				newtask--;
			}

			if (newtask == task_dir.current_task) {
				return 0;
			}

			current_task = &task_dir.tasks[task_dir.current_task];
			current_task->status = IDLE;
			
			task_dir.current_task = newtask;
			next_task = &task_dir.tasks[task_dir.current_task];
			next_task->status = ACTIVE;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
		}

		return 0;
	}
}


uint32_t xEventGroupWaitBits(uint32_t taskid, uint8_t uxBitsToWaitFor, bool xClearOnExit, uint32_t time, bool forever) {
	resurfacedFromDelay[taskid] = 0;
	if (uxBitsToWaitFor == notifyArray[taskid]) {
		if (xClearOnExit) {
			notifyArray[taskid] &= ~uxBitsToWaitFor;
		}
		return 1;
	}
	if (forever == true) {
		set_task(taskid, 0);
		trigger_switch();
		while(get_trigger() == 1) {
			__NOP();
		}
	} else {
        delayTask(time, taskid);
	}

	while (!((resurfacedFromDelay[taskid] == 1) || (notifyArray[taskid] == uxBitsToWaitFor))) {
		set_task(taskid, 0);
		trigger_switch();
		while(get_trigger() == 1) {
			__NOP();
		}
	}
	for (int i = 0; i < OS_CONFIG_MAX_TASKS; i++) {
		if (requestArray[i].isset == 1) {
			if (requestArray[i].taskid == taskid) {
				requestArray[i].isset = 0;
			}
		}
	}
	
	if (resurfacedFromDelay[taskid] == 1) {
		resurfacedFromDelay[taskid] = 0;
		return 0;
	}

	if (xClearOnExit) {
		notifyArray[taskid] &= ~uxBitsToWaitFor;
	}

	return 1;
}

void set_notify_array(uint32_t taskid, uint8_t uxBitsToWaitFor) {
	notifyArray[taskid] |= uxBitsToWaitFor;
}

static void task0_handler(void* bullshit)
{
	// this is our default "idle" task where we just do nothing.
	while (1);
}