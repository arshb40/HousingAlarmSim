#include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include <io.h>
#include "io.c"
#include <usart.h>


unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
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


unsigned char data=0;
unsigned char shiftReg=0;

void GetNESData(){
	shiftReg=SetBit(shiftReg,1,1);
	PORTA=shiftReg;
	shiftReg=SetBit(shiftReg,1,0);
	PORTA=shiftReg;
		for(unsigned char i=0;i<8;i++){
			data=SetBit(data,i,GetBit(PINA & 0x01,0));
			shiftReg=SetBit(shiftReg,2,1);
			PORTA=shiftReg;
			shiftReg=SetBit(shiftReg,2,0);
			PORTA=shiftReg;
		}
}


// --------Shared Variables--------------------------------------------------
unsigned char start_req=0;
unsigned char setUp_req=0;

// all of the possible positions that the cursor can move on the LED Matrix 
unsigned char row[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char col[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	
//the positions of the room walls in the "house" 
unsigned char houserow[9]={0xE0,0x20,0xA0,0x10,0x0D,0x10,0x0A,0x08,0x02};
unsigned char housecol[9]={0x08,0x05,0x20,0xE0,0x20,0xE0,0x08,0x0D,0x07};
	
	
unsigned char column_val = 0x01; // sets the pattern displayed on columns
unsigned char column_sel = 0x7F; // grounds column to display pattern
  	
unsigned char xpos=0;
unsigned char ypos=0;
unsigned char disabled=1;


//--------End Shared Variables------------------------------------------------

//--------User defined FSMs---------------------------------------------------
//Enumeration of states.
enum Sim_States {sim_init,sim_waitStart,sim_waitdePress, sim_startSim};

// Monitors button connected to PA0. 
// When button is pressed, shared variable "pause" is toggled.
int beginSim(int state) {
		
    switch (state) {
		case -1:
			state=sim_init;
			break;
		case sim_init:
			state=sim_waitStart;
			break;
		case sim_waitStart:
			if(~data & 0x08){
				start_req=1;
				state=sim_waitdePress;	
				
			}
			else{
				state=sim_waitStart;	
			}
			break;
		case sim_waitdePress:
			if(data & 0x08){
				state=sim_startSim;	
			}
			else{
				state=sim_waitdePress;	
			}
			break;
		case sim_startSim:
			if(data & 0x08){
				state=sim_startSim;	
			}
			else{
				start_req=0;
				state=sim_waitStart;	
			}
			break;
		default:
			state=sim_init;
			break;
    }

    switch(state) {
		case sim_init:
			start_req=0;
			break;
		case sim_waitStart:
			break;
		case sim_waitdePress:
			break;
		case sim_startSim:
			break;
		default:
			break;
    }

    return state;
}

enum M_States {M_init,M_setUpHouse,M_waitForMove,M_up,M_down,M_left,M_right,M_waitup,M_waitdown,M_waitleft,M_waitright};
	
int Move_Tick(int state) {
	
    // === Transitions ===
	  switch (state) {
		  
   		case -1:
		   setUp_req=0;
			state=M_init;
			break;
			
		case M_init:
			if(start_req){
				ypos=0;
				xpos=0;
				state=M_setUpHouse;
			}
			else{
				state=M_init;	
			}
			break;
					
		case M_setUpHouse:
			setUp_req=1;
			state=M_waitForMove;
			break;
			
		case M_waitForMove:		
			if(USART_HasReceived(0)){
				disabled=USART_Receive(0);
			}
			if(disabled){
				if(start_req==0){
					state=M_init;	
				}
				else if(~data & 0x10){
					state=M_up;
				}
				else if(~data & 0x20){					
					state=M_down;	
				}
				else if(~data & 0x80){
					state=M_right;	
				}
				else if(~data & 0x40){		
					state=M_left;	
				}
				else{
					state=M_waitForMove;	
				}
			}
			else{
				state=M_waitForMove;	
			}
			break;
				
		case M_up:
			state=M_waitup;
			break;
			
		case M_down:
			state=M_waitdown;
			break;
			
		case M_left:
			state=M_waitleft;
			break;
			
		case M_right:
			state=M_waitright;
			break;
			
		case M_waitup:
			if(~data & 0x10){
				state=M_waitup;	
			}
			else if (~data & 0x20){
				state=M_down;	
			}
			else if(~data & 0x80){
				state=M_right;	
			}
			else if(~data & 0x40){
				state=M_left;	
			}
			else{
				state=M_waitForMove;	
			}
			break;
			
		case M_waitdown:
			 if(~data & 0x20){
				state=M_waitdown;
			}
			else if(~data & 0x10){
				state=M_up;	
			}
			else if (~data & 0x80){
				state=M_right;
			}
			else if(~data & 0x40){
				state=M_left;	
			}
			else{
				state=M_waitForMove;	
			}
			break;
			
		case M_waitright:
			if(~data & 0x80){
				state=M_waitright;	
			}
			else if(~data & 0x40){
				state=M_left;	
			}
			else if(~data & 0x20){
				state=M_down;
			}
			else if(~data & 0x10){
				state=M_up;	
			}
			else{
				state=M_waitForMove;	
			}
			break;
			
		case M_waitleft:
			if(~data & 0x40){
				state=M_waitleft;	
			}
			else if(~data & 0x80){
				state=M_waitright;	
			}
			else if(~data & 0x20){
				state=M_down;
			}
			else if(~data & 0x10){
				state=M_up;	
			}
			else{
				state=M_waitForMove;
			}	
			break;
				
		default:
			state=M_init;
			break;	
    }
    // === Actions ===
    switch (state) {
		
		case M_init:
			column_val=0x00;
			column_sel=0x00;
			break;
			
		case M_setUpHouse:
			break;
			
   		case M_waitForMove:	   
		   if(USART_IsSendReady(0)){
				 USART_Send(xpos,0);
			}
			if(USART_IsSendReady(1)){
				USART_Send(ypos,1);	
			}
			column_val=row[ypos];
			column_sel=col[xpos];
			break;
			
		case M_up:
			if(ypos<7){
				ypos++;
				
				for(unsigned char i=0;i<9;i++){
					if((row[ypos] & houserow[i]) >0){
						if((col[xpos] & housecol[i])>0){
							ypos--;
						}
					}
				}
			}
			
			column_val= row[ypos];
			column_sel= col[xpos];
			break;
			
		case M_down:
			if(ypos>0){
				ypos--;
				
				for(unsigned char i=0;i<9;i++){
					if((row[ypos] & houserow[i]) >0){
						if((col[xpos] & housecol[i])>0){
							ypos++;
						}
					}
				}
			}
			column_val=row[ypos];
			column_sel=col[xpos];
			break;
			
		case M_left:
			if(xpos>0){
				xpos--;
				
				for(unsigned char i=0;i<9;i++){
					if((row[ypos] & houserow[i]) >0){
						if((col[xpos] & housecol[i])>0){
							xpos++;
						}
					}
				}
			}
			column_val=row[ypos];
			column_sel=col[xpos];
			break;
			
		case M_right:
			if(xpos<7){
				xpos++;
				
				for(unsigned char i=0;i<9;i++){
					if((row[ypos] & houserow[i]) >0){
						if((col[xpos] & housecol[i])>0){
							xpos--;
						}
					}
				}
			}
			column_val=row[ypos];
			column_sel=col[xpos];
			break;
			
		case M_waitup:
			break;
			
		case M_waitdown:
			break;
			
		case M_waitleft:
			break;
			
		case M_waitright:
			break;
			
		default:
			break;
	}	
	
    PORTC = ~column_val; // PORTA displays column pattern
    PORTB = column_sel; // PORTB selects column to display pattern

    return state;    
};


/*House_Tick will set up all the "rooms" in the house. It basically goes through each column
and sets the required row values to 1. Since it ticks every millisecond, 
it seems as if the LEDS are always lit up.*/

enum House_States {H_init,H_waitsetUp,H_setUp};
	
int House_Tick(int state){
	static unsigned char counter=0;
	switch(state){
		case -1:
			state=H_init;
			break;
		case H_init:
			state=H_waitsetUp;
			break;
		case H_waitsetUp:
			if(setUp_req){
				state=H_setUp;
			}
			else{
				state=H_waitsetUp;	
			}
		case H_setUp:
			if(setUp_req){
				state=H_setUp;
			}
			else{
				
				state=H_init;
			}
			break;
		default:
			break;	
	}
	switch(state){
		case H_init:
		column_val=0xFE;
		column_sel=0x01;
			break;
		case H_waitsetUp:
			break;
		case H_setUp:
			if(disabled){
				if(counter==0){
					column_val=0x2A;
					column_sel=0x01;	
					counter++;
				}
				else if(counter==1){
					column_val=0x02;
					column_sel=0x02;
					counter++;
				}
				else if(counter==2){
					column_val=0x2A;
					column_sel=0x04;	
					counter++;
				}
				else if(counter==3){
					column_val=0xEA;
					column_sel=0x08;
					counter++;
				}
				else if(counter==4){
					column_val=0xBD;
					column_sel=0x20;	
					counter++;
				}
				else if(counter==5){
					column_val=0x10;	
					column_sel=0x40;
					counter++;
				}
				else if(counter==6){
					column_val=0x10;
					column_sel=0x80;	
					counter++;
				}
				else{
					column_val=row[ypos];
					column_sel=col[xpos];	
					counter=0;	
				}
			}
			break;
		default:
			break;
	}
	
	 PORTC = ~column_val; // PORTA displays column pattern
    PORTB = column_sel; // PORTB selects column to display pattern

    return state;    
}


// --------END User defined FSMs-------------------------------------------

int main()
{


	DDRA = 0xFE; PORTA = 0x01;
	DDRC = 0xFF; PORTC = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	// Period for the tasks
	unsigned long int beginSim_calc = 50;
	unsigned long int Move_Tick_calc = 50;
	unsigned long int House_calc=1;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(beginSim_calc, Move_Tick_calc);
	tmpGCD = findGCD(tmpGCD, House_calc);
	//tmpGCD = findGCD(tmpGCD, Alarm_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int beginSim_period = beginSim_calc/GCD;
	unsigned long int Move_Tick_period = Move_Tick_calc/GCD;
	unsigned long int House_period = House_calc/GCD;
	//unsigned long int Alarm_period = Alarm_calc/GCD;
	//unsigned long int SMTick4_period = SMTick4_calc/GCD;

	//Declare an array of tasks 
	static task task1, task2, task3; // task3, task4;
	task *tasks[] = { &task1, &task2, &task3};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = beginSim_period;//Task Period.
	task1.elapsedTime = beginSim_period;//Task current elapsed time.
	task1.TickFct = &beginSim;//Function pointer for the tick.

	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = Move_Tick_period;//Task Period.
	task2.elapsedTime = Move_Tick_period;//Task current elapsed time.
	task2.TickFct = &Move_Tick;//Function pointer for the tick.
	
	//Task 3
	task3.state = -1;//Task initial state.
	task3.period = House_period;//Task Period.
	task3.elapsedTime = House_period;//Task current elapsed time.
	task3.TickFct = &House_Tick;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	initUSART(0);
	initUSART(1);
	unsigned short i; // Scheduler for-loop iterator
	while(1) {
    // Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			GetNESData();
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
