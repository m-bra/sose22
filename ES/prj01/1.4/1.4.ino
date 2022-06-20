// Digital I/O / Blinking LED
// Board connection pin of the LED
// ANODE (+) -> "longer leg" of the LED

#include <TimerThree.h>

uint8_t pin_led = 13;
uint8_t pin_input = 3;
volatile bool led_state = LOW;	
bool last_input_state = HIGH;

void on_input()
{
  led_state = !led_state;
}

void setup () {
  pinMode(pin_led, OUTPUT);
  pinMode(pin_input, INPUT_PULLUP);
  digitalWrite(pin_led, led_state);

  attachInterrupt(digitalPinToInterrupt(pin_input), on_input, FALLING);
}

void loop() {  
  digitalWrite(pin_led, led_state);
}
