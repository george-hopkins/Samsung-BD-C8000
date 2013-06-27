subdir := cross

cross-tpl-files := dl-cache.h dl-procinfo.c dl-procinfo.h ldconfig.h \
		   lib-names.h a.out.h cross-endian.h cross-config.h

cross-stpl-files := field-decls.h

include ../Rules
include ../Makeconfig

CFLAGS-cross-config.h.i := '-DLIBDIR="$(libdir)"' \
			   '-DSLIBDIR="$(slibdir)"' \
			   '-DSYSCONFDIR="$(sysconfdir)"'
