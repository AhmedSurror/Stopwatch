/* Stopwatch Project
 * Author: Ahmed Surror
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrUpt.h>

unsigned char Hours = 0 ,Minutes = 0 ,Seconds = 0 ,Running = 1 ,Up = 1;

ISR(TIMER1_COMPA_vect){
	/* If not paused it will increment as default
	 Seconds and Minutes increment until 59 then reset , Hours increment until 23 the reset */

	if(Running)
	{
		if(Up)
		{
			if(Seconds>58)
			{
				Seconds = 0;

				if(Minutes >58)
				{
					Minutes =0;

					if(Hours > 23)
					{
						Hours = 0;
					}
					else
					{
						Hours++;
					}
				}
				else
				{
					Minutes++;
				}
			}
			else
			{
				Seconds++;
			}
		}

		/* TOGGLE Button is pressed it will decrement */

		else if(!Up)
		{
			if(!(Seconds == 0 && Hours == 0 && Minutes == 0))
			{
				if(Seconds == 0)
				{
					Seconds = 59;

					if(Minutes == 0)
					{
						Minutes =59;

						if(Hours == 0)
						{
							Hours = 23;
						}
						else
						{
							Hours--;
						}
					}
					else
					{
						Minutes--;
					}
				}
				else
				{
					Seconds--;
				}
			}
			else
			{
				PORTA = 0xFF;
				PORTD |= (1<<PD0);		/* If all LEDs are 0 after decrementing then BUZZER and LED will be ON */
				_delay_ms(2500);
				PORTD &= ~(1<<PD0);
				PORTA = 0xC0;
			}
		}
	}
}

ISR(INT0_vect){
	/* Reset all LEDs and start again*/

	Hours   = 0;
	Minutes = 0;
	Seconds = 0;
	_delay_ms(250);
}

ISR(INT1_vect){
	Running = 0;		/* To PAUSE the watch */
}

ISR(INT2_vect){
	Running = 1;		/* To RESUME the watch */
}

void INT2_resume(void){
	MCUCSR &= ~(1<<ISC2);		/* Trigger INT2 with the falling edge */

	GICR |= (1<<INT2);			/* Enable external interrupt pin */
}

void INT1_pause(void)
{
	MCUCR |= (1<<ISC11) | (1<<ISC10);			/* Trigger INT1 with the raising edge */

	GICR |= (1<<INT1);							/* Enable external interrupt pin */
}

void INT0_reset(void)
{
	MCUCR |= (1<<ISC01);		/* Trigger INT0 with the falling edge */
	MCUCR &= ~(1<<ISC00);

	GICR |= (1<<INT0);			/* Enable external interrupt pin */
}

/* System Frequency = 16 MHz and Prescaler = F_CPU/1024
 * Timer Frequency = 15.625 KHz , Timer time = 64 us
 * We need 15625 ticks for 1 sec
 * */

void TIMER1_CTC(void){
	TCNT1 = 0;				/* Set Timer1 initially to 0 */

	OCR1A = 15624;			/* Set compare value to 15624 */

	TIMSK |= (1<<OCIE1A);	/* Enable Timer1 compare A interrupt */

	/* Configure timer control register TCCR1B
	 * 1. CTC Mode WGM12=1 WGM13=0 (Mode Number 4)
	 * 2. Prescaler = F_CPU/1024 CS10=1 CS11=0 CS12=1
	 */

	TCCR1B |= (1<<CS12) | (1<<CS10) | (1<<WGM12);
}

int main(void)
{
	unsigned char HI = 0 ,HD = 0 ,MI = 0 ,MD = 0, SI = 0 ,SD = 0;		/* Setting Hours,Minutes and Seconds I: Increment and D: Decrement flags */

	DDRC |= 0x0F; 			/* 7 SEGMENT Decoder */
	PORTC &= 0xF0; 			/* LEDs OFF */

	DDRB = 0x00;			/* Buttons as INPUTs */
	PORTB = 0xFF;			/* To allow INTERNAL PULL UP RES */

	DDRD &= ~((1<<PD2) | (1<<PD3) );		/* Pause and Reset as INPUTs */
	DDRD |= (1<<PD0) | (1<<PD4) |(1<<PD5);	/* Buzzer , Count UP and DOWN LEDs as OUTPUTs */
	PORTD &= ~(1<<PD0);						/* Buzzer OFF */

	DDRA |= 0x3F;			/* 7 SEGMENT Enable */

	PORTD &= ~(1<<PD5);		/* Counting DOWN LED YELLOW */
	PORTD |= (1<<PD4); 		/* Counting UP LED RED */
	PORTA = 0xC0;			/* Turning LEDs ON */
	SREG |= (1<<7);			/* Enable Interrupts */

	INT0_reset();
	INT2_resume();
	INT1_pause();
	TIMER1_CTC();

	for(;;)
	{
		/*										ENABLING Seconds' LEDs										*/

		PORTC = (PORTC & 0xF0) | ((Seconds%10) & 0x0F);		/* Enabling Ones of Seconds */
		PORTA |= (1<<PA5);
		_delay_ms(1);
		PORTA &= ~(1<<PA5);

		PORTC = (PORTC & 0xF0) | ((Seconds/10) & 0x0F);		/* Enabling Tens of Seconds	*/
		PORTA |= (1<<PA4);
		_delay_ms(1);
		PORTA &= ~(1<<PA4);

		/*										ENABLING Minutes' LEDs										*/

		PORTC = (PORTC & 0xF0) | ((Minutes%10) & 0x0F);		/* Enabling Ones of Minutes */
		PORTA |= (1<<PA3);
		_delay_ms(1);
		PORTA &= ~(1<<PA3);

		PORTC = (PORTC & 0xF0) | ((Minutes/10) & 0x0F);		/* Enabling Tens of Minutes	*/
		PORTA |= (1<<PA2);
		_delay_ms(1);
		PORTA &= ~(1<<PA2);

		/*										ENABLING Hours' LEDs										*/

		PORTC = (PORTC & 0xF0) | ((Hours%10) & 0x0F);		/* Enabling Ones of Hours */
		PORTA |= (1<<PA1);
		_delay_ms(1);
		PORTA &= ~(1<<PA1);

		PORTC = (PORTC & 0xF0) | ((Hours/10) & 0x0F);		/* Enabling Tens of Hours */
		PORTA |= (1<<PA0);
		_delay_ms(1);
		PORTA &= ~(1<<PA0);

		/*										TOGGLING Count UP \ DOWN										*/

		if(!(PINB & (1<<PB7)) && Running == 0)
		{
			_delay_ms(30);								/* Avoid De-Bouncing of button */

			if(!(PINB & (1<<PB7)) && Running == 0)		/* IF TOGGLE button is pressed and watch is PAUSED it will count DOWN */
			{
				Up ^= 1;
				PORTD ^= (1<<PD5);						/* Toggle count DOWN LED */
				PORTD ^= (1<<PD4);						/* Toggle count UP LED  */
			}
			while(!(PINB & (1<<PB7)));
		}

		/*										Hours INCREMENT										*/

		if(!(PINB & (1<<PB1)) && Running == 0){
			_delay_ms(30);								/* Avoid De-Bouncing */

			if(!(PINB & (1<<PB1)) && Running == 0)		/* If Hours Increment button is pushed and watch is paused */
			{
				if(HI == 0){
					if(Hours > 22)
					{
						Hours = 0;
					}
					else
					{
						Hours++;
					}
					HI = 1;								/* Just to stop incrementing until HI resets */
				}
			}

		}
		else
		{
			HI = 0;
		}

		/*										Hours DECREMENT										*/

		if(!(PINB & (1<<PB0)) && Running == 0){
			_delay_ms(30);								/* Avoid De-Bouncing */

			if(!(PINB & (1<<PB0)) && Running == 0)		/* If Hours Decrement button is pushed and watch is paused */
			{
				if(HD == 0)
				{
					if(Hours == 0)
					{
						Hours = 23;
					}
					else
					{
						Hours--;
					}
					HD = 1;								/* Just to stop decrementing until HD resets */
				}
			}

		}
		else
		{
			HD = 0;
		}

		/*										Minutes INCREMENT										*/

		if(!(PINB & (1<<PB4)) && Running == 0){
			_delay_ms(30);								/* Avoid De-Bouncing */

			if(!(PINB & (1<<PB4)) && Running == 0)		/* If Minutes Increment button is pushed and watch is paused */
			{
				if(MI == 0){
					if(Minutes > 58)
					{
						Minutes = 0;
					}
					else
					{
						Minutes++;
					}
					MI = 1;								/* Just to stop incrementing until MI resets */
				}
			}

		}
		else
		{
			MI = 0;
		}

		/*										Minutes DECREMENT										*/

		if(!(PINB & (1<<PB3)) && Running == 0)
		{
			_delay_ms(30);								/* Avoid De-Bouncing */

			if(!(PINB & (1<<PB3)) && Running == 0)		/* If Minutes Decrement button is pushed and watch is paused */
			{
				if(MD == 0)
				{
					if(Minutes == 0)
					{
						Minutes = 59;
					}
					else
					{
						Minutes--;
					}
					MD = 1;								/* Just to stop decrementing until MD resets */
				}
			}

		}
		else
		{
			MD = 0;
		}

		/*										Seconds INCREMENT										*/

		if(!(PINB & (1<<PB6)) && Running == 0)
		{
			_delay_ms(30);								/* Avoid De-Bouncing */

			if(!(PINB & (1<<PB6)) && Running == 0)		/* If Seconds Increment button is pushed and watch is paused */
			{
				if(SI == 0){
					if(Seconds > 58)
					{
						Seconds = 0;
					}
					else
					{
						Seconds++;
					}
					SI = 1;								/* Just to stop incrementing until SI resets */
				}
			}

		}
		else
		{
			SI = 0;
		}

		/*										Seconds DECREMENT										*/

		if(!(PINB & (1<<PB5)) && Running == 0)
		{
			_delay_ms(30);								/* Avoid De-Bouncing */

			if(!(PINB & (1<<PB5)) && Running == 0)		/* If Seconds Decrement button is pushed and watch is paused */
			{
				if(SD == 0){
					if(Seconds == 0)
					{
						Seconds = 59;
					}
					else
					{
						Seconds--;
					}
					SD = 1;								/* Just to stop decrementing until SI resets */
				}
			}

		}
		else
		{
			SD = 0;
		}
	}
}
