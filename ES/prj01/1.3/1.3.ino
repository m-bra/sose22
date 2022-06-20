// Digital I/O / Blinking LED
// Board connection pin of the LED
// ANODE (+) -> "longer leg" of the LED

uint8_t pin_led = 13;
uint8_t pin_input = 3;
bool led_state = LOW;	
bool last_input_state = HIGH;

void setup () {
  pinMode(pin_led, OUTPUT);
  pinMode(pin_input, INPUT_PULLUP);
  digitalWrite(pin_led, led_state);
}

void loop() {

  bool current_input_state = digitalRead(pin_input);
  if (current_input_state == LOW && last_input_state == HIGH)
  {
    led_state = !led_state;
  }
  last_input_state = current_input_state;
  
  digitalWrite(pin_led, led_state);
}
