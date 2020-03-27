#include "MMLtone.h"

/*
 * Devices used during tests :
 *  - 1 x Arduino Nano V3 / Atmega 328p-au
 *  - 1 x buzzer (part number unknown)
 *  - 1 x 2.2k Ohms resistor (current limitation)
 *  - 1 x 10k Ohms resistor (pull-down)
 *  - 1 x N-channel MOSFET (P3NK507)
 * 
 * Music library testing procedure
 * --------------------------------
 *      MOSFET pins   |   
 *      --------------|------------------
 *      Drain : 1     |   Arduino : D12, Atmega : 16 (+ pull-down to GND in //)
 *      Gate  : 2     |   buzzer -
 *      Source: 3     |   GND
 *      
 *      Buzzer pins   |
 *      --------------|------------------------------------
 *      -             |   MOSFET : 2
 *      +             |   VCC (via 2.2k resistor in series)
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
char notebuffer[8] = {0};
unsigned char noteindex = 0, notesize=95;

MMLtone melody = MMLtone(12);


/****************************************************************************/
/*  I : /                                                                   */
/*  P : Setup procedure                                                     */
/*  O : /                                                                   */
/****************************************************************************/
void setup() {
  //stop interrupts
  cli();

  Serial.begin(9600);
  
  //setup melody and pin 13 (test led)
  melody.getNextNote(&noteindex, notebuffer, melodycode, notesize);
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
  //onTick() witout refresh : 20 us max
  //onTick() with refresh : 284 us max

  melody.onTick(notebuffer);
  if(!melody.refreshed())
    return;

  melody.getNextNote(&noteindex, notebuffer, melodycode, notesize);
}

/****************************************************************************/
/*  I : /                                                                   */
/*  P : Main loop                                                           */
/*  O : /                                                                   */
/****************************************************************************/
void loop() {
    if(melody.finished())
      melody.stop();
    else
      melody.start();
}
