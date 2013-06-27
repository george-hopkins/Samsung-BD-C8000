@ifndef DL_PROCINFO_H
@define DL_PROCINFO_H

#define PROCINFO_CLASS static
#define PROCINFO_DECL
@include_leaf dl-procinfo.c

@include_leaf dl-procinfo.h
#define PROCINFO_CLASS static
@include_leaf dl-procinfo.c

@copy _DL_HWCAP_PLATFORM
@copy _DL_HWCAP_COUNT
@copy _DL_FIRST_PLATFORM
@copy _DL_PLATFORMS_COUNT
@copy _dl_procinfo(X)
@copy _dl_hwcap_string(idx)
@copy _dl_platform_string(idx)
@copy _dl_string_hwcap(str)
@copy _dl_string_platform(str)
@copy HWCAP_IMPORTANT

@endif
