#include <stdio.h>
#include <string.h>
#include "ijvm.h"
#include "util.h"
#include "snapshot.h"

static void print_help(void)
{ 
  printf("Usage: ./ijvm binary \n");
  printf("       ./ijvm -r snapshot_file \n");
}

int main(int argc, char **argv) 
{
  if (argc < 2) 
  {
    print_help();
    return 1;
  }

  ijvm* m;

  if (strcmp(argv[1], "-r") == 0)
  {
    if (argc < 3)
    {
      print_help();
      return 1;
    }
    m = load_snapshot(argv[2], stdin, stdout);
    if (m == NULL)
    {
      fprintf(stderr, "Couldn't load snapshot %s\n", argv[2]);
      return 1;
    }
  }
  else
  {
    m = init_ijvm_std(argv[1]);
    if (m == NULL) 
    {
      fprintf(stderr, "Couldn't load binary %s\n", argv[1]);
      return 1;
    }
  }

  run(m);

  destroy_ijvm(m);

  return 0;
}
