// Digital I/O / Blinking LED
// Board connection pin of the LED
// ANODE (+) -> "longer leg" of the LED

#include <TimerThree.h>

uint8_t pin_led = 13;
uint8_t pin_input = 3;
volatile bool led_state = LOW;	
bool last_input_state = HIGH;

void on_timer() {
  int now = digitalRead(pin_input);
  if (last_input_state == HIGH && now == LOW) {
    led_state = !led_state;
  }
  last_input_state = now;
}

void setup () {
  pinMode(pin_led, OUTPUT);
  pinMode(pin_input, INPUT_PULLUP);
  digitalWrite(pin_led, led_state);
  //configuration of timer3 (@ 1000Hz, 1E3us)
  Timer3.initialize(1E3);
  //attatch ISR
  Timer3.attachInterrupt(on_timer); // attatch ISR & start timer
  Timer3.stop(); Timer3.start();
}

void loop() {  
  digitalWrite(pin_led, led_state);
}
