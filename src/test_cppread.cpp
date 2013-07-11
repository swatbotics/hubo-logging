#include "LogReader.h"
#include <algorithm>
#include <iostream>

int main(int argc, char** argv) {

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " FILENAME\n";
    return 1;
  }

  LogReader reader(argv[1]);

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
