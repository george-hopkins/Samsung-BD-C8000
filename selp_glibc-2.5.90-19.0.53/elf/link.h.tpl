@ifndef LINK_H
@define LINK_H 1

/* These declarations can't be copied from link.h because they use
   token pasting.  */
@define ElfW(type)      _ElfW (Elf, __ELF_NATIVE_CLASS, type)
@define _ElfW(e,w,t)    _ElfW_1 (e, w, _##t)
@define _ElfW_1(e,w,t)  e##w##t

#include <link.h>

@copy __ELF_NATIVE_CLASS

@endif
