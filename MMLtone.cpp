#include "MMLtone.h"
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
