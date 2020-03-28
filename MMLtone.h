#ifndef MUSIC_H_INCLUDED
#define MUSIC_H_INCLUDED
#include <Arduino.h>

#define NOTBUFSZ 8

class MMLtone
{ 
  private:
      unsigned char   pin;
      bool            isFinished;
      bool            lastnote;
      bool            isStarted;
      bool            cut_note;
      bool            isRefreshed;
      unsigned char   m_octave;
      unsigned char   m_nbtick;
      unsigned char   m_duration;
      unsigned char   m_index;
      unsigned char   m_size;
      char            m_buffer[NOTBUFSZ];
      char*           m_code;

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
