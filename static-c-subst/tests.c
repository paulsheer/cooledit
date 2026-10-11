/* Self-test driver: compile the substitute with STANDALONE so its built-in
 * test main() is included.  Run via `make tests` (or `make check`). */
#define STANDALONE
#include "getaddrinfo.c"
