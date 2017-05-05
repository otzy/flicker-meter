/*
  Flicker Meter
  Arduino Nano

  This device checks the quality of light sources, especcially LED lamps.
  These lamps are often flickering, usually on the 100-120Hz frequency (doubled frequency of power grid).

  Flickering is displayed with the row of 10 leds (Kingbright DC-10EWA). The number of blinking LEDs is proportional to the flicker rate.

  Light sensor used: Avago APDS-9006-020
  It's possible and even better to use APDS-9007, because of higher range. It needs some testing.
  Leds connected to digital pins from 2 to 11
  Sensor connected to pin A6

  Button connected to pin 12.

  Ask me if you need user manual
  
  (C) 2017 Evgeny Mazovetskiy
*/

//During waiting state we constantly measure average light and show it on display
#define STATE_WAITING 1

//Measuring state is when we inside light flickering measurement process. It's not used actually anywhere outside
#define STATE_MEASURING 2

//In this state we show flicker percentage. The number of blinking LEDs is proportional to the flicker rate.
#define STATE_DISPLAY 3

#define BUTTON1 12
#define SENSOR_PIN 6
#define SERIAL_SWITCH_PIN 13

#define LOOP_DELAY 10
#define SAMPLE_COUNT 256

int leds[] = {2,3,4,5,6,7,8,9,10,11};

int state = STATE_WAITING;
int button_last_state = HIGH;
int flickering = 0; // 0 - 100
int leds_to_blink = 0; //number of blinking LEDs 
int x[SAMPLE_COUNT];
int serial_on = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i<10; i++){
    pinMode(leds[i], OUTPUT);
  }

  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(SERIAL_SWITCH_PIN, INPUT_PULLUP);
  analogReference(EXTERNAL);

  if (digitalRead(SERIAL_SWITCH_PIN) == 0){
    Serial.begin(9600);
    serial_on = 1;
  }

  ledTest();
}

// the loop function runs over and over again forever
void loop() {
  int light;
  static int counter = 0;
  processButton();
    
  switch (state){
    case STATE_WAITING:
      light = measureAverageLight();
      if (serial_on && ((counter % 20) == 0)){
        Serial.print("light sensor = ");
        Serial.println(light);
        counter = 0;
      }
      showLight(light);
      counter++;
      break;
    case STATE_DISPLAY:
      showFlickering();
      delay(LOOP_DELAY);
      break;
    default:
      delay(LOOP_DELAY);
  }
  
}

void ledTest(){
  for (int i=0; i<10; i++) {
    digitalWrite(leds[i], HIGH);
    delay(70);
    digitalWrite(leds[i], LOW);
  }

  ledsOn();
  delay(150);
  ledsOff();
}

void ledsOff(){
  for (int i=0; i<10; i++) {
    digitalWrite(leds[i], LOW);
  }
}

void ledsOn(){
  for (int i=0; i<10; i++) {
    digitalWrite(leds[i], HIGH);
  }
}

// This version requires additional button press to switch from flickering display to waiting mode
//
//void processButton(){
//  int btn_state = digitalRead(BUTTON1);
//  
//  if ((btn_state == LOW) && (button_last_state == HIGH)){
//    switch (state){
//      case STATE_WAITING:
//        flickering = measureFlickering();
//        calcLEDsToBlink(flickering);
//        state = STATE_DISPLAY;
//        break;
//      case STATE_DISPLAY:
//        state = STATE_WAITING;
//        break;
//    }
//  }
//
//  
//
//  button_last_state = btn_state;
//}

/**
 * this version displays flickering while button is pressed. When bottun released, the state switches to waiting mode
 */
void processButton(){
  int btn_state = digitalRead(BUTTON1);

  if ((btn_state == LOW) && (button_last_state == HIGH)){
    flickering = measureFlickering();
    calcLEDsToBlink(flickering);
    state = STATE_DISPLAY;
  } else if ((btn_state == LOW) && (state == STATE_DISPLAY)) {
    //do nothing, continue doing STATE_DISPLAY
  } else if (btn_state == HIGH && (state != STATE_WAITING)){
    //button released, we should be in waiting mode
    state = STATE_WAITING;
  }

  button_last_state = btn_state;
}

void calcLEDsToBlink(int flickering){
  leds_to_blink = 0;
  if (flickering <= 5){
    return;
  }
  
  leds_to_blink = ((flickering-1) - (flickering-1)%10) / 10 + 1;
  return;  
}

// /*
// *
// * Version for Avago APDS-9007 sensor with 27k load resistor
// * It did not work actually, because 27k is too much for ATMEGA328 ADC. To avoid noise, it should be less that 10k. I did not test APDS-9007 with lower loads yet
// *
// * return flickering percentage 0 - 100%
// */
//int measureFlickering(){
//  float x_min = 1024,
//        x_max = 0;
//
//  ledsOff();
//  noInterrupts();
//  for (int i = 0; i < SAMPLE_COUNT; i++){
//    x[i] = analogRead(SENSOR_PIN);
//    x_min = min(x_min, x[i]);
//    x_max = max(x_max, x[i]);
//    delay(1);
//  }
//  interrupts();
//
//  serialPrintX();
//
//  //exp(0.006674178041 * (adc / 1.06667)); this is a magic formula of Lux for 12bit ADC with 3.3V max. It's not 100% accurate, because I don't have industrial Luxmeter, but it doesn't metter for flickering
//
//  //convert to the magic-formula scale
//  x_min = 6.06 * x_min;
//  x_max = 6.06 * x_max;
//
//  //convert to Lux
//  x_min = pow(2.7182818285, 0.006674178041127581 * (x_min / 1.06667));
//  x_max = pow(2.7182818285, 0.006674178041127581 * (x_max / 1.06667));
//  
//  return int((x_max - x_min) / x_max *100);
//}


/*
 * return flickering percentage 0 - 100%
 */

// 
// Version for Avago APDS-9006-020 sensor with 1.8k load resistor
// 
// return flickering percentage 0 - 100%
// 
int measureFlickering(){
  float x_min = 1024,
        x_max = 0;

  ledsOff();
  noInterrupts();
  for (int i = 0; i < SAMPLE_COUNT; i++){
    x[i] = analogRead(SENSOR_PIN);
    x_min = min(x_min, x[i]);
    x_max = max(x_max, x[i]);
    delay(1);
  }
  interrupts();

  if (serial_on){
    serialPrintX();
  }

  return int((x_max - x_min) / x_max *100);
}

/**
 * APDS-9006-020 is slow. It can not really measure > than 150Hz.
 */
int measureAverageLight(){
  int result = 0;
  for (int i = 1; i <= 10; i++){
    result += analogRead(SENSOR_PIN);
    delay(5);
  }
  return result / 10;
}

void serialPrintX(){
  for (int i = 0; i < SAMPLE_COUNT; i++){
    Serial.println(x[i]);
  }
}

void showLight(int light){
  //Number of leds to turn on
  light = (light - light % 70) / 70;

  for (int i = 0; i < 10; i++){
    digitalWrite(leds[9-i], (i < light) );
  }
  
}

void showFlickering(){
  static int counter = 0;
  static int blink_state = 1;

  counter++;
  if (counter > (300 / LOOP_DELAY)){
    counter = 0;
    blink_state = blink_state ^ 1;
  }

  for (int i = 1; i <= 10; i++){
    if (i <= leds_to_blink){
      digitalWrite(leds[i-1], blink_state); 
    } else {
      digitalWrite(leds[i-1], HIGH);
    }    
  }
}


