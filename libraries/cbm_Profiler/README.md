This is an Arduino library for the profiling of program execution

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
