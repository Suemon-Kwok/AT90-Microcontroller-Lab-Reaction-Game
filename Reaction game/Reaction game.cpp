/*
Lab Week 7 Reaction Game
Exercise
Design and implement an LED reaction game on the AT90 lab board. The game will randomly
illuminate an LED, and the user must input the corresponding number within a specified time. As the
user continues to input correctly, the reaction time will gradually shorten, increasing the game's
difficulty. Demonstrate the use of an interrupt in your solution.
Initialization Settings:
1. Configure USART1 for serial communication with a baud rate of 9600.
2. Set PORTC as output to control 8 LEDs.
3. Set PB6 as output to indicate the game status.
Game Process:
1. At the start of the game, illuminate the PB6 indicator light to signal the game is about to
begin; after 3 seconds, turn it off to officially start the game. The initial time limit is 2000ms.
2. Randomly illuminate an LED. The user can now enter an LED number (1-8) via the serial port.
The microcontroller program starts measuring the time it takes for the user to make the
input.
3. If the user entered the correct LED number, and the input has been made within the current
time limit, then the PB6 indicator light quickly flashes three times and the user proceeds to
the next round, with time limit reduced by 100ms, and the program continues from step 2.
4. Otherwise (timeout or incorrect input), indicate the end of the game by turning on the PB6
indicator light for 3 seconds. Write a text message with the correct LED number and the
user's reaction time to the USART. The game then restarts from step 1.
HINTS:
1. Putty terminal clear screen command:
char clear_cmd[] = "\033[2J\033[H";
2. Putty terminal backspace command backspace
UDR1 = '\b';
3. Generate a random number:
#include <stdlib.h>
int num = rand() % 100 + 1;
4. Convert ASCII to integer:
uint8_t input = atoi(uart_buffer);
5. Example code of writing a game-over message on the USART:
char msg[80];
sprintf(msg, "\r\nError! Game Over. LED %d was active.\r\nYour reaction time
is %d ms", __builtin_ctz(led) + 1, delay_time);
for (uint8_t i = 0; msg[i]; i++)
{
while (!(UCSR1A & (1 << UDRE1)));
UDR1 = msg[i];
}
*

/

/*

*	GccApplication2.c
*
 *

*	Created: 1/05/2025 12:06:09 pm
*
*	Author :
*
 */

 /*

 *Includes necessary AVR libraries for microcontroller functionality.

 *Defines clock speed (F_CPU = 8 MHz) for delay calculations.

 */

#include <avr/io.h>

#include <avr/interrupt.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#define F_CPU 8000000UL   // 8MHz clock frequency

#include <util/delay.h>

#include <stdbool.h>

 // Constants

 /*

 *9600 baud for USART communication.

 *Initial reaction time limit = 2000ms (decreases after each round).

 Minimum reaction time limit = 500ms (to make the game harder)

 */

#define BAUD 9600

#define UBRR_VALUE ((F_CPU / 16 / BAUD) - 1)

 // Game parameters

#define INITIAL_TIME_LIMIT 2000  // Initial time limit in ms

#define TIME_REDUCTION 100       // Time reduction per round in ms

#define MIN_TIME_LIMIT 500       // Minimum time limit in ms

#define MAX_BUFFER_SIZE 10       // Maximum buffer size for UART input

// Global variables

/*

active_led: Stores the currently active LED.

time_limit: Current round's time limit (reduces after each successful round).

reaction_time: Tracks player response time. game_running: True when a round is active, False when waiting for input. timeout_occurred: Set when the player exceeds the reaction time limit. input_received: Set when a valid user input is received via USART. uart_buffer: Stores user input (up to MAX_BUFFER_SIZE).

buffer_index: Tracks the buffer position.

*/

volatile uint8_t active_led = 0;         // Currently active LED volatile uint16_t time_limit = INITIAL_TIME_LIMIT;  // Current time limit volatile uint16_t reaction_time = 0;     // User's reaction time volatile bool game_running = false;      // Game state volatile bool timeout_occurred = false;  // Timeout flag volatile bool input_received = false;    // Input received flag volatile char uart_buffer[MAX_BUFFER_SIZE];  // Buffer for UART input volatile uint8_t buffer_index = 0;       // Current index in buffer

// Timer1 compare match interrupt handler

/*

*Triggers when the countdown expires.

*Stops the game and sets timeout_occurred = true.

*Calculates reaction time using TCNT1 / 125 (milliseconds).

*/

ISR(TIMER1_COMPA_vect) {

    if (game_running) {

        reaction_time = (TCNT1 / 125);  // capture actual elapsed time in ms         timeout_occurred = true;         game_running = false;

    }
}

// USART1 Receive Complete interrupt handler

/*

*Receives characters via USART.

*Handles backspace (\b) correctly.

*Stores input in uart_buffer until ENTER (\r or \n) is pressed.

Echoes the character back to PuTTY terminal.

*/

ISR(USART1_RX_vect) {
    char received_char = UDR1;     // Echo the character back to the terminal     while (!(UCSR1A & (1 << UDRE1)));     UDR1 = received_char;

    if (received_char == '\r' || received_char == '\n') {         // End of input: null-terminate and signal         uart_buffer[buffer_index] = '\0';         input_received = true;

        // buffer_index will be reset in main()

    }
    else if (received_char == '\b' && buffer_index > 0) {

        // Handle backspace         buffer_index--;         // Clear character on terminal         while (!(UCSR1A & (1 << UDRE1)));         UDR1 = ' ';         while (!(UCSR1A & (1 << UDRE1)));

        UDR1 = '\b';

    }
    else if (buffer_index < MAX_BUFFER_SIZE - 1) {         // Store the character in the buffer         uart_buffer[buffer_index++] = received_char;

    }
}

// Initialize USART1

/*

*Configures USART1 for 9600 baud.

*Enables the receiver (RXEN1) and transmitter (TXEN1).

*Enables Receive Complete Interrupt (RXCIE1).

*/ void usart_init(void) {

    UBRR1H = (uint8_t)(UBRR_VALUE >> 8);

    UBRR1L = (uint8_t)UBRR_VALUE;

    UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);

    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);

}

// Initialize Timer1 for timeout detection

/*

*Configures Timer1 for countdown timing.

*Uses CTC (Clear Timer on Compare Match) mode.

*Triggers ISR(TIMER1_COMPA_vect) when timeout occurs

*/ void timer1_init(void) {
    TCCR1A = 0;

    TCCR1B = 0;

    TCNT1 = 0;

    // CTC mode, prescaler = 256 (for up to ~2.1s)

    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);

    TIMSK1 |= (1 << OCIE1A);

}

// Set Timer1 timeout in milliseconds

/*

*Randomly selects an LED (0-7) using rand() % 8.

*Activates that LED on PORTC.

*/

void set_timer1_timeout(uint16_t ms) {

    // Stop timer

    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));     // Calculate compare value, capped to 16 bits     uint32_t ticks = (uint32_t)ms * (F_CPU / 1000UL) / 256UL;     if (ticks > 0xFFFF) ticks = 0xFFFF;

    OCR1A = (uint16_t)ticks;

    TCNT1 = 0;

    // Clear any pending interrupt

    TIFR1 = (1 << OCF1A);

    // Restart timer with prescaler = 256

    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);

}

// Send a string over USART1 void usart_send_string(const char *str) {     for (uint8_t i = 0; str[i]; i++) {

while (!(UCSR1A & (1 << UDRE1)));

UDR1 = str[i];

    } }

    // Clear terminal screen void clear_screen(void) {     usart_send_string("\033[2J\033[H");

}

// Illuminate a random LED on PORTC void illuminate_random_led(void) {     uint8_t idx = rand() % 8;     active_led = (1 << idx);

PORTC = active_led;

}

// Get current LED index 0-7 uint8_t get_active_led_number(void) {     for (uint8_t i = 0; i < 8; i++) {         if (active_led & (1 << i)) return i;

    }

    return 0;

}

// Flash PB6 indicator three times void flash_indicator(void) {

cli();

for (uint8_t i = 0; i < 3; i++) {

    PORTB |= (1 << PB6);

    _delay_ms(100);

    PORTB &= ~(1 << PB6);

    _delay_ms(100);

}     sei(); }

// Display game over sequence

/*

*Indicates game over by turning on PB6.

*Prints game results to PuTTY (correct LED + reaction time)

*/

void game_over(uint16_t rt) {
    PORTB |= (1 << PB6);     uint8_t ledn = get_active_led_number();     char msg[80];

    sprintf(msg, "\r\nGame Over! LED %d. Reaction time: %d ms.\r\n", ledn, rt);     usart_send_string(msg);     _delay_ms(3000);

    PORTB &= ~(1 << PB6);

}

// Reset flags and clear UART buffer void reset_game_state(void) {     timeout_occurred = false;     input_received = false;     game_running = false;     buffer_index = 0;     PORTC = 0;

memset((char*)uart_buffer, 0, MAX_BUFFER_SIZE);

}

/*

*Starts the game with a 3-second countdown.

*Randomly illuminates an LED.

*Waits for user input or timeout.

*Reduces reaction time limit after correct input.

*Game over if incorrect or too slow.

*/ int main(void) {
    srand(0);

    // Port setup

    DDRC = 0xFF;

    DDRB |= (1 << PB6);

    PORTC = 0;     PORTB &= ~(1 << PB6);

    usart_init();     timer1_init();     sei();

    clear_screen();

    usart_send_string("Welcome to the LED Reaction Game!\r\n");

    while (1) {

        time_limit = INITIAL_TIME_LIMIT;         reset_game_state();

        usart_send_string("\r\nGame starts in 3 seconds...\r\n");

        PORTB |= (1 << PB6);

        _delay_ms(3000);

        PORTB &= ~(1 << PB6);

        bool keep_playing = true;         uint8_t round = 0;

        while (keep_playing) {

            reset_game_state();             illuminate_random_led();

            uint8_t correct = get_active_led_number();

            char prompt[50];

            sprintf(prompt, "\r\nRound %d - Limit=%d ms. LED (0-7): ", ++round, time_limit);             usart_send_string(prompt);

            set_timer1_timeout(time_limit);             game_running = true;

            while (!input_received && !timeout_occurred) {}             game_running = false;

            if (timeout_occurred) {
                game_over(reaction_time);                 keep_playing = false;

            }
            else {

                uart_buffer[buffer_index] = '\0';                 uint8_t user = atoi((char*)uart_buffer);                 buffer_index = 0;                 reaction_time = (TCNT1 / 125);

                if (user == correct && reaction_time <= time_limit) {
                    char ok[80];

                    sprintf(ok, "\r\nCorrect! LED %d. RT=%d ms\r\n", correct, reaction_time);                     usart_send_string(ok);                     flash_indicator();

                    if (time_limit > MIN_TIME_LIMIT + TIME_REDUCTION)                         time_limit -= TIME_REDUCTION;                     else

                        time_limit = MIN_TIME_LIMIT;

                }
                else {

                    if (user != correct) {
                        char w[80];

                        sprintf(w, "\r\nIncorrect! You entered %d, was %d\r\n", user, correct);                         usart_send_string(w);

                    }
                    else {
                        char s[80];                         sprintf(s, "\r\nToo slow! RT=%d ms > %d ms\r\n", reaction_time, time_limit);                         usart_send_string(s);

                    }

                    game_over(reaction_time);                     keep_playing = false;

                }

            }

            _delay_ms(2000);

        }

    }

    return 0;

}



