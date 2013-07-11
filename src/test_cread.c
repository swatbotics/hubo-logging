#include "mrdplot.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char** argv) {

  MRDPLOT_DATA* data; 
  int i, j, nsamp;

  if (argc != 2) {
    fprintf(stderr, "usage: %s FILENAME\n", argv[0]);
    return 1;
  }

  data = read_mrdplot(argv[1]);

  if (!data) {
    fprintf(stderr, "error opening %s\n", argv[1]);
    return 1;
  }

  printf("read %d channels, %d samples/channel, %d samples total at %g samples/sec\n",
	 data->n_channels, data->n_points, data->total_n_numbers, data->frequency);

  nsamp = 5;
  if (data->n_points < nsamp) { nsamp = data->n_points; }

  for (i=0; i<data->n_channels; ++i) {
    printf("Channel %d named %s with units %s. First %d samples: ",
	   i, data->names[i], data->units[i], nsamp);
    for (j=0; j<nsamp; ++j) {
      printf("%g ", data->data[data->n_channels*j + i]);
    }
    printf("\n");
  }

  return 0;

}
