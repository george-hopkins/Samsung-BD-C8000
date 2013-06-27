#!/usr/bin/env bash
umask 0
pushd /testsuite/glibc/objdir
glibc_source=/testsuite/glibc/glibc-2.5.90
libdir=lib
export TIMEOUTFACTOR=30
export CFLAGS='-g -O2 -fgnu89-inline'
$glibc_source/configure --enable-add-ons --prefix=/usr
for i in $glibc_source/*/Makefile; do
  mkdir -p $(basename $(dirname $i))
done

ln -sf /usr/$libdir/libc_nonshared.a libc_nonshared.a
ln -sf /usr/$libdir/libc.a libc.a
ln -sf /usr/$libdir/libc_pic.a libc_pic.a
old="libc_nonshared.a libc.a libc_pic.a"

ln -sf `ls /$libdir/libc.so.*` libc.so
ln -sf `ls /$libdir/libc.so.*` .
old="$old libc.so"

ln -sf `ls /$libdir/ld*.so.*` elf/
ln -sf `ls /$libdir/ld-2.5.90.so` elf/ld.so
ln -sf /sbin/ldconfig elf/
ln -sf /usr/bin/ldd elf/
old="$old elf/ld.so"

ln -sf `ls /$libdir/libdl.so.*` dlfcn/
ln -sf `ls /$libdir/libdl.so.*` dlfcn/libdl.so
old="$old dlfcn/libdl.so"

ln -sf `ls /$libdir/libpthread.so.*` nptl/
ln -sf `ls /$libdir/libpthread.so.*` nptl/libpthread.so
ln -sf /usr/$libdir/libpthread.a nptl/
ln -sf /usr/$libdir/libpthread_pic.a nptl/
ln -sf /usr/$libdir/libpthread_nonshared.a nptl/
old="$old nptl/libpthread.so nptl/libpthread.a"
old="$old nptl/libpthread_nonshared.a nptl/libpthread_pic.a"

ln -sf `ls /$libdir/librt.so.*` rt/
ln -sf `ls /$libdir/librt.so.*` rt/librt.so
old="$old rt/librt.so"

ln -sf `ls /$libdir/libm.so.*` math/
ln -sf `ls /$libdir/libm.so.*` math/libm.so
old="$old math/libm.so"
ln -sf `ls /$libdir/libresolv.so.*` resolv/
ln -sf `ls /$libdir/libresolv.so.*` resolv/libresolv.so
ln -sf `ls /$libdir/libanl.so.*` resolv/
ln -sf `ls /$libdir/libanl.so.*` resolv/libanl.so
old="$old resolv/libresolv.so resolv/libanl.so"
ln -sf `ls /$libdir/libcrypt.so.*` crypt/
ln -sf `ls /$libdir/libcrypt.so.*` crypt/libcrypt.so
old="$old crypt/libcrypt.so"

for file in /usr/$libdir/*crt*.o; do
  ln -sf $file csu/
done

ln -sf /usr/bin/localedef locale/
ln -sf /usr/bin/mtrace malloc/
ln -sf /usr/bin/iconv iconv/iconv_prog

for i in /usr/$libdir/gconv/*; do ln -sf $i iconvdata/`basename $i`; done
old="$old `echo -n iconvdata/*.so`"

# Linking test shared libraries wants this.
make no_deps=t objdir=$PWD -C $glibc_source/csu $PWD/csu/abi-note.o

# For math/atest-exp
for i in add_n sub_n cmp addmul_1 mul_1 mul_n divmod_1 lshift rshift         mp_clz_tab udiv_qrnnd inlines
do
        make objdir=$PWD -C $glibc_source/stdlib $PWD/stdlib/$i.o no_deps=t
done

# For dlfcn/tststatic
make objdir=$PWD -C $glibc_source/csu $PWD/csu/start.o no_deps=t

# Make check will rebuild headers in iconvdata/; it's harmless and doesn't
# take too long.

for i in $old; do
  oldopt="$oldopt -o $PWD/$i"
done

set +e

# Catgets requires the de_DE.ISO-8859-1 locale to be generated before
# it runs.  The localedata tests will do this for us...
make MAKE="make $oldopt" no_deps=t objdir=$PWD -C $glibc_source -k localedata/tests
# Build dlfcn before the rest because it will build shlib.lds which is used
# by tst-putenvmod test. Right solution would be to fix the dependency tree
# for putenvmod test but we test in a customized way so the current solution
# is good for us.
make MAKE="make $oldopt" no_deps=t objdir=$PWD -C $glibc_source/dlfcn -k check
make MAKE="make $oldopt" no_deps=t objdir=$PWD -C $glibc_source -k check

popd

exit 0

