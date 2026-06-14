/* stub for Ruby's internal/sanitizers.h and ruby/defines.h — standalone Onigmo build */

#ifndef INTERNAL_SANITIZERS_H
#define INTERNAL_SANITIZERS_H

#include <stdbool.h>

#define NO_SANITIZE(x, y) y

#ifdef __GNUC__
# define RB_GNUC_EXTENSION        __extension__
# define RB_GNUC_EXTENSION_BLOCK(x) __extension__ ({ x; })
#else
# define RB_GNUC_EXTENSION
# define RB_GNUC_EXTENSION_BLOCK(x) (x)
#endif

#endif /* INTERNAL_SANITIZERS_H */
