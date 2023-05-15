#include <stdlib.h>

#include "watcher.h"
#include "store.h"

int main(int argc, char *argv[]) {
  if (ticker())
    return EXIT_FAILURE;
  else
    return EXIT_SUCCESS;
}
