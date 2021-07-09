#ifndef MUSIC_H_INCLUDED
#define MUSIC_H_INCLUDED

#define NOTBUFSZ 8

class MMLtone
{ 
  private:
      unsigned char   pin;                  //pin to which output the Tone() signal
      unsigned char   m_octave;             //octave in which the notes will be played until updated
      unsigned char   m_nbtick;             //amount of ticks remaining to play the note (decrements while playing)
      unsigned char   m_duration;           //duration or value of the notes until updated (e.g. : 1/16 note)
      unsigned char   m_next;               //index of the next note in the MML code
      unsigned char   m_current;            //index of the current note playing in the MML code
      unsigned char   m_size;               //size (in bytes) of the whole MML code
      char            m_buffer[NOTBUFSZ];   //buffer holding the next note played
      char*           m_code;               //PROGMEM address of the entire MML code
      bool            isFinished;           //flag indicating whether the last note has been played
      bool            lastnote;             //flag indicating whether the last note is being played
      bool            isStarted;            //flag indicating whether the music is to be played or not
      bool            cut_note;             //flag indicating whether there is a clear-cut in the note
      bool            isRefreshed;          //flag indicating whether the next note is to be read

  protected:
    //declared as inline to avoid function calls and speed up process
    inline float getFrequency(const unsigned char note) __attribute__((always_inline));

  public:
      MMLtone(const unsigned char Pin, const char* code, const unsigned char siz);
      ~MMLtone();
      void setup();
      void start();
      int onTick();
      void getNextNote();
      void stop();
      void reset();

      bool started();
      bool finished();
      bool last();
      bool refreshed();
};
#endif
