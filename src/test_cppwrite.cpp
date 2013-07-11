#include "LogWriter.h"
#include <math.h>

int main(int argc, char** argv) {

  LogWriter writer("test.data", 100, 4096);

  float s, c;

  writer.add(&s, "sin");
  writer.add(&c, "cos");

  for (int t=0; t<200; ++t) {
    s = sin(t*M_PI/100.0);
    c = cos(t*M_PI/100.0);
    writer.writeSample();
  }

  writer.finish();
  
  return 0;

}
