/**
 * This is the main file of the ESPLaboratory Demo project.
 * It implements simple sample functions for the usage of UART,
 * writing to the display and processing user inputs.
 *
 * @author: Jonathan Müller-Boruttau,
 * 			Tobias Fuchs tobias.fuchs@tum.de
 * 			Nadja Peters nadja.peters@tum.de (RCS, TUM)
 *
 */
#include "includes.h"
#include "gfxconf.h"
#include "math.h"
#include "stdlib.h"
#include "time.h"
#include "stdbool.h"


// start and stop bytes for the UART protocol
static const uint8_t startByte = 0xAA,
					 stopByte  = 0x55;

static const uint16_t displaySizeX = 320,
					  displaySizeY = 240;
uint16_t			  counter_A=0,
					  counter_B=0,
					  counter_C=0,
					  counter_D=0,
					  counter_E=0,
					  regA=0,
					  regB=0,
					  regC=0,
					  regD=0,
					  regE=0,
					  regK=0,
					  jstickX=0,
					  jstickY=0,
					  jstick2X=0, jstick2Y=0;


static double  theta = 0.0,
			   text_x_pos=0.0;


QueueHandle_t ESPL_RxQueue; // Already defined in ESPL_Functions.h
SemaphoreHandle_t ESPL_DisplayReady;

// Stores lines to be drawn
QueueHandle_t JoystickQueue;
const point triangle_points[] = {
		{ 40, 100 },
		{ 60, 50 },
		{ 80, 150 },
};
StaticTask_t xTaskBuffer;

StackType_t xStack[100];
int RedState=0,GreenState=0;
SemaphoreHandle_t xSemaphore = NULL;
static TaskHandle_t xTaskA = NULL;
static TaskHandle_t xExercise2Handle = NULL;
static TaskHandle_t xDrawTaskEx3_2_2 = NULL;
static TaskHandle_t xTaskEx3_2_4 = NULL;
int stateA=0;
int stateB=0;
int stateC=0;
int stateD=0;
int stateE=0;
int stateK=0;
int task_state=0;
int task3_2_4_counter=0;
int main() {
	// Initialize Board functions and graphics
	TaskHandle_t xdrawtaskHandle, xExercise3Handle;

	ESPL_SystemInit();

	// Initializes Draw Queue with 100 lines buffer
	JoystickQueue = xQueueCreate(100, 2 * sizeof(char));

	// Initializes Tasks with their respective priority
	//xTaskCreate(drawTask, "drawTask", 1000, NULL, 5, &xdrawtaskHandle);
	//xTaskCreate(checkJoystick, "checkJoystick", 1000, NULL, 3, NULL);
	//xTaskCreate(DynamicTask, "DynamicTask", 1000 , NULL , 3 ,NULL);
	xTaskCreate(UpdateDisplay, "UpdateDisplay", 1000 , NULL , 5 ,NULL);
	//xTaskCreate(DrawTaskEx3_2_2, "DrawTaskEx3_2", 1000 , NULL ,3   ,&xDrawTaskEx3_2_2);
	//xTaskCreateStatic(StaticTask, "StaticTask", 100 , NULL , 3 ,xStack, &xTaskBuffer);
	//xTaskCreate(Task3_2_3, "Task3_2_3", 1000 , NULL ,1  ,NULL);
	xTaskCreate(checkButtons, "checkButtons", 1000 , NULL ,4  ,NULL);
	//xTaskCreate(Exercise2, "Exercise2", 1000 , NULL ,3  ,&xExercise2Handle);
	//xTaskCreate(ResetStateE, "ResetStateE", 1000 , NULL ,4  ,NULL);
	xTaskCreate(Task3_2_4, "Task3_2_4", 1000 , NULL ,3  , &xTaskEx3_2_4);
	xTaskCreate(ToggleTask3_2_4, "ToggleTask3_2_4", 1000 , NULL ,4  , NULL);
	//xTaskCreate(Task3_2_3_b, "Task3_2_3_b", 1000 , NULL ,1  ,&xTaskA);

	// Start FreeRTOS Scheduler
	vTaskStartScheduler();
}
void ToggleTask3_2_4(){
	char str[100];
	TickType_t xLastWakeTime;
	const TickType_t xFrequency =20;
	xLastWakeTime = xTaskGetTickCount();
	font_t font1; // Load font for ugfx
			font1 = gdispOpenFont("DejaVuSans24*");
	vTaskSuspend(xTaskEx3_2_4);
	int prevstate=0;
	while(1){
		if (stateA==1){
			task_state=!task_state;

		}
		if (task_state==0 && task_state!=prevstate){

			vTaskResume(xTaskEx3_2_4);

		}
		if (task_state==1 && task_state!=prevstate){

			vTaskSuspend(xTaskEx3_2_4);

		}
		//sprintf(str, "The value of counter is: %5d", task3_2_4_counter);
		//gdispDrawString(40,100,str,font1,Black);
		prevstate=task_state;
		vTaskDelayUntil(&xLastWakeTime, xFrequency );
		}


}

void Task3_2_4(){
	char str[100];

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 1000;
	xLastWakeTime = xTaskGetTickCount();
	font_t font1; // Load font for ugfx
	font1 = gdispOpenFont("DejaVuSans24*");
	while(1){

		task3_2_4_counter++;
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		sprintf(str, "The value of counter is: %5d", task3_2_4_counter);
		gdispDrawString(40,100,str,font1,Black);

	}

}

void ResetStateE(){
	TickType_t xLastWakeTime;
			const TickType_t xFrequency = 19;
			xLastWakeTime = xTaskGetTickCount();
	while(1){

		if(stateE == 1)
		{

					counter_E++;
		}
		vTaskDelayUntil(&xLastWakeTime, xFrequency );



	}
}
void Exercise2(){
	TickType_t xLastWakeTime;
		const TickType_t xFrequency = 19;
		xLastWakeTime = xTaskGetTickCount();
		char str[100], str1[100]; // buffers for messages to draw to display
		struct coord joystickPosition; // joystick queue input buffer

		font_t font1; // Load font for ugfx
		font1 = gdispOpenFont("DejaVuSans24*");

		/* building the cave:
		   caveX and caveY define the top left corner of the cave
		    circle movement is limited by 64px from center in every direction
		    (coordinates are stored as uint8_t unsigned bytes)
		    so, cave size is 128px */
		const uint16_t caveX    = displaySizeX/2 - UINT8_MAX/4+40,
					   caveY    = displaySizeY/2 - UINT8_MAX/4+20,
					   caveSize = UINT8_MAX/2;
		uint16_t circlePositionX = caveX,
				 circlePositionY = caveY;



		// Start endless loop
		while(TRUE) {
			// wait for buffer swap
			//Clear the LCD display


			//Ignore noise in the X ADC values
			if(abs(ADC_GetConversionValue(ESPL_ADC_Joystick_2)-jstick2X)>=15)
			{
				//map the value of ADC proportionally to the X axis of the LCD
				jstickX=(ADC_GetConversionValue(ESPL_ADC_Joystick_2)*320/4096)-160;

			}
			jstick2X=ADC_GetConversionValue(ESPL_ADC_Joystick_2);

			//Ignore noise in the Y ADC values
			if(abs(ADC_GetConversionValue(ESPL_ADC_Joystick_1)-jstick2Y)>=15)
			{
				//map the value of ADC proportionally to the Y axis of the LCD
				jstickY=-((ADC_GetConversionValue(ESPL_ADC_Joystick_1)*240/4096)-120-3);
			}
			jstick2Y= ADC_GetConversionValue(ESPL_ADC_Joystick_1);

			//Drawing the triangle at the center using three lines
			gdispDrawLine(160+jstickX,100+jstickY,180+jstickX,140+jstickY,Blue);
			gdispDrawLine(140+jstickX,140+jstickY,180+jstickX,140+jstickY,Blue);
			gdispDrawLine(140+jstickX,140+jstickY,160+jstickX,100+jstickY,Blue);

			//Draw square and circle which revolves around triangle by theta
			gdispFillArea(80*cos(theta+160)+160-20+jstickX, 80*sin(theta+160)+120-20+jstickY, 40, 40, Blue);
			gdispFillCircle(80*cos(theta)+160+jstickX, 80*sin(theta)+120+jstickY ,20, Red);


			//theta is radial position of the square/circle incremented by certain degrees at each iteration
			//the increment value actually determines the speed of rotation/lateral movement of text
			theta=theta+0.05;



			//initialize theta to 0 after one revolution to prevent variable overflow
			if (theta==360.0)
				theta=0.0;


			//top and bottom text
			sprintf(str,"Devil is a lie");


			//display moving text at top and bottom
			gdispDrawString(text_x_pos+jstickX, 2 +jstickY, str, font1, Black);
			gdispDrawString(320-text_x_pos+jstickX, 230+jstickY, str, font1, Black);


			//increment the x axis coordinates of the text to make it appear as moving horizontally
			text_x_pos++;


			//initialize text_x to 0 after completely moving through the screen horizontally to prevent variable overflow
			if (text_x_pos==320)
				text_x_pos=0;

			//##Debouncing A,B,C,D,K Buttons#####
			//edge triggering is implemented here. The counter only increments if the previous value does not equal the current value
			if(stateA==1){

				counter_A++;

			}
			//Store the current value of button for next iteration



			//edge triggering is implemented here. The counter only increments if the previous value does not equal the current value
			if(stateB==1){

				counter_B++;

			}
			//Store the current value of button for next iteration


			//edge triggering is implemented here. The counter only increments if the previous value does not equal the current value
			if(stateC==1){

				counter_C++;

			}
			//Store the current value of button for next iteration

			//edge triggering is implemented here. The counter only increments if the previous value does not equal the current value
			if(stateD==1){

				counter_D++;

			}
			//Store the current value of button for next iteration



			//if button K is pressed reset all the counter values.
			if(stateK==1){

					counter_A=0;
					counter_B=0;
					counter_C=0;
					counter_D=0;
				}

			//Store the values of counters and Axis in str1
			sprintf( str1, "A: %d|B: %d|C %d|D: %d    Axis X: %5d| Axis Y: %5d", counter_A, counter_B, counter_C, counter_D, ADC_GetConversionValue(ESPL_ADC_Joystick_2),ADC_GetConversionValue(ESPL_ADC_Joystick_1));

			//display the values of counters and Axis on the LCD
			gdispDrawString(0+jstickX, 12+jstickY , str1, font1, Black);

			vTaskDelayUntil(&xLastWakeTime, xFrequency );


		}


}

void checkButtons(){
	int state=0;
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 19;
	xLastWakeTime = xTaskGetTickCount();
	while(1){
		if(!GPIO_ReadInputDataBit(ESPL_Register_Button_A, ESPL_Pin_Button_A)&&(!GPIO_ReadInputDataBit(ESPL_Register_Button_A, ESPL_Pin_Button_A))!=regA){

					stateA=1;
			}
		else {

			stateA=0;
		}
				//Store the current value of button for next iteration
		regA=!GPIO_ReadInputDataBit(ESPL_Register_Button_A, ESPL_Pin_Button_A);
		if(!GPIO_ReadInputDataBit(ESPL_Register_Button_B, ESPL_Pin_Button_B)&&(!GPIO_ReadInputDataBit(ESPL_Register_Button_B, ESPL_Pin_Button_B))!=regB){

			stateB=1;
		}
		else stateB=0;
					//Store the current value of button for next iteration
		regB=!GPIO_ReadInputDataBit(ESPL_Register_Button_B, ESPL_Pin_Button_B);
		if(!GPIO_ReadInputDataBit(ESPL_Register_Button_C, ESPL_Pin_Button_C)&&(!GPIO_ReadInputDataBit(ESPL_Register_Button_C, ESPL_Pin_Button_C))!=regC){

					stateA=1;
		}
		else stateC=0;
						//Store the current value of button for next iteration
		regC=!GPIO_ReadInputDataBit(ESPL_Register_Button_C, ESPL_Pin_Button_C);

		if(!GPIO_ReadInputDataBit(ESPL_Register_Button_D, ESPL_Pin_Button_D)&&(!GPIO_ReadInputDataBit(ESPL_Register_Button_D, ESPL_Pin_Button_D))!=regD){

					stateD=1;
		}
		else stateD=0;
							//Store the current value of button for next iteration
		regD=!GPIO_ReadInputDataBit(ESPL_Register_Button_D, ESPL_Pin_Button_D);
		if(!GPIO_ReadInputDataBit(ESPL_Register_Button_E, ESPL_Pin_Button_E)&&(!GPIO_ReadInputDataBit(ESPL_Register_Button_E, ESPL_Pin_Button_E))!=regE){

							stateE=1;
		}
		else stateE=0;
									//Store the current value of button for next iteration
		regD=!GPIO_ReadInputDataBit(ESPL_Register_Button_D, ESPL_Pin_Button_D);
		if(!GPIO_ReadInputDataBit(ESPL_Register_Button_K, ESPL_Pin_Button_K)&&(!GPIO_ReadInputDataBit(ESPL_Register_Button_K, ESPL_Pin_Button_K))!=regK){
					stateK=1;
		}
		else stateK=0;
								//Store the current value of button for next iteration
		regK=!GPIO_ReadInputDataBit(ESPL_Register_Button_K, ESPL_Pin_Button_K);
		vTaskDelayUntil(&xLastWakeTime, xFrequency );
	}

}


void Task3_2_3(){
	int count=0;
	char str[100];
	font_t font1; // Load font for ugfx
	font1 = gdispOpenFont("DejaVuSans24*");
    xSemaphore = xSemaphoreCreateBinary();
    gdispClear(White);
    while( 1 ){
    	if( xSemaphore != NULL )
    	    {
    	       	if(stateA == 1)
    	       		xSemaphoreGive(xSemaphore);
    	        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
    	        {
    	        	sprintf(str, "The total count is: %5d ",count );
    	        	//gdispClear(White);
    	        	gdispDrawString(50, 20 , str, font1, Black);
    	        	count++;
    	            //xSemaphoreGive( xSemaphore );
    	        }
    	   //xSemaphoreTake(ESPL_DisplayReady, portMAX_DELAY);
    	   //ESPL_DrawLayer();
        }

 }
}
void Task3_2_3_b(){
	int count=0;
	char str[100];
	font_t font1;
	font1 = gdispOpenFont("DejaVuSans24*");
    while( 1 ){


    	       	if(stateB==1)
    	       		xTaskNotifyGive(xTaskA);
    	        if(ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) == pdTRUE ){
    	        	sprintf(str, "The total count is: %5d ",count );
    	        	//gdispClear(White);
    	        	gdispDrawString(100, 20 , str, font1, Black);
    	        	count++;

    	        }



 }
}



void UpdateDisplay(){
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 20;
	xLastWakeTime = xTaskGetTickCount();
	while(1)
	{
		ESPL_DrawLayer();
		xSemaphoreTake(ESPL_DisplayReady, portMAX_DELAY);
		gdispClear(White);

		vTaskDelayUntil(&xLastWakeTime, xFrequency );
		//gdispClear(White);



	}

}
void DrawTaskEx3_2_2(){
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 100;
	xLastWakeTime = xTaskGetTickCount();
	while(1)
		{
			//gdispClear(White);
			if(GreenState==1)
				gdispFillCircle(100, 120 ,40, Green);
			if(RedState==1)
				gdispFillCircle(200, 120 ,40, Red);


		}

}
void DynamicTask() {
	//vTaskDelay(500);
	TickType_t xLastWakeTime;
	const TickType_t xFrequency =1000;
	xLastWakeTime = xTaskGetTickCount();
	while(1){


		//gdispClear(White);
		//gdispFillCircle(100, 120 ,40, Green);
		//xSemaphoreTake(ESPL_DisplayReady, portMAX_DELAY);
		//ESPL_DrawLayer();
		GreenState=!GreenState;
		vTaskDelayUntil(&xLastWakeTime, xFrequency );
		//vTaskDelay(xDelay);
	}

}
void StaticTask() {


	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 500;
	xLastWakeTime = xTaskGetTickCount();
		while(1){
			//gdispClear(White);
			//gdispFillCircle(200, 120 ,40, Red);
			//xSemaphoreTake(ESPL_DisplayReady, portMAX_DELAY);
			//ESPL_DrawLayer();
			RedState=!RedState;
			vTaskDelayUntil( &xLastWakeTime, xFrequency);
			//vTaskDelay(xDelay);
		}
}



/**
 * Example task which draws to the display.
 */
void drawTask(){
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = 20;
	xLastWakeTime = xTaskGetTickCount();
	vTaskSuspend(xExercise2Handle );
	vTaskSuspend(xDrawTaskEx3_2_2 );
	// Start endless loop
	while(TRUE) {

		if (counter_E==0)
		{
			vTaskResume(xExercise2Handle );
			vTaskDelayUntil( &xLastWakeTime, xFrequency);

		}

		if (counter_E==1){

					vTaskSuspend(xExercise2Handle);
					vTaskResume(xDrawTaskEx3_2_2);

					vTaskDelayUntil( &xLastWakeTime, xFrequency);
		}
		 if (counter_E==2){
			 	 	 vTaskSuspend(xDrawTaskEx3_2_2 );



					 counter_E=0;

		 }







	}
}


/**
 * This task polls the joystick value every 20 ticks
 */
void checkJoystick() {
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	struct coord joystickPosition = {0, 0};
	const TickType_t tickFramerate = 20;

	while (TRUE) {
		// Remember last joystick values
		joystickPosition.x =
					(uint8_t) (ADC_GetConversionValue(ESPL_ADC_Joystick_2) >> 4);
		joystickPosition.y = (uint8_t) 255 -
						 (ADC_GetConversionValue(ESPL_ADC_Joystick_1) >> 4);

		xQueueSend(JoystickQueue, &joystickPosition, 100);

		// Execute every 20 Ticks
		vTaskDelayUntil(&xLastWakeTime, tickFramerate);
	}
}

/**
 * Example function to send data over UART
 *
 * Sends coordinates of a given position via UART.
 * Structure of a package:
 *  8 bit start byte
 *  8 bit x-coordinate
 *  8 bit y-coordinate
 *  8 bit checksum (= x-coord XOR y-coord)
 *  8 bit stop byte
 */
void sendPosition(struct coord position) {
	const uint8_t checksum = position.x^position.y;

	UART_SendData(startByte);
	UART_SendData(position.x);
	UART_SendData(position.y);
	UART_SendData(checksum);
	UART_SendData(stopByte);
}

/**
 * Example how to receive data over UART (see protocol above)
 */
void uartReceive() {
	char input;
	uint8_t pos = 0;
	char checksum;
	char buffer[5]; // Start byte,4* line byte, checksum (all xor), End byte
	struct coord position = {0, 0};
	while (TRUE) {
		// wait for data in queue
		xQueueReceive(ESPL_RxQueue, &input, portMAX_DELAY);

		// decode package by buffer position
		switch(pos) {
		// start byte
		case 0:
			if(input != startByte)
				break;
		case 1:
		case 2:
		case 3:
			// read received data in buffer
			buffer[pos] = input;
			pos++;
			break;
		case 4:
			// Check if package is corrupted
			checksum = buffer[1]^buffer[2];
			if(input == stopByte || checksum == buffer[3]) {
				// pass position to Joystick Queue
				position.x = buffer[1];
				position.y = buffer[2];
				xQueueSend(JoystickQueue, &position, 100);
			}
			pos = 0;
		}
	}
}

/*
 *  Hook definitions needed for FreeRTOS to function.
 */
void vApplicationIdleHook() {
	while (TRUE) {
	};
}

void vApplicationMallocFailedHook() {
	while(TRUE) {
	};
}


/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
