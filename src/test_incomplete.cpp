#include "LogWriter.h"
#include "LogReader.h"
#include <math.h>
#include <iostream>

int main(int argc, char** argv) {

  // unbuffered writer (buffer size = 0)
  LogWriter* writer = new LogWriter("corrupt.data", 100, 0);

  float s, c;

  writer->add(&s, "sin");
  writer->add(&c, "cos");

  for (int t=0; t<200; ++t) {
    s = sin(t*M_PI/100.0);
    c = cos(t*M_PI/100.0);
    writer->writeSample();
  }

  // simulate program crash without cleanup
  writer = 0;

  LogReader reader("corrupt.data");

  std::cout << "read " << reader.numChannels() << " channels, "
	    << reader.numTicks() << " samples/channel, at "
	    << reader.frequency() << " samples/sec.\n";

  size_t nsamp = std::min(size_t(5), reader.numTicks());

  for (size_t i=0; i<reader.numChannels(); ++i) {
    
    std::cout << "Channel " << (i+1) << " named " 
	      << reader.channelName(i) << " with units "
	      << reader.channelUnits(i) << ". First " 
      	      << nsamp << " samples: ";

    for (size_t j=0; j<nsamp; ++j) {
      std::cout << reader(i, j) << " ";
    }

    std::cout << "\n";

  }

  return 0;

}
