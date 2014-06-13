/*
 * abuch003_testGame2.c
 *
 * Created: 5/30/2014 2:31:22 PM
 *  Author: student
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include <usart.h>
#include <io.h>
#include "io.c"

unsigned long int findGCD(unsigned long int a, unsigned long int b){
	unsigned long int c;

	while(1){

		c = a%b;

	if(c==0){return b;}
	a = b;
	b = c;

}
return 0;
}

typedef struct _task {


	signed char state; //Task's current state

	unsigned long int period; //Task period

	unsigned long int elapsedTime; //Time elapsed since last task tick

	int (*TickFct)(int); //Task tick function

} task;


void set_PWM(double frequency) {

static double current_frequency; // Keeps track of the currently set frequency

// Will only update the registers when the frequency changes, otherwise allows 

// music to play uninterrupted.

if (frequency != current_frequency) {

if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter

else { TCCR3B |= 0x03; } // resumes/continues timer/counter

// prevents OCR3A from overflowing, using prescaler 64

// 0.954 is smallest frequency that will not result in overflow

if (frequency < 0.954) { OCR3A = 0xFFFF; }

// prevents OCR3A from underflowing, using prescaler 64

// 31250 is largest frequency that will not result in underflow

else if (frequency > 31250) { OCR3A = 0x0000; }

// set OCR3A based on desired frequency

else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

TCNT3 = 0; // resets counter

current_frequency = frequency; // Updates the current frequency

}

}

void PWM_on() {

TCCR3A = (1 << COM3A0);

TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);

// COM3A0: Toggle PB6 on compare match between counter and OCR3A

// WGM32: When counter (TCNT3) matches OCR3A, reset counter

// CS31 & CS30: Set a prescaler of 64

set_PWM(0);

}

void PWM_off() {

TCCR3A = 0x00;

TCCR3B = 0x00;

}

unsigned char getxpos=0;
unsigned char getypos=0;
unsigned char setAlarm=0;
unsigned char Alarm_disabled=0;
unsigned char checkDisable_req=0;
unsigned char intruderAlert=0;
unsigned char row[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char col[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	
	
	
/*roomCheck makes reads the xpos and ypos from the other microcontroller
and with this, determines if it should set the alarm or not.
*/
enum R_States {R_init,R_receive};
	
int roomCheck(int state){
	switch(state){
		case -1:
			state=R_init;
			break;
		case R_init:
				state=R_receive;	
			break;
		case R_receive:
			state=R_receive;
			break;
		default:
			state=R_init;
			break;
		
	}
	switch(state){
		case R_init:
			setAlarm=0;
			break;
		case R_receive:
				if (USART_HasReceived(0)) {
						getxpos=USART_Receive(0);	
				}
				if(USART_HasReceived(1)){
						getypos=USART_Receive(1);	
				}
				setAlarm=0;
				if(getxpos==4 ||(getypos==0 && getxpos<5)||getypos==4){
					LCD_DisplayString(1,"Hallway");	
					if(getxpos==0 && getypos==0){
						Alarm_disabled=0;	
					}					
				}
				else if(getxpos>4 && getypos<5){
					if(!Alarm_disabled){
						setAlarm=1;
					}
					else{
						setAlarm=0;	
						LCD_DisplayString(1,"Kitchen ");	
					}			
				}
				else if(getypos==2 && getxpos!=4 ||getypos==3){
					if(!Alarm_disabled){
						setAlarm=1;
					}
					else{
						setAlarm=0;	
						LCD_DisplayString(1,"Living Room ");	
					}			
				}
				else if(getxpos>4 && getypos>4){
						if(!Alarm_disabled){
						setAlarm=1;
					}
					else{
						setAlarm=0;	
						LCD_DisplayString(1,"Bed Room A ");	
					}			
				}
				else{
					if(!Alarm_disabled){
						setAlarm=1;
					}
					else{
						setAlarm=0;	
						LCD_DisplayString(1,"Bed Room B ");	
					}			
				}
				if(USART_IsSendReady(0)){
					USART_Send(!setAlarm,0);
				}
			break;
		default:
			break;	
				
	}
	return state;
}		

/* once getAlarm is set from roomChec, Alarm_Tick will keep the Alarm on for a few ticks and off for
another few ticks. It will also set checkDisable_req=1 to notify Disable_Tick SM. If intruderAlert=1, there will
be a different alarm pattern.*/

enum Alarm_States {Alarm_init,Alarm_waitForRise,Alarm_On,Alarm_Off,Alarm_disable,Alarm_shutdown};

int Alarm_Tick(int state){
	static unsigned char count=0; //on for 8 ticks and off for 3
	static unsigned char intruderAlarm=0;
	switch(state){
		case -1:
			state=Alarm_init;
			break;
		case Alarm_init:
			state=Alarm_waitForRise;
				break;
		case Alarm_waitForRise:
			if(setAlarm){
				checkDisable_req=1;
				LCD_DisplayString(1,"Disable Alarm!");
				state=Alarm_On;	
			}
			else{
				state=Alarm_waitForRise;	
			}
				break;
		case Alarm_On:
			count++;
			if(Alarm_disabled){
				LCD_DisplayString(1,"Alarm Disabled!");
				state=Alarm_disable;
			}
			if(intruderAlarm>7){
				state=Alarm_shutdown;	
			}
			else if (count<8){
				state=Alarm_On;	
			}
			else if(count>8){
				if(intruderAlert){
					intruderAlarm++;
				}
				count=0;
				state=Alarm_Off;	
			}
			break;
		case Alarm_Off:
			count++;
			if(count<3){
				state=Alarm_Off;
			}
			else{
				count=0;
				state=Alarm_On;	
			}
			break;
		case Alarm_disable:
			state=Alarm_waitForRise;
			break;
		case Alarm_shutdown:
			state=Alarm_shutdown;
			break;
		default:
			state=Alarm_init;
			break;
	}
	
	switch(state){
		case Alarm_init:
			break;
		case Alarm_On:
			if(intruderAlert){
				set_PWM(493.88);
				LCD_DisplayString(1,"INTRUDER ALERT");
			}
			else{
				set_PWM(349.23);
			}
			break;
		case Alarm_Off:
			if(intruderAlert){
				set_PWM(349.23);
			}
			else{
				set_PWM(0);
			}
			break;
		case Alarm_disable:
			checkDisable_req=0;
			intruderAlarm=0;
			set_PWM(0);
			break;
		case Alarm_shutdown:
			LCD_DisplayString(1,"System Shutdown");
			set_PWM(0);
			break;
	}
	
	return state;
}

/* Disable_Tick will check if the player has entered the password correctly.
If it has entered it correctly in time, then player can go around the house, otherwise
the "lights" will stay off and it will set intruderAlert=1 notifying the other SMs to make the necessary
actions.*/

enum Disable_States {Disable_init,Disable_waitForReq,Disable_waitRise,Disable_countNum,Disable_waitFall,Disable_disAlarm,Disable_armSystem};

int Disable_Tick(int state){
	static unsigned intruderCheck=0;
	static unsigned char alarmCount=0;
	switch(state){
		case -1:
			state=Disable_init;
			break;
		case Disable_init:
			state=Disable_waitForReq;
			break;
		case Disable_waitForReq:
			if(checkDisable_req){
				state=Disable_waitRise;
			}
			else{
				state=Disable_waitForReq;					
			}
			break;
		case Disable_waitRise:
			intruderCheck++;
			if(intruderCheck>70){
				intruderAlert=1;	
				state=Disable_armSystem;
			}
			if(~PINA & 0x10){
				intruderCheck=0;
				state=Disable_countNum;	
			}
			else{
				state=Disable_waitRise;	
			}
			break;
		case Disable_countNum:
			state=Disable_waitFall;
				break;
		case Disable_waitFall:
			intruderCheck++;
			if(intruderCheck>70){
				intruderAlert=1;
				state=Disable_armSystem;
			}
			if(alarmCount>2){
				Alarm_disabled=1;
				state=Disable_disAlarm;
			}
			else if(~PINA & 0x10){
				state=Disable_waitFall;	
			}
			else{
				intruderCheck=0;
				state=Disable_waitRise;	
			}
			break;
		case Disable_disAlarm:
			state=Disable_waitForReq;
			break;
		case Disable_armSystem:
			state=Disable_armSystem;
			break;
		default:
			state=Disable_init;
			break;
	}
	switch(state){
		case Disable_init:
			intruderCheck=0;
			Alarm_disabled=0;
			alarmCount=0;
			break;
		case Disable_waitRise:
			break;
		case Disable_countNum:
			alarmCount++;
			break;
		case Disable_waitFall:
			LCD_init();
			LCD_WriteData(alarmCount+ '0');
			break;
		case Disable_disAlarm:
			intruderCheck=0;
			Alarm_disabled=1;
			alarmCount=0;
			break;
		case Disable_armSystem:
			Alarm_disabled=0;
			break;
		default:
			break;
	}
	
	return state;
}
	
int main(void)
{
		DDRA=0x0F;PORTA=0xF0;
		DDRC=0xFF;PORTC=0x00;
		DDRB=0xFF;PORTB=0x00;
		LCD_init();
		PWM_on();
	unsigned long int Alarm_calc = 100;
	unsigned long int roomCheck_calc =100;
	unsigned long int Disable_calc=100;


	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(Alarm_calc, roomCheck_calc);
	tmpGCD = findGCD(tmpGCD, Disable_calc);
	//tmpGCD = findGCD(tmpGCD, SMTick4_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int Alarm_period = Alarm_calc/GCD;
	unsigned long int roomCheck_period = roomCheck_calc/GCD;
	unsigned long int Disable_period = Disable_calc/GCD;
	//unsigned long int SMTick4_period = SMTick4_calc/GCD;

	//Declare an array of tasks
	static task task1,task2,task3;// task2, task3; // task3, task4;
task *tasks[] = { &task1,&task2,&task3};
const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

// Task 1
task1.state = -1;//Task initial state.
task1.period = roomCheck_period;//Task Period.
task1.elapsedTime = roomCheck_period;//Task current elapsed time.
task1.TickFct = &roomCheck;//Function pointer for the tick.

// Task 2
task2.state = -1;//Task initial state.
task2.period = Alarm_period;//Task Period.
task2.elapsedTime = Alarm_period;//Task current elapsed time.
task2.TickFct = &Alarm_Tick;//Function pointer for the tick.

//Task 3
task3.state = -1;//Task initial state.
task3.period = Disable_period;//Task Period.
task3.elapsedTime = Disable_period;//Task current elapsed time.
task3.TickFct = &Disable_Tick;//Function pointer for the tick.


// Set the timer and turn it on
TimerSet(GCD);
TimerOn();
initUSART(0);
initUSART(1);
//initUSART(1);
unsigned short i; // Scheduler for-loop iterator
while(1) {
	
	for ( i = 0; i < numTasks; i++ ) {
		
		if ( tasks[i]->elapsedTime == tasks[i]->period ) {
			
			tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			tasks[i]->elapsedTime = 0;
		}
		tasks[i]->elapsedTime += 1;
	}
	
	while(!TimerFlag);
	TimerFlag = 0;
}

return 0;
}
