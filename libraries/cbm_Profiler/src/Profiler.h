/*
  Code profiler - counts hits, time spent between start and stop calls
                  as a histogram.
  Plan: break out the histogram thingy into a separate library
        allow the option of histogram or statistics
        
        
  Synopsis:
  #include <Profiler.h>

  Profile section1;

  section1.setup ( section_name, nBins, binSize, lowBinStart );
    where section_name is your arbitrary name for the section being profiled
    nBins is the number of histogram bins you want the timings tallied in
    binSize is the size (width) of each bin
    lowBinStart is the lowest value for the leftmost bin (shifts them all)
    
  void loop() {
    ....
    section1.start ();
    .... some code to be profiled
    section1.stop ();
    ....
    .... eventually,
    section1.report ();
  }

  can reset using section1.reset()

*/

#ifndef Profile_h
#define Profile_h

  class Profile {
    public:
      Profile ();
      void setup ( const char * name, int nBins, int binSize, unsigned long lowBinStart = 0 );
      const char *version();
      void reset ();
      void start ();
      void stop ();
      // print report
      void report ();
    private:
      static const int PROFILE_VERBOSE = 2;
      static const int maxBins = 10;
      static const int nameLen = 20;
      char name [ nameLen ];
      unsigned long nHits;
      unsigned long bins [ maxBins ];
      unsigned long startedAt_ms;
      int nBins;
      int binSize;
      unsigned long lowBinStart;
      unsigned long sum;
      unsigned long totalTime;
  };
  
#endif
