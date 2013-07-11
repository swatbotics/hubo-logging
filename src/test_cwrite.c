#include "mrdplot.h"
#include <math.h>
#include <assert.h>

int main(int argc, char** argv) {

  MRDPLOT_DATA* data = malloc_mrdplot_data(2, 200);
  int offs = 0, t;

  data->filename = "test.data";
  data->frequency = 100;
  
  data->names[0] = "sin";
  data->names[1] = "cos";
  
  data->units[0] = "1";
  data->units[1] = "1";
  
  for (t=0; t<200; ++t) {
    data->data[offs++] = sin(t*M_PI/100.0);
    data->data[offs++] = cos(t*M_PI/100.0);
  }

  assert(offs == 400);

  write_mrdplot_file(data);

  return 0;

}
