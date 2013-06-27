#ifdef __KERNEL__
# ifdef CONFIG_X86_32
#  include "kgdb_32.h"
# else
#  include "kgdb_64.h"
# endif
#else
# ifdef __i386__
#  include "kgdb_32.h"
# else
#  include "kgdb_64.h"
# endif
#endif
