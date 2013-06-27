@ifndef DL_CACHE_H
@define DL_CACHE_H 1

@include <file-fields.h>

@include_leaf dl-cache.h

@copy _DL_CACHE_DEFAULT_ID
@copy _dl_cache_check_flags(flags)
@copy add_system_dir(dir)
@copy LD_SO_CACHE
@copy CACHEMAGIC
@copy CACHEMAGIC_NEW
@copy CACHE_VERSION
@copy CACHEMAGIC_VERSION_NEW
@copy ALIGN_CACHE(addr)

@endif
