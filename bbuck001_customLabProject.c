/*    Partner(s) Name & E-mail: Braeton Buckley
 *    Lab Section: 023 TR 940-1100am
 *    Assignment: Custom Lab Project 
 *    Exercise Description: Game, 4 Build-upons: 8x8 LED Matrix, Accelerometer,
 *    Game Levels, Random Map Generation
 *
 *    I acknowledge all content contained herein, excluding template or example
 *    code, is my own original work.
 */

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>
#include <scheduler.h>

enum Char_States {char_move, char_wait} char_state;
enum Enemy_States {spawn, ene_move, ene_wait} enemy_state;
enum Score_States {check_point, level_change} score_state;

unsigned short adc_val; //for accelerometer
unsigned char charPos = 0xEF; //vertical (from player's perspective) position of character
unsigned char vertPos = 0x7F; //"enemy" vertical position
unsigned char horizPos = 0x80;//"enemy" horizontal position
unsigned int enemyWaitVal = 10;//lower value gives faster enemy
unsigned int score = 0;

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

void Char_Tick() {
	static unsigned int char_wait_count = 0;//"timer" to slow down character
	adc_val = ADC; //movement based on accelerometer
	unsigned char bot8Bits = (char)adc_val & 0xFF;
	switch(char_state) {
		case char_move:
			char_state = char_wait;
			break;
		case char_wait:
			if(char_wait_count >= 15) {
				char_state = char_move;
			} else {
				char_state = char_wait;
			}
			break;
		default:
			char_state = char_move;
			break;
	}
	
	//display character position
	switch(char_state) {
		case char_move:
			char_wait_count = 0;
			PORTC = 0x01;
			if(bot8Bits < 35) {
				charPos = 0x7F;
			} else if(bot8Bits < 55) {
				charPos = 0xBF;
			} else if(bot8Bits < 75) {
				charPos = 0xDF;
			} else if(bot8Bits < 95) {
				charPos = 0xEF;
			} else if(bot8Bits < 115) {
				charPos = 0xF7;
			} else if(bot8Bits < 135) {
				charPos = 0xFB;
			} else if(bot8Bits < 155) {
				charPos = 0xFD;
			} else {
				charPos = 0xFE;
			}
			PORTD = charPos;
			break;
		case char_wait:
			char_wait_count++;
			PORTD = charPos;
		default:
			PORTC = 0x01;
			break;
	}
}

void Enemy_Tick() {
	static unsigned int ene_wait_count = 0; //"timer" to slow down enemy
	switch(enemy_state) {
		case spawn:
			enemy_state = ene_wait;
			break;
		case ene_move:
			enemy_state = ene_wait;
			break;
		case ene_wait:
			if(ene_wait_count >= enemyWaitVal) {
				if(horizPos == 0x01) {
					enemy_state = spawn;
				} else {
					enemy_state = ene_move;
				}
			} else {
				enemy_state = ene_wait;
			}
			break;
		default:
			enemy_state = spawn;
			break;
	}
	switch(enemy_state) {
		//create new enemy
		case spawn:
			ene_wait_count = 0;
			vertPos = ~(0x01 << (rand() % 8)); //initial pseudorandom position, 
			//srand is seeded in main for real random map generation
			
			PORTD = vertPos;
			horizPos = 0x80;
			PORTC = horizPos;
			break;
			
		//move enemy
		case ene_move:
			ene_wait_count = 0;
			PORTD = vertPos;
			horizPos = horizPos >> 1;
			PORTC = horizPos;
			break;
			
		//wait enemy
		case ene_wait:
			ene_wait_count++;
			PORTD = vertPos;
			PORTC = horizPos;
			
		default:
			break;
	}
}

void Score_Tick() {
	//unsigned int mod4 = score % 4;
	unsigned char tmpB = ~PINB;
	switch(score_state) {
		case check_point:
			if(score >= 3) {
				if(horizPos == 0x01) {
					if(vertPos == charPos) {
						score_state = level_change;
						break;
					}
				}
			}
			score_state = check_point;
			break;
		case level_change:
			score_state = check_point;
			break;
		default:
			score_state = check_point;
			break;
	}
	switch(score_state) {
		case check_point:
			//change score when applicable
			if(horizPos == 0x01) {
				if(vertPos == charPos) {
					score++;
				} else {
					if(score != 0) {
						score--;	
					}
				}
			}
			
			//display score lights
			if(score == 0) {
				PORTB = (tmpB & 0xF0) | 0x00;
			} else if(score == 1) {
				PORTB = (tmpB & 0xF0) | 0x04;
			} else if(score == 2) {
				PORTB = (tmpB & 0xF0) | 0x06;
			} else {
				PORTB = (tmpB & 0xF0) | 0x07;
			}
			break;
			
		case level_change:
			score = 0; //reset score for next level
			enemyWaitVal = enemyWaitVal - 2; //enemy gets faster
			PORTC = 0xFF; //flash screen to show level is changing
			PORTD = 0x00;
			break;
		default:
			break;
	}
}

int main(void)
{
	DDRA = 0x00; PORTA = 0x00; //Accelerometer
	DDRB = 0x0F; PORTB = 0x00; //Score lights and srand button
	DDRC = 0xFF; PORTC = 0x00; //LED Matrix Rows
	DDRD = 0xFF; PORTD = 0x00; //LED Matrix Columns
	unsigned char button = ~PINB & 0x10;
	unsigned int seed = 0;
	unsigned int winScreen = 0;
	ADC_init();
	
	// Period for the tasks
	unsigned long int SMTick1_calc = 10;
	unsigned long int SMTick2_calc = 20;
	unsigned long int SMTick3_calc = 200;	
	
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
	tmpGCD = findGCD(tmpGCD, SMTick3_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;

	//Declare an array of tasks
	static task task1, task2, task3;
	task *tasks[] = { &task1, &task2, &task3 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &Char_Tick;//Function pointer for the tick.
	
	// Task 2
	task2.state = -1;//Task initial state.
	task2.period = SMTick2_period;//Task Period.
	task2.elapsedTime = SMTick2_period;//Task current elapsed time.
	task2.TickFct = &Enemy_Tick;//Function pointer for the tick.
	
	// Task 3
	task3.state = -1;//Task initial state.
	task3.period = SMTick3_period;//Task Period.
	task3.elapsedTime = SMTick3_period; // Task current elasped time.
	task3.TickFct = &Score_Tick; // Function pointer for the tick.
	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();

	unsigned short i; // Scheduler for-loop iterator
	
	while (1)
	{
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		
		//Display victory screen after game ends
		if(enemyWaitVal < 4 && winScreen < 20) {
			PORTC = 0xAA;
			PORTD = 0xAA;
			winScreen++;
		} else if(enemyWaitVal < 4 && winScreen < 40) {
			PORTC = 0x55;
			PORTD = 0x55;
			winScreen++;
		} else {
			winScreen = 0;
		}
		
		//Random seeding of rand based on when the user presses a button
		button = (~PINB & 0x10);
		if(button == 0x10) {
			srand(seed);
		}
		seed++;
		
		
		while (!TimerFlag) {};
		TimerFlag = 0;
	}
}
