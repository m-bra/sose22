// Digital I/O / Blinking LED
// Board connection pin of the LED
// ANODE (+) -> "longer leg" of the LED

#if defined(__SAM3X8E__)
  #include <DueTimer.h>
  DueTimer Timer3;
#else
  #include <TimerThree.h>
#endif

struct {
   int R = 6, G = 7, B = 8;
} pin_led;
uint8_t pin_input = 3;
uint8_t other_pin_input = 2;

bool last_input_state = HIGH;
bool user_input_state = HIGH;
bool other_last_input_state = HIGH;
bool other_user_input_state = HIGH;

bool suppress_next_keyup = false;
int measurement_i = -1;
int other_measurement_i = -1;

void on_both_keyup();
void on_keyup(int pin);

uint8_t pinPWMA = 9;
uint8_t pinAIN1 = 47;
uint8_t pinAIN2 = 48;

void on_timer() {
    int now = digitalRead(pin_input);
    int other_now = digitalRead(other_pin_input);

    if (now != last_input_state)
    measurement_i = -1;
    
    if (other_now != other_last_input_state)
    other_measurement_i = -1;

    last_input_state = now;
    other_last_input_state = other_now;

    ++measurement_i; ++other_measurement_i;
    bool stable = measurement_i >= 16, other_stable = measurement_i >= 16;

    bool keyup = stable && user_input_state == LOW && now == HIGH;
    bool other_keyup = other_stable && other_user_input_state == LOW && other_now == HIGH;

    if (!suppress_next_keyup) {
    // we assume that the user either (1) presses one key and after that releases the same key or 
    //                                (2) presses one key, then the other, and after that releases both keys in arbitrary order, without pressing any key.
    if ((keyup || other_keyup) && user_input_state == LOW && other_user_input_state == LOW) {
    on_both_keyup();
    suppress_next_keyup = true;
    }
    else if (keyup) {
    on_keyup(pin_input);
    } 
    else if (other_keyup) {
        on_keyup(other_pin_input);
      }
    }
    else if (keyup || other_keyup) {
      suppress_next_keyup = false;
    }

  if (stable)             user_input_state =       now;
    if (other_stable) other_user_input_state = other_now;

    if (stable && other_stable) Timer3.stop();
}

void on_input() {
   Timer3.start();
   measurement_i = other_measurement_i = 0;
}

void setup () {
  Serial.begin(9600);

  pinMode(pin_led.R, OUTPUT);
  pinMode(pin_led.G, OUTPUT);
  pinMode(pin_led.B, OUTPUT);

  pinMode(pin_input, INPUT_PULLUP);
  pinMode(other_pin_input, INPUT_PULLUP);
  
  pinMode(pinPWMA, OUTPUT);
  pinMode(pinAIN1, OUTPUT);
  pinMode(pinAIN2, OUTPUT);

 #if defined(__SAM3X8E__)
   Timer3.configure(1E3, on_timer);
 #else
   Timer3.initialize(1E3);
   //attatch ISR
   Timer3.attachInterrupt(on_timer); // attatch ISR & start timer
   Timer3.stop();
 #endif


  attachInterrupt(digitalPinToInterrupt(pin_input), on_input, CHANGE);
  attachInterrupt(digitalPinToInterrupt(other_pin_input), on_input, CHANGE);

  void write_led(); write_led();
}


void loop() {  

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <math.h>

/*extern struct {
   int R, G, B;
} pin_led;
extern int pin_input;
extern int other_pin_input;*/

enum {
    ROTATION_MODE,   POWER_MODE, MAX_MODES
}; char const *mode_str[] = {
   "ROTATION_MODE", "POWER_MODE"
};
int input_mode = ROTATION_MODE;

enum {
    CCW,   STOP,   CW
}; char const *rotation_str[] = {
   "CCW", "STOP", "CW"
};
int rotation = STOP;

float power = 0;
float power_step = 10;
float power_max = 255;
float power_min = 0;

void on_both_keyup()
{
  input_mode = (input_mode + 1) % MAX_MODES;
    void write_led(); write_led();
}

void on_keyup(int target_pin)
{
  int prevrotation = rotation;

  if (input_mode == ROTATION_MODE)
    {
        if (target_pin == pin_input)
        {
      switch (rotation)
            {
            case CCW:  rotation = STOP; break;
            case STOP: rotation = CW;   break;
            case CW:   rotation = STOP; break;
            }
        }
        else if (target_pin == other_pin_input)
        {
      switch (rotation)
            {
            case CCW:  rotation = STOP; break;
            case STOP: rotation = CCW;   break;
            case CW:   rotation = STOP; break;
            }
        }
    }
    else if (input_mode == POWER_MODE && rotation != STOP)
    {
        if (target_pin == pin_input)
        {
      power = fminf(power + power_step, power_max);
        }
        else if (target_pin == other_pin_input)
        {
      power = fmaxf(power - power_step, power_min);
        }
    }

  if (input_mode == POWER_MODE && rotation == STOP)
    Serial.println("Warning: Must set rotation to CW or CCW before changing power!");

    void write_led(); 
         write_led();
  void write_motor(int prevrotation); 
         write_motor(    prevrotation);
}

void write_led()
{
  int R = input_mode == POWER_MODE; 
  int G = input_mode == ROTATION_MODE;
  int B = rotation == STOP;
    
  analogWrite(pin_led.R, !R * 255); 
  analogWrite(pin_led.G, !G * 255);
  analogWrite(pin_led.B, !B * 255);

  Serial.print("[");
  Serial.print(mode_str[input_mode]); 
  Serial.print("] ");
  Serial.print(rotation_str[rotation]);
  Serial.print("; power = ");
  Serial.println(power);
}

void write_motor(int unused)
{
  analogWrite(pinPWMA, power);
  if (rotation == STOP)
  {
    // in order to properly halt the motor,
    // the user must first set power to zero.
    digitalWrite(pinAIN1, LOW);
    digitalWrite(pinAIN2, LOW);
  }
  if (rotation == CW)
  {
    digitalWrite(pinAIN1, HIGH);
    digitalWrite(pinAIN2, LOW);
  }
  if (rotation == CCW)
  {
    digitalWrite(pinAIN1, LOW);
    digitalWrite(pinAIN2, HIGH);
  }
}
