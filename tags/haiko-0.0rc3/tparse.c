#include "voxel.h"
#include <stdio.h>


int main(int argc, char ** argv)
{
  voxel_parse_tab_t voxel_parse_tab;
  FILE * configfile;
  
  if (argc > 1)
    configfile = fopen(argv[1], "r");
  else
    configfile = stdin;
  
  voxel_parse_tab.debug = 1;
  if (0 != voxel_parse_file(configfile, &voxel_parse_tab))
    fprintf(stderr, "ooopsie, voxel_parse_file() failed\n");
  else {
    if (voxel_parse_tab.error)
      fprintf(stderr, "ooopsie, voxel_parse_tab.error\n");
    else {
      voxel_t * voxel;
      int count = 0;
      for (voxel = voxel_parse_tab.first; NULL != voxel; voxel = voxel->next) {
	printf("voxel %x [%f %f %f] (%f  %f  %f)\n", voxel,
	       voxel->pos[0], voxel->pos[1], voxel->pos[2],
	       voxel->color[0], voxel->color[1], voxel->color[2]);
	++count;
      }
      printf("counted %d voxels\n", count);
    }
  }
  
  voxel_free_list(voxel_parse_tab.first);
  
  return 0;
}
