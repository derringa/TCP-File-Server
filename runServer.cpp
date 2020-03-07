#include "FTServer.hpp"

#include <stdio.h>

int main (int argc, char *argv[]) {

     if (argc > 2 || argc < 2) {
          fprintf(stderr, "USAGE ERROR: runServer portnum &\n"); exit(1);
     }

     FTServer server(argv[1]);

     return 0;
}
