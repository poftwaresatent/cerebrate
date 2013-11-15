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
  
  voxel_parse_file(configfile, &voxel_parse_tab);
  if (voxel_parse_tab.error)
    fprintf(stderr, "ooopsie, voxel_parse_tab.error\n");
  else {
    voxel_t * voxel;
    int count = 0;
    for (voxel = voxel_parse_tab.first; NULL != voxel; voxel = voxel->next) {
      printf("voxel %x [%f %f %f] (%f  %f  %f)\n", voxel,
	     voxel->x, voxel->y, voxel->z, voxel->r, voxel->g, voxel->b);
      ++count;
    }
    printf("counted %d voxels\n", count);
  }
  
  voxel_free_list(voxel_parse_tab.first);
  
  return 0;
}
