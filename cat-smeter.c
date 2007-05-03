
#define F_CPU 4000000

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdlib.h>

void displayTest(void);

int main(void) {
    // Divide clock by 256
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS3);

    // Enable pull-ups
    PORTB = 0xff;
    PORTC = 0xff;
    PORTD = 0xff;

    // PD6 is output
    DDRD = _BV(DDD6);

    // Timer 0 - PWM
    TCCR0A |= _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
    TCCR0B |= _BV(CS00);

    // Timer 2 - Main control loop
    TCCR2A |= _BV(WGM21);
    TCCR2B |= _BV(CS21);
    OCR2A = 0x7f;
    TIMSK2 |= _BV(OCIE2A);

    // Enable as much power saving as possible
    PRR |= _BV(PRTWI) | _BV(PRTIM1) | _BV(PRSPI) | _BV(PRUSART0) | _BV(PRADC);

    displayTest();

    // Go!
    wdt_reset();
    wdt_enable(WDTO_500MS);

    set_sleep_mode(SLEEP_MODE_IDLE);

    sei();

    for (;;) {
	sleep_mode();
    }

    return 0;
}

void displayTest() {
    OCR0A = 225;
    _delay_ms(5);
    OCR0A = 56;
    _delay_ms(5);
    OCR0A = 169;
    _delay_ms(5);
    OCR0A = 0;
    _delay_ms(5);
}

void uartTx(uint8_t a[], uint8_t len) {
    uint8_t i;

    // TX contents of array
    for (i = 0; i < len; i++) {
	while (!(UCSR0A & _BV(UDRE0))) {
	}

	UDR0 = a[i];
    }

    // Spin until all data is sent
    while (!(UCSR0A & _BV(UDRE0))) {
    }
}

SIGNAL(SIG_OUTPUT_COMPARE2A) {
    // Step down timer2 prescaler
    TCCR2B |= _BV(CS22) | _BV(CS21) | _BV(CS20);
    // Divide clock by 2
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS0);

    // Enable UART0
    PRR &=  ~(_BV(PRUSART0));

    // Enable TX & RX
    UCSR0B |= _BV(RXEN0) | _BV(TXEN0);

    // 8-bit, 2 stop-bits, no parity, 38.400kbps
    UCSR0C |= _BV(USBS0) | _BV(UCSZ01) | _BV(UCSZ00);
    UCSR0A |= _BV(U2X0);
    UBRR0L |= 12;

    // Empty RX FIFO
    while (UCSR0A & _BV(RXC0)) {
	UDR0;
    }

    // TX for signal level
    static uint8_t txData[] = {0x00, 0x00, 0x00, 0x00, 0xe7};

    uartTx(txData, 5);

    // Let the radio respond
    _delay_ms(100);

    // RX data and set PWM duty-cycle according to signal level
    if (UCSR0A & _BV(RXC0)) {
	static uint8_t values[] = {
	    0, 15, 30, 45, 
	    60, 75, 90, 105, 
	    120, 135, 150, 165,
	    180, 195, 210, 225
	};

	uint8_t c = UDR0 & 0x0f;

	OCR0A = values[c];
    } else {
	OCR0A = 0;
    }

    // Disable UART0
    PRR |=  _BV(PRUSART0);
    // Divide clock by 256
    CLKPR = _BV(CLKPCE);
    CLKPR = _BV(CLKPS3);
    // Step up timer2 prescaler
    TCCR2B &= ~(_BV(CS20) | _BV(CS22));

    wdt_reset();
}
