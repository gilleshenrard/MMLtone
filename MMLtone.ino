#include "MMLtone.h"

/*
 * Devices used during tests :
 *  - 1 x Arduino Nano V3 / Atmega 328p-au
 *  - 1 x piezoelectric buzzer w/o oscillator (GPC3010YB-5V)
 *  - 1 x TC4428 IC
 *  - 1 x 9V battery (to power the Arduino)
 * 
 * Music library testing procedure
 * --------------------------------
 *      TC4428	      |   
 *      --------------|------------------
 *      IN_A    : 2   |   Arduino : D12, Atmega : 16 (PB4)
 *      GND     : 3   |   GND
 *      IN_B    : 4   |   Arduino : D12, Atmega : 16 (PB4)
 *      OUT_B   : 5   |   buzzer
 *      Vdd     : 6   |   +9V
 *      ~OUT_A  : 7   |   buzzer
 *      
 *      Buzzer pins   |
 *      --------------|------------------------------------
 *                    |   Each on the TC4428 outputs
 */

//timer1 values with 16 MHz crystal
// 1/64 notes are chosen as smallest musical increment
// (value = 16,000,000 / (prescaler * Hz) - 1) -> must be < 65536 for timer1
#define BPM120CRY 62499 // 32Hz tick (1/64 note at 120 BPM)
#define BPM140CRY 53570 // 37.333Hz tick (1/64 note at 140 BPM)
#define BPM160CRY 46873 // 42.667Hz tick (1/64 note at 160 BPM)

//timer1 values with 8 MHz internal resonator
// 1/64 notes are chosen as smallest musical increment
// (value = 8,000,000 / (prescaler * Hz) - 1) -> must be < 65536 for timer1
#define BPM120INT 31249 // 32Hz tick (1/64 note at 120 BPM)
#define BPM140INT 26784 // 37.333Hz tick (1/64 note at 140 BPM)
#define BPM160INT 23436 // 42.667Hz tick (1/64 note at 160 BPM)

const char melodycode[] PROGMEM = {"4D4 G2 G8 B8 A8 B8 G2./ G4 A2/ A8/ A8 G8 A8 B4 G4/ G4 D4 G2 G8 B8 A8 B8 G2. B4 A4 5C4 4B4 A4 G4"};

MMLtone melody = MMLtone(12, melodycode, sizeof(melodycode));


/****************************************************************************/
/*  I : /                                                                   */
/*  P : Setup procedure                                                     */
/*  O : /                                                                   */
/****************************************************************************/
void setup() {
  //stop interrupts
  cli();
  
  //setup melody and pin 13 (test led)
  pinMode(LED_BUILTIN, OUTPUT);
  melody.setup();
  
  //clear TCCR1
  TCCR1A = 0;
  TCCR1B = 0;

  //reset counter value +
  //set compare match register for 32 Hz increments
  TCNT1  = 0;
  OCR1A = BPM120CRY;

  // turn on CTC mode
  // + Set CS12, CS11 and CS10 bits for 8 prescaler
  // + enable timer compare interrupt
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
  TIMSK1 |= (1 << OCIE1A);

  //allow interrupts
  sei();
}

/****************************************************************************/
/*  I : timer1 comparator vector                                            */
/*  P : updates the melody at each tick (1/64 note)                         */
/*  O : /                                                                   */
/****************************************************************************/
ISR(TIMER1_COMPA_vect){
  //timing tests (w/ 16KHz crystal, in microseconds) :
  //4  <= getNextNote() <= 16
  //20 <=   onTick()    <= 256
  
  melody.getNextNote();
  melody.onTick();
}

/****************************************************************************/
/*  I : /                                                                   */
/*  P : Main loop                                                           */
/*  O : /                                                                   */
/****************************************************************************/
void loop() {
    if(melody.last())
      digitalWrite(LED_BUILTIN, HIGH);
  
    if(melody.finished())
      melody.stop();
    else
      melody.start();
}
