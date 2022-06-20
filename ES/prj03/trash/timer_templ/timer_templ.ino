#include <TimerThree.h>

static const uint8_t led_pin = 13; // the pin with a LED
volatile bool led_state = LOW;

void changeLedState(void){
  led_state = !led_state; //change of led state
}

void setup(void)
{
  //configuration of digital I/O pin #13
  pinMode(led_pin , OUTPUT);
  //configuration of timer3 (@ 1Hz, 1E6us)
  Timer3.initialize(1E6);
  //attatch ISR
  Timer3.attachInterrupt(changeLedState); // attatch ISR & start timer
  //Timer3.stop(); Timer3.start()
}

void loop(void)
{
  digitalWrite(led_pin , led_state);
  delay(10);
}
