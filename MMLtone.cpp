#include "MMLtone.h"
#include "pitches.h"
#include <Arduino.h>

/****************************************************************
 * I : Pin on which the buzzer is plugged                       *
 *     Pointer to the MML code array                            *
 *     Size of the code array (sizeof())                        *
 * P : Builds a new MMLtone module                              *
 * O : /                                                        *
 ****************************************************************/
MMLtone::MMLtone(const unsigned char Pin, const char* code, const unsigned char siz)
:isFinished(false), lastnote(false), isStarted(false), cut_note(false), isRefreshed(false), m_octave(0), m_nbtick(0), m_duration(0), m_next(0), m_current(0), m_buffer{0}
{
  this->pin = Pin;
  this->m_code = code;
  this->m_size = siz;
}

/****************************************************************
 * I : /                                                        *
 * P : Destroys the current MMLtone module                      *
 * O : /                                                        *
 ****************************************************************/
MMLtone::~MMLtone()
{}

/****************************************************************
 * I : /                                                        *
 * P : Set the pin as an output                                 *
 * O : /                                                        *
 ****************************************************************/
void MMLtone::setup(){
  pinMode((int)this->pin, OUTPUT);
}

/****************************************************************
 * I : /                                                        *
 * P : Set the isStarted flag as true                           *
 * O : /                                                        *
 ****************************************************************/
void MMLtone::start(){
  if(!this->finished())
    this->isStarted=true;
}

/****************************************************************/
/*  I : /                                                       */
/*  P : When a tick is reached, decode a note and play it       */
/*  O : /                                                       */
/****************************************************************/
int MMLtone::onTick()
{
    //if music is supposed to be stopped, exit
    if(!this->isStarted)
      return 0;

    //if note is to be cut, noTone() during the last tick
    if(this->cut_note && this->m_nbtick == 1)
      noTone(this->pin);

    //on first tick, clear the flag indicating next note is to be decoded
    if(this->m_nbtick >= (64 / this->m_duration) - 1)
      this->isRefreshed = false;

    //check if note is still to be played
    if(this->m_nbtick > 0)
    {
        this->m_nbtick--;
        return 0;
    }

    //if last note has been played, set the finished flag
    if(this->m_current == this->m_next){
      this->isFinished = true;
      return 0;
    }

    //if last note has been reached, set the last note flag
    if(this->m_next >= this->m_size)
      this->lastnote = true;


    //NOTE DECODING

    //get the code for the current note + declare all variables
    char* it = this->m_buffer;
    float frequency;
    unsigned char duration = 0;
    
    //reinitialise the note cut flag
    this->cut_note = false;

    //decode eventual octave change
    if(isdigit(*it))
    {
        this->m_octave = *it - 48; //translate ASCII to number ('0' = 48)
        it++;
    }

    //set the base freq. for the note requested (octave 0 @ A440 by default)
    switch(*it)
    { 
      case 'C':
      case 'c':
        frequency = 16.35;
        break;
      
      case 'D':
      case 'd':
        frequency = 18.35;
        break;
      
      case 'E':
      case 'e':
        frequency = 20.6;
        break;
      
      case 'F':
      case 'f':
        frequency = 21.83;
        break;
      
      case 'G':
      case 'g':
        frequency = 24.5;
        break;

      case 'A':
      case 'a':
        frequency = 27.5;
        break;
      
      case 'B':
      case 'b':
        frequency = 30.87;
        break;
      
      case 'R':
      case 'r':
      default:
        frequency = 0.0;
        break;
    }

    //multiply the freq. to get the right octave (freq * 2 ^ octave)
    frequency *= (float)(1 << this->m_octave);
    it++;

    //decode sharp or flat notes
    if ((*it == '#') || (*it =='+'))
    {
        frequency *= 1.059;
        it++;
    }
    if (*it == '-')
    {
        frequency *= 0.9443; // = div 1.059
        it++;
    }

    //play the note
    // + set the flag to decode next note on 2nd tick
    tone(this->pin, frequency);
    this->isRefreshed = true;



    //DURATION DECODING

    //decode note duration (possible 2 digits)
    if(isdigit(*it))
    {
      duration = *it - 48;
      it++;
    }
    if(isdigit(*it))
    {
      duration = (duration * 10) + (*it - 48);
      it++;
    }

    //if none specified, reuse last specified
    //otherwise, update duration
    if(!duration)
      duration = this->m_duration;
    else
      this->m_duration = duration;

    //set the number of ticks
    // (nb of ticks = nb of 1/64 notes to reach proper duration)
    // + rounds up the number of ticks to a divisor of 64 (int division)
    this->m_nbtick = 64 / duration;

    //decode dotted note (duration * 1.5)
    if (*it == '.')
    {
        this->m_nbtick = (unsigned char)((float)this->m_nbtick * 1.5);
        it++;
    }

    //if note is to be cut (ends with '/'), set the flag to noTone() for the last tick
    if (*it == '/')
        this->cut_note = true;

    //decrement tick count (1 cycle is used to refresh note)
    this->m_nbtick--;

    return 0;
}

/****************************************************************/
/*  I : /                                                       */
/*  P : Fetches the next note in memory and loads in in the buf.*/
/*  O : /                                                       */
/****************************************************************/
void MMLtone::getNextNote(){
  //if note is not to be refreshed, exit
  if(this->m_next>0 && !this->isRefreshed)
    return;

  //update current note index
  this->m_current = this->m_next;

  //if last byte of the MML code has already been read, exit
  if(this->m_next >= this->m_size)
    return;

  //read the EEPROM memory byte by byte to retrieve the next note
  //  and put the note in the buffer
  unsigned char i=0;
  do
  {
    this->m_buffer[i] = pgm_read_word_near(this->m_code + this->m_next);
    this->m_next++;
    i++;
  }while(this->m_next < this->m_size && i<NOTBUFSZ && this->m_buffer[i-1]!=' '&& this->m_buffer[i-1]!='\0');
  this->m_buffer[i] = '\0';
}

/****************************************************************
 * I : /                                                        *
 * P : Turn the tone off and unset the started flag             *
 * O : /                                                        *
 ****************************************************************/
void MMLtone::stop(){
    noTone(this->pin);
    this->isStarted=false;
}

/****************************************************************
 * I : /                                                        *
 * P : Reset all the flags to 0                                 *
 * O : /                                                        *
 ****************************************************************/
void MMLtone::reset(){
  this->lastnote=false;
  this->isFinished=false;
  this->m_next = 0;
  this->m_current = 0;
}

/****************************************************************
 * I : /                                                        *
 * P : Inform about whether the melody is started or not        *
 * O : Melody state                                             *
 ****************************************************************/
bool MMLtone::started()
{
  return this->isStarted;
}

/****************************************************************
 * I : /                                                        *
 * P : Inform about whether the melody is finished or not       *
 * O : Melody state                                             *
 ****************************************************************/
bool MMLtone::finished()
{
  return this->isFinished;
}

/****************************************************************
 * I : /                                                        *
 * P : Informs about whether the last tone is reached or not    *
 * O : Melody state                                             *
 ****************************************************************/
bool MMLtone::last()
{
  return this->lastnote;
}

/****************************************************************
 * I : /                                                        *
 * P : Informs about whether the last tone is reached or not    *
 * O : Melody state                                             *
 ****************************************************************/
bool MMLtone::refreshed()
{
  return this->isRefreshed;
}

/****************************************************************
 * I : Index of a note compared to the A at the octave 0        *
 *        e.g. : C@2 = 1 octave + diff between A1 and C2        *
 *                   = 12 + 3 = 15                              *
 * P : Get the corresponding frequency for a note               *
 * O : Frequency of the note                                    *
 ****************************************************************/
float getFrequency(const unsigned char note){
  switch(note){
    case NOTE_A0:     // A @ octave 0
      return 27.50;
      break;

    case NOTE_As0:     // A#/Bb @ octave 0
      return 29.14;
      break;

    case NOTE_B0:    // B @ octave 0
      return 30.87;
      break;
      
    case NOTE_C1:     //C @ octave 1
      return 32.70;
      break;

    case NOTE_Cs1:     // C#/Db @ octave 1
      return 34.65;
      break;

    case NOTE_D1:     // D @ octave 1
      return 36.71;
      break;

    case NOTE_Ds1:     // D#/Eb @ octave 1
      return 38.89;
      break;

    case NOTE_E1:     // E @ octave 1
      return 41.20;
      break;

    case NOTE_F1:     // F @ octave 1
      return 43.65;
      break;

    case NOTE_Fs1:     // F#/Gb @ octave 1
      return 46.25;
      break;

    case NOTE_G1:     // G @ octave 1
      return 49.00;
      break;

    case NOTE_Gs1:    // G#/Ab @ octave 1
      return 51.91;
      break;

    case NOTE_A1:     // A @ octave 1
      return 55.00;
      break;

    case NOTE_As1:     // A#/Bb @ octave 1
      return 58.27;
      break;

    case NOTE_B1:    // B @ octave 1
      return 61.74;
      break;
      
    case NOTE_C2:     //C @ octave 2
      return 65.41;
      break;

    case NOTE_Cs2:     // C#/Db @ octave 2
      return 69.30;
      break;

    case NOTE_D2:     // D @ octave 2
      return 73.42;
      break;

    case NOTE_Ds2:     // D#/Eb @ octave 2
      return 77.78;
      break;

    case NOTE_E2:     // E @ octave 2
      return 82.41;
      break;

    case NOTE_F2:     // F @ octave 2
      return 87.31;
      break;

    case NOTE_Fs2:     // F#/Gb @ octave 2
      return 92.50;
      break;

    case NOTE_G2:     // G @ octave 2
      return 98.00;
      break;

    case NOTE_Gs2:    // G#/Ab @ octave 2
      return 103.83;
      break;

    case NOTE_A2:     // A @ octave 2
      return 110.00;
      break;

    case NOTE_As2:     // A#/Bb @ octave 2
      return 116.54;
      break;

    case NOTE_B2:    // B @ octave 2
      return 123.47;
      break;
      
    case NOTE_C3:     //C @ octave 3
      return 130.81;
      break;

    case NOTE_Cs3:     // C#/Db @ octave 3
      return 138.59;
      break;

    case NOTE_D3:     // D @ octave 3
      return 146.83;
      break;

    case NOTE_Ds3:     // D#/Eb @ octave 3
      return 155.56;
      break;

    case NOTE_E3:     // E @ octave 3
      return 164.81;
      break;

    case NOTE_F3:     // F @ octave 3
      return 174.61;
      break;

    case NOTE_Fs3:     // F#/Gb @ octave 3
      return 185.00;
      break;

    case NOTE_G3:     // G @ octave 3
      return 196.00;
      break;

    case NOTE_Gs3:    // G#/Ab @ octave 3
      return 207.65;
      break;

    case NOTE_A3:     // A @ octave 3
      return 220.00;
      break;

    case NOTE_As3:     // A#/Bb @ octave 3
      return 233.08;
      break;

    case NOTE_B3:    // B @ octave 3
      return 246.94;
      break;
      
    case NOTE_C4:     //C @ octave 4
      return 261.63;
      break;

    case NOTE_Cs4:     // C#/Db @ octave 4
      return 277.18;
      break;

    case NOTE_D4:     // D @ octave 4
      return 293.66;
      break;

    case NOTE_Ds4:     // D#/Eb @ octave 4
      return 311.13;
      break;

    case NOTE_E4:     // E @ octave 4
      return 329.63;
      break;

    case NOTE_F4:     // F @ octave 4
      return 349.23;
      break;

    case NOTE_Fs4:     // F#/Gb @ octave 4
      return 369.99;
      break;

    case NOTE_G4:     // G @ octave 4
      return 392.00;
      break;

    case NOTE_Gs4:    // G#/Ab @ octave 4
      return 415.30;
      break;

    case NOTE_A4:     // A @ octave 4
      return 440.00;
      break;

    case NOTE_As4:     // A#/Bb @ octave 4
      return 466.16;
      break;

    case NOTE_B4:    // B @ octave 4
      return 493.88;
      break;
      
    case NOTE_C5:     //C @ octave 5
      return 523.25;
      break;

    case NOTE_Cs5:     // C#/Db @ octave 5
      return 554.37;
      break;

    case NOTE_D5:     // D @ octave 5
      return 587.33;
      break;

    case NOTE_Ds5:     // D#/Eb @ octave 5
      return 622.25;
      break;

    case NOTE_E5:     // E @ octave 5
      return 659.25;
      break;

    case NOTE_F5:     // F @ octave 5
      return 698.46;
      break;

    case NOTE_Fs5:     // F#/Gb @ octave 5
      return 739.99;
      break;

    case NOTE_G5:     // G @ octave 5
      return 783.99;
      break;

    case NOTE_Gs5:    // G#/Ab @ octave 5
      return 830.61;
      break;

    case NOTE_A5:     // A @ octave 5
      return 880.00;
      break;

    case NOTE_As5:     // A#/Bb @ octave 5
      return 932.33;
      break;

    case NOTE_B5:    // B @ octave 5
      return 987.77;
      break;
      
    case NOTE_C6:     //C @ octave 6
      return 1046.50;
      break;

    case NOTE_Cs6:     // C#/Db @ octave 6
      return 1108.73;
      break;

    case NOTE_D6:     // D @ octave 6
      return 1174.66;
      break;

    case NOTE_Ds6:     // D#/Eb @ octave 6
      return 1244.51;
      break;

    case NOTE_E6:     // E @ octave 6
      return 1318.51;
      break;

    case NOTE_F6:     // F @ octave 6
      return 1396.91;
      break;

    case NOTE_Fs6:     // F#/Gb @ octave 6
      return 1479.98;
      break;

    case NOTE_G6:     // G @ octave 6
      return 1567.98;
      break;

    case NOTE_Gs6:    // G#/Ab @ octave 6
      return 1661.22;
      break;

    case NOTE_A6:     // A @ octave 6
      return 1760.00;
      break;

    case NOTE_As6:     // A#/Bb @ octave 6
      return 1864.66;
      break;

    case NOTE_B6:    // B @ octave 6
      return 1975.53;
      break;
      
    case NOTE_C7:     //C @ octave 7
      return 2093.00;
      break;

    case NOTE_Cs7:     // C#/Db @ octave 7
      return 2217.46;
      break;

    case NOTE_D7:     // D @ octave 7
      return 2349.32;
      break;

    case NOTE_Ds7:     // D#/Eb @ octave 7
      return 2489.02;
      break;

    case NOTE_E7:     // E @ octave 7
      return 2637.02;
      break;

    case NOTE_F7:     // F @ octave 7
      return 2793.83;
      break;

    case NOTE_Fs7:     // F#/Gb @ octave 7
      return 2959.96;
      break;

    case NOTE_G7:     // G @ octave 7
      return 3135.96;
      break;

    case NOTE_Gs7:    // G#/Ab @ octave 7
      return 3322.44;
      break;

    case NOTE_A7:     // A @ octave 7
      return 3520.00;
      break;

    case NOTE_As7:     // A#/Bb @ octave 7
      return 3729.31;
      break;

    case NOTE_B7:    // B @ octave 7
      return 3951.07;
      break;
      
    case NOTE_C8:     //C @ octave 8
      return 4186.01;
      break;

    case NOTE_Cs8:     // C#/Db @ octave 8
      return 4434.92;
      break;

    case NOTE_D8:     // D @ octave 8
      return 4698.63;
      break;

    case NOTE_Ds8:     // D#/Eb @ octave 8
      return 4978.03;
      break;

    case NOTE_E8:     // E @ octave 8
      return 5274.04;
      break;

    case NOTE_F8:     // F @ octave 8
      return 5587.65;
      break;

    case NOTE_Fs8:     // F#/Gb @ octave 8
      return 5919.91;
      break;

    case NOTE_G8:     // G @ octave 8
      return 6271.93;
      break;

    case NOTE_Gs8:    // G#/Ab @ octave 8
      return 6644.88;
      break;

    case NOTE_A8:     // A @ octave 8
      return 7040.00;
      break;

    case NOTE_As8:     // A#/Bb @ octave 8
      return 7458.62;
      break;

    case NOTE_B8:    // B @ octave 8
      return 7902.13;
      break;

    default :       //code does not match any note
      return 0.0;
      break;
  } 
}
