#ifndef MUSIC_H_INCLUDED
#define MUSIC_H_INCLUDED
#include <Arduino.h>

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

  public:
      MMLtone(unsigned char Pin);
      ~MMLtone();
      void setup();
      void start();
      int onTick(const char* nextnote);
      void getNextNote(unsigned char* index, char buf[], const char melody[], const unsigned char notesize);
      void stop();
      void reset();

      bool started();
      bool finished();
      bool last();
      bool refreshed();
};
#endif
