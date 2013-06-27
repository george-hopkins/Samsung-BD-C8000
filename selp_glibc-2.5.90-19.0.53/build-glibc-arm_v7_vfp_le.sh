#!/bin/bash
# SELP Glibc (GNU C Libraries) Auto-Build Script
# ==============================================
#            (2008.04.17)
# This script was originally generated from mvl-edition-rpmbuild
# and edited using exit 1 , exit 0 intercept command at %build line of *.spec file.
#
# @Requirements:
#  . Cross Compiler: SELP Cross Toolchain
#  . Host PC's Utility: gmake 3.80 , sed 4.1.5 , autoconf 2.59 , gawk 3.1.5 ,
#                       perl 5.8.8 , bash 3.1.7
#
# @Caution:
#  1. Confirm  permission 777  of /tmp directory in your linux pc.
#  2. Copy devpts , hosts.conf , nsswitch.conf , tzconfig , tzcode2007c.tar.gz ,tzdata2007c.tar.gz from
#      ~/montavista/rpm/SOURCES/ directory.
#
# @History:
# 2008.01.02 Lim,GeunSik Created initial glibc build script code for glibc-2.5.9
# 2008.01.02 Lim,GeunSik Resolved path finding of /usr/lib/rpm/debugedit(rpm-build package) command.
# 2008.01.03 Lim,GeunSik Changed cp command option from "-a" to "-rf" to copy timezone directory's files.
#                        ex) cp -a ../timezone/* ./LOCALOBJDIR   --> cp -rf ../timezone/* ./LOCALOBJDIR
# 2008.01.04 Lim,GeunSik Solved problem that happend ".debug_abbrev" error when use cross-prelink command.
#                        Replaced command from eu-strip  to arm_v6_vfp_le-objcopy for standard debug info.        
# 2008.01.04 Lim,GeunSik If ./objdir/ already existed in ./gcc/ dir, Remove ./objdir/ for successfull compiliation.
# 2009.04.17 Lee,Hakbong migration to Moblinux.5.0.24 arm_v7_vfp_le, add -fshort-wchar option
# 2009.11.13 Lee,JungSeung enable to backtrace on glibc, add -mpoke-function-name
# 2009.11.16 Lee,JungSeung replace file name tzcode2007c.tar.gz => tzcode2009k.tar.gz
#                                            tzdata2007c.tar.gz => tzdata2009m.tar.gz
#


RPM_BUILD_DIR=`pwd` 
RPM_OPT_FLAGS="-O2"
export  RPM_BUILD_DIR RPM_OPT_FLAGS 

RPM_PACKAGE_NAME="glibc"
RPM_PACKAGE_VERSION="2.5.90"
RPM_PACKAGE_RELEASE="19.0.53.custom"
export RPM_PACKAGE_NAME RPM_PACKAGE_VERSION RPM_PACKAGE_RELEASE

RPM_BUILD_ROOT=""
export RPM_BUILD_ROOT
  
StartTime="`date '+%Y-%m-%d(%H:%M)'`"

if [ -z "$1" ]
then
  echo ""
  echo "--------------------------------------- "
  echo "GLIBC AutoBuilder 1.0 (SELP) 2009.11.16 "
  echo "--------------------------------------- "
  echo "Usage : `basename $0` absolute-dir-to-install(ex:/home/yourid/glibcbuild.1025)"
  echo ""
  echo ""
  exit 1
fi

INSTALL_DIR=$1
export INSTALL_DIR
  
set -x
umask 022
cd $RPM_BUILD_DIR

LANG=C
export LANG

export CFLAGS='-mpoke-function-name -fshort-wchar -g -O2'
export CXXFLAGS='-mpoke-function-name -fshort-wchar'


CC="arm_v7_vfp_le-gcc " ; export CC ; 
AS="arm_v7_vfp_le-as" ; export AS ; 
LD="arm_v7_vfp_le-ld" ; export LD ; 
AR="arm_v7_vfp_le-ar" ; export AR ; 
RANLIB="arm_v7_vfp_le-ranlib" ; export RANLIB ; 
CPP="arm_v7_vfp_le-cpp " ; export CPP ; 
CXX="arm_v7_vfp_le-g++ " ; export CXX ; 
NM="arm_v7_vfp_le-nm" ; export NM ; 
STRIP="true" ; export STRIP ; 
CFLAGS="${CFLAGS:--O2}" ; export CFLAGS ; 
CXXFLAGS="${CXXFLAGS:--O2}" ; export CXXFLAGS ; 
FFLAGS="${FFLAGS:--O2}" ; export FFLAGS ; 
LDFLAGS="" ; export LDFLAGS ; 
true


# Touch all configure scripts.  Glibc requires autoconf 2.5x to build,
# which is awkward.
find . -name configure -exec touch '{}' ';'

# Unforunately, glibc's configure script tries to locate perl in $PATH
# and then run it.  Force it to pick up the right location.
export PERL=/usr/bin/perl

# When you run multiple versions of glibc using the same dynamic loader,
# they have to be configured very similarly.  Changing which thread add-on
# is used works.  Changing which kernel is required works.  Changing
# whether TLS is supported does not work; if the linker includes TLS support
# but libc does not, dlopen will crash.

numaopt=--with-numa-policy=yes


# Can't link these tests cross at build time because the require parts of
# glibc that aren't built yet.

if [[ -d ./objdir ]]; then  rm -rf ./objdir;fi
mkdir -p objdir
# write the config.cache file in objdir which will be used
# by configure option --cache-file

pushd objdir
# Things configure will get wrong:
#  - ac_tool_prefix=mipsel-hardhat-linux instead of mips_fp_le
#    (we override the only thing this is used for anyway)
#  - CPP=/lib/cpp instead of arm_v7_vfp_le-gcc -E
#    when bootstrapping, because no <assert.h> exists for the
#    target.  No harm done, since it never invokes $(CPP) anyway.
# echo "ac_cv_prog_cc_works=no">>config.cache
# echo "ac_cv_prog_cc_cross=yes">>config.cache
# This test relies on -lgcc_eh, which isn't built yet for gcc-bootstrap.
# The value is verified correct for ARM, MIPS, x86, SH, PowerPC.
# echo "libc_cv_gcc_dwarf2_unwind_info=no_registry_needed">>config.cache
# This test links to -lc.  Which isn't built yet, of course.
# echo "libc_cv_asm_set_directive=yes">>config.cache
popd

echo "●SELP: Make Step - localedef support ( running  configure & make )"
mv localedef objdir/

[ ! -f objdir ] && mkdir -p objdir ; 
cd objdir && ../configure \
	--host=armv7fl-montavista-linux-gnueabi \
	--build=i686-pc-linux-gnu \
        --prefix=/usr \
        --exec-prefix=/usr \
        --bindir=/usr/bin \
        --sbindir=/usr/sbin \
        --sysconfdir=/etc \
        --datadir=/usr/share \
        --includedir=/usr/include \
        --libdir=/usr/lib \
        --libexecdir=/usr/libexec \
        --localstatedir=/var \
        --sharedstatedir=/usr/share \
        --mandir=/usr/share/man \
        --infodir=/usr/share/info \
	--libexecdir=/usr/lib \
	--enable-add-ons \
	--without-cvs \
        --enable-profile \
	--with-tls --with-__thread \
	--enable-kernel=2.6.18 \
	$numaopt \
	--with-glibc=$PWD/../ \
	2>&1
if [ $? != 0 ]; then echo "SELP: glibc Error Occurred while configure task "; exit 1 ;fi

mv localedef ../

make -j 1	2>&1


echo "●SELP: Make Step - tzcode/tzdata support ( running  copy )"
#if cross compiling....
mkdir -p ../LOCALOBJDIR

pushd ../LOCALOBJDIR
cp -a ../timezone/* .
tar xvfz $RPM_BUILD_DIR/tzcode2009k.tar.gz
tar xvfz $RPM_BUILD_DIR/tzdata2009m.tar.gz
if [ $? != 0 ]; then echo "SELP: glibc Error Occurred while decompressing tzdata2009k.tar.gz "; exit 1 ;fi

make 2>&1
if [ $? != 0 ]; then echo "SELP: glibc Error Occurred while make task "; exit 1 ;fi
popd
#endif cross compiling....

####################
#  INSTALL
#

  
set -x
umask 022
cd $RPM_BUILD_DIR

rm -rf 
mkdir -p $INSTALL_DIR/target

echo "●SELP: Install Step - <install-dir>/target/ support ( running  make install )"
cd objdir
make install_root=$INSTALL_DIR/target install 2>&1
if [ $? != 0 ]; then echo "SELP: glibc Error Occurred while make install task "; exit 1 ;fi

#add empty /etc/ld.so.conf file
touch $INSTALL_DIR/target/etc/ld.so.conf

##i18n support
# make cross locale-def to build locale files

echo "●SELP: Install Step - i18n support ( running configure )"
pushd ../localedef
CC=gcc \
AS=as \
LD=ld \
CPP=cpp \
CXX=g++ \
STRIP=false \
sh ./configure \
       --prefix=/usr \
       --with-glibc=$PWD/..
popd

echo "●SELP: Install Step - i18n support ( running make )"
CC=gcc \
AS=as \
LD=ld \
CPP=cpp \
CXX=g++ \
STRIP=false \
make install_root=$INSTALL_DIR/target \
        objdir=`pwd` -C ../localedef \
        LOCALEDEF_OPTS=--little-endian  \
        2>&1

##end of i18n support

(cd ../manual && makeinfo --html libc.texinfo)

mkdir -p $INSTALL_DIR/target/etc/init.d
mkdir -p $INSTALL_DIR/target/etc/default

# nsswitch.conf
install -m 644 $RPM_BUILD_DIR/nsswitch.conf $INSTALL_DIR/target/etc/.
# host.conf
install -m 644 $RPM_BUILD_DIR/host.conf $INSTALL_DIR/target/etc/.
# Should this still be in the glibc package?
install -m 755 $RPM_BUILD_DIR/devpts $INSTALL_DIR/target/etc/default/devpts

install -m 755 $RPM_BUILD_DIR/nscd $INSTALL_DIR/target/etc/init.d/nscd
install -m 755 $RPM_BUILD_DIR/tzconfig $INSTALL_DIR/target/usr/sbin/.


#if cross compiling....
( cd ../LOCALOBJDIR
export TZBASE="africa antarctica asia australasia europe northamerica southamerica etcetera factory solar87 solar88 solar89 backward systemv"
./zic -d $INSTALL_DIR/target/usr/share/zoneinfo/right/ -L leapseconds -y yearistype $TZBASE
./zic -d $INSTALL_DIR/target/usr/share/zoneinfo/posix/ -L /dev/null -y yearistype $TZBASE
./zic -d $INSTALL_DIR/target/usr/share/zoneinfo/ -L /dev/null -y yearistype $TZBASE
cp $INSTALL_DIR/target/usr/share/zoneinfo/America/New_York $INSTALL_DIR/target/usr/share/zoneinfo/posixrules
)
#endif


echo "●SELP: Install Step - Install libc_pic in the correct location ( using install command)"
# Install libc_pic in the correct location
mkdir -p           $INSTALL_DIR/target/usr/lib/libc_pic

install -m 644 libc_pic.a      $INSTALL_DIR/target/usr/lib/.
install -m 644 libc.map        $INSTALL_DIR/target/usr/lib/libc_pic.map

install -m 644 elf/soinit.os   $INSTALL_DIR/target/usr/lib/libc_pic/soinit.o
install -m 644 elf/sofini.os   $INSTALL_DIR/target/usr/lib/libc_pic/sofini.o

install -m 644 elf/interp.os   $INSTALL_DIR/target/usr/lib/libc_pic/interp.o

install -m 644 math/libm_pic.a $INSTALL_DIR/target/usr/lib/.
install -m 644 libm.map        $INSTALL_DIR/target/usr/lib/libm_pic.map

install -m 644 resolv/libresolv_pic.a $INSTALL_DIR/target/usr/lib/.
install -m 644 libresolv.map   $INSTALL_DIR/target/usr/lib/libresolv_pic.map

install -m 644 nptl/libpthread_pic.a $INSTALL_DIR/target/usr/lib/
install -m 644 libpthread.map $INSTALL_DIR/target/usr/lib/libpthread_pic.map

# Set Library Optimizer variables
OBJECTDIR=`/bin/pwd`

echo "●SELP: Install Step - Install Library Optimizer information (libc) ( using install command)"
# Install Library Optimizer information (libc)
install -d -m 755 $INSTALL_DIR/target/usr/lib/optinfo/libc
sed -e "s|@CFLAGS@||g" -e "s|@LIBDIR@|lib|g"  -e "s|@TLSDIR@||g" $RPM_BUILD_DIR/glibc-libopt-build-libc > build.libc
install -m 755 build.libc $INSTALL_DIR/target/usr/lib/optinfo/libc/build
install -m 644 libc.map $INSTALL_DIR/target/usr/lib/optinfo/libc/libc.map
install -m 644 shlib.lds $INSTALL_DIR/target/usr/lib/optinfo/libc/libc.so.lds
install -m 644 csu/abi-note.o $INSTALL_DIR/target/usr/lib/optinfo/libc/
[ -f csu/numa-note.o ] && install -m 644 csu/numa-note.o $INSTALL_DIR/target/usr/lib/optinfo/libc/
install -m 644 elf/soinit.os $INSTALL_DIR/target/usr/lib/optinfo/libc/soinit.o
install -m 644 elf/sofini.os $INSTALL_DIR/target/usr/lib/optinfo/libc/sofini.o
install -m 644 elf/interp.os $INSTALL_DIR/target/usr/lib/optinfo/libc/interp.o
(cd $INSTALL_DIR/target/usr/lib/optinfo/libc; umask 022; arm_v7_vfp_le-ar x $OBJECTDIR/libc_pic.a)
mv $INSTALL_DIR/target/usr/lib/optinfo/libc/version.os $INSTALL_DIR/target/usr/lib/optinfo/libc/version.o
(cd $INSTALL_DIR/target/usr/lib/optinfo/libc; umask 022; arm_v7_vfp_le-libindex $OBJECTDIR/libc.so -r *.o -o *.os > index)
(umask 022; echo "/lib/libc-"2.5.90".so" > $INSTALL_DIR/target/usr/lib/optinfo/libc/path)

echo "●SELP: Install Step - Install Library Optimizer information (libm) ( using install command)"
# Install Library Optimizer information (libm)
install -d -m 755 $INSTALL_DIR/target/usr/lib/optinfo/libm
sed -e "s|@CFLAGS@||g" -e "s|@LIBDIR@|lib|g"  -e "s|@TLSDIR@||g" $RPM_BUILD_DIR/glibc-libopt-build-libm > build.libm
install -m 755 build.libm $INSTALL_DIR/target/usr/lib/optinfo/libm/build
install -m 644 libm.map $INSTALL_DIR/target/usr/lib/optinfo/libm/libm.map
install -m 644 shlib.lds $INSTALL_DIR/target/usr/lib/optinfo/libm/libm.so.lds
install -m 644 csu/abi-note.o $INSTALL_DIR/target/usr/lib/optinfo/libm/
[ -f csu/numa-note.o ] && install -m 644 csu/numa-note.o $INSTALL_DIR/target/usr/lib/optinfo/libc/
install -m 644 elf/interp.os $INSTALL_DIR/target/usr/lib/optinfo/libm/interp.o
install -m 644 libc_nonshared.a $INSTALL_DIR/target/usr/lib/optinfo/libm/libc_nonshared.a
(cd $INSTALL_DIR/target/usr/lib/optinfo/libm; umask 022; arm_v7_vfp_le-ar x $OBJECTDIR/math/libm_pic.a)
(cd $INSTALL_DIR/target/usr/lib/optinfo/libm; umask 022; arm_v7_vfp_le-libindex $OBJECTDIR/math/libm.so -r *.o -o *.os > index)
(umask 022; echo "/lib/libm-"2.5.90".so" > $INSTALL_DIR/target/usr/lib/optinfo/libm/path)

mkdir -p $INSTALL_DIR/target/etc
mv $INSTALL_DIR/target/usr/share/locale/locale.alias $INSTALL_DIR/target/etc/locale.alias
ln -s ../../../etc/locale.alias $INSTALL_DIR/target/usr/share/locale/locale.alias

cd ..


install -m 644 nscd/nscd.conf $INSTALL_DIR/target/etc/nscd.conf


# RPM is on crack..  We need to "prepare" the doc files..
cp -pr nptl/ChangeLog ChangeLog.nptl
cp -pr localedata/ChangeLog ChangeLog.localedata
cp -pr localedata/README README.localedata

# gzip -9 $INSTALL_DIR/target/usr/share/man/man*/*
gzip -9 $INSTALL_DIR/target/usr/share/info/*

mv $INSTALL_DIR/target/usr/sbin/zdump $INSTALL_DIR/target/usr/bin/
mv $INSTALL_DIR/target/usr/sbin/rpcinfo $INSTALL_DIR/target/usr/bin/

# Get manual files generated by makeinfo
mkdir -p html
cp manual/libc/*.html html/


# Make sure we always have .../target/usr/lib (needed for MIPS64)
mkdir -p $INSTALL_DIR/target/usr/lib

# Don't install an info directory file; should be built in common for all pkgs.
rm -f $INSTALL_DIR/target/usr/share/info/dir.gz

# rquota.x and rquota.h are now provided by quota
rm -f $INSTALL_DIR/target/usr/include/rpcsvc/rquota.[hx]

# The pt_chown program is only required for kernels that are not
# configured to use /dev/pts.  All modern kernels can use
# /dev/pts, and it is better to use that technique, so we believe
# that shipping pt_chown is unncessary.  Furthermore, since it is
# supposed to be an suid binary, it's confusing to ship it
# not-suid.  Shipping it suid does not work if the the user
# unpacking the file is not root.  So, we just remove the file.

rm -f $INSTALL_DIR/target/usr/lib/pt_chown


cp -rp objdir/cross-output $INSTALL_DIR/target/usr/include/cross-output

# package the testsuites
# include the MV testsuite driver snippet.

cat >> test-glibc.sh << EOF
#!/usr/bin/env bash
umask 0
pushd /testsuite/glibc/objdir
glibc_source=/testsuite/glibc/glibc-2.5.90
libdir=lib
export CFLAGS='-g -O2 -fgnu89-inline'
\$glibc_source/configure --host=armv7fl-montavista-linux-gnueabi --enable-add-ons --prefix=/usr
for i in \$glibc_source/*/Makefile; do
  mkdir -p \$(basename \$(dirname \$i))
done

ln -sf /usr/\$libdir/libc_nonshared.a libc_nonshared.a
ln -sf /usr/\$libdir/libc.a libc.a
ln -sf /usr/\$libdir/libc_pic.a libc_pic.a
old="libc_nonshared.a libc.a libc_pic.a"

ln -sf \`ls /\$libdir/libc.so.*\` libc.so
ln -sf \`ls /\$libdir/libc.so.*\` .
old="\$old libc.so"

ln -sf \`ls /\$libdir/ld*.so.*\` elf/
ln -sf \`ls /\$libdir/ld-2.5.90.so\` elf/ld.so
ln -sf /sbin/ldconfig elf/
ln -sf /usr/bin/ldd elf/
old="\$old elf/ld.so"

ln -sf \`ls /\$libdir/libdl.so.*\` dlfcn/
ln -sf \`ls /\$libdir/libdl.so.*\` dlfcn/libdl.so
old="\$old dlfcn/libdl.so"

ln -sf \`ls /\$libdir/libpthread.so.*\` nptl/
ln -sf \`ls /\$libdir/libpthread.so.*\` nptl/libpthread.so
ln -sf /usr/\$libdir/libpthread.a nptl/
ln -sf /usr/\$libdir/libpthread_pic.a nptl/
ln -sf /usr/\$libdir/libpthread_nonshared.a nptl/
old="\$old nptl/libpthread.so nptl/libpthread.a"
old="\$old nptl/libpthread_nonshared.a nptl/libpthread_pic.a"

ln -sf \`ls /\$libdir/librt.so.*\` rt/
ln -sf \`ls /\$libdir/librt.so.*\` rt/librt.so
old="\$old rt/librt.so"

ln -sf \`ls /\$libdir/libm.so.*\` math/
ln -sf \`ls /\$libdir/libm.so.*\` math/libm.so
old="\$old math/libm.so"
ln -sf \`ls /\$libdir/libresolv.so.*\` resolv/
ln -sf \`ls /\$libdir/libresolv.so.*\` resolv/libresolv.so
ln -sf \`ls /\$libdir/libanl.so.*\` resolv/
ln -sf \`ls /\$libdir/libanl.so.*\` resolv/libanl.so
old="\$old resolv/libresolv.so resolv/libanl.so"
ln -sf \`ls /\$libdir/libcrypt.so.*\` crypt/
ln -sf \`ls /\$libdir/libcrypt.so.*\` crypt/libcrypt.so
old="\$old crypt/libcrypt.so"

for file in /usr/\$libdir/*crt*.o; do
  ln -sf \$file csu/
done

ln -sf /usr/bin/localedef locale/
ln -sf /usr/bin/mtrace malloc/
ln -sf /usr/bin/iconv iconv/iconv_prog

for i in /usr/\$libdir/gconv/*; do ln -sf \$i iconvdata/\`basename \$i\`; done
old="\$old \`echo -n iconvdata/*.so\`"

echo "●SELP: Install Step - Linking test shared libraries wants this. (Running make)"
# Linking test shared libraries wants this.
make no_deps=t objdir=\$PWD -C \$glibc_source/csu \$PWD/csu/abi-note.o

echo "●SELP: Install Step - For math/atest-exp (Running make)"
# For math/atest-exp
for i in add_n sub_n cmp addmul_1 mul_1 mul_n divmod_1 lshift rshift \
        mp_clz_tab udiv_qrnnd inlines
do
        make objdir=\$PWD -C \$glibc_source/stdlib \$PWD/stdlib/\$i.o no_deps=t
done

echo "●SELP: Install Step - For dlfcn/tststatic (Running make)"
# For dlfcn/tststatic
make objdir=\$PWD -C \$glibc_source/csu \$PWD/csu/start.o no_deps=t

# Make check will rebuild headers in iconvdata/; it's harmless and doesn't
# take too long.

for i in \$old; do
  oldopt="\$oldopt -o \$PWD/\$i"
done

set +e

echo "●SELP: Install Step - The localedata tests will do this for us.(Running make)"
# Catgets requires the de_DE.ISO-8859-1 locale to be generated before
# it runs.  The localedata tests will do this for us...
make MAKE="make \$oldopt" no_deps=t objdir=\$PWD -C \$glibc_source -k localedata/tests
make MAKE="make \$oldopt" no_deps=t objdir=\$PWD -C \$glibc_source -k check

popd

#exit 0

EOF

cat >> mv-test-native-glibc.sh << EOF
#!/usr/bin/env bash
pushd /testsuite/glibc

./test-glibc.sh 2>&1 |tee result.log

cat result.log |grep Error 2>&1 |tee fail.log

popd

EOF

install -d $INSTALL_DIR/target/testsuite/glibc
install -d $INSTALL_DIR/target/testsuite/glibc/objdir
install -d $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90
cp -pr * $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90
rm -rf $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90/objdir/
rm -rf $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90/LOCALOBJDIR
rm -rf $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90/localedef
rm -rf $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90/cross*
rm -rf $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90/test-glibc.sh
rm -rf $INSTALL_DIR/target/testsuite/glibc/glibc-2.5.90/mv-test-native-glibc.sh
install -m 755 mv-test-native-glibc.sh $INSTALL_DIR/target/testsuite/glibc
install -m 755 test-glibc.sh $INSTALL_DIR/target/testsuite/glibc

#make a link of mv-test-native-glibc.sh to testglibc
#ln -s $INSTALL_DIR/target/testsuite/glibc/mv-test-native-glibc.sh $INSTALL_DIR/target/usr/bin/testglibc
ln -s ../../testsuite/glibc/mv-test-native-glibc.sh $INSTALL_DIR/target/usr/bin/testglibc

echo Building debuginfo subpackage...

blf=debugfiles.list
sf=debugsources.list

echo -n > $sf

debugdir="${RPM_BUILD_ROOT}$INSTALL_DIR/target/usr/lib/debug"


echo "●SELP: Install Step - Create filelist for stripping (using find command)"
# Create filelist for stripping
find ${RPM_BUILD_ROOT}$INSTALL_DIR/target -path "${debugdir}" \
        -prune -o -type f \( -perm -0100 -or -perm \
        -0010 -or -perm -0001 \) -exec file {} \; | \
        sed -n -e 's/^\(.*\):[  ]*.*ELF.* executable.*, not stripped/\1/p' | sed -e 's?//*?/?g' > installed_file.list
  
find ${RPM_BUILD_ROOT}$INSTALL_DIR/target -path "${debugdir}" \
        -prune -o -type f \( -perm -0100 -or -perm \
        -0010 -or -perm -0001 \) -exec file {} \; | \
        sed -n -e 's/^\(.*\):[  ]*.*ELF.* shared object.*, not stripped/\1/p' | sed -e 's?//*?/?g' > installed_file_so.list

echo "●SELP: Install Step - Run standard debuginfo scripts (using find command)"
# Run standard debuginfo scripts
for f in `cat installed_file.list installed_file_so.list`
do
        dn=$(dirname $f | sed -n -e "s#^$RPM_BUILD_ROOT##p" | sed -n -e "s#/$INSTALL_DIR/target##p")
        bn=$(basename $f .debug).debug

        debugdn="${debugdir}${dn}"
        debugfn="${debugdn}/${bn}"
        [ -f "${debugfn}" ] && continue
  
        mkdir -p "${debugdn}"
 
        # Extract debugging information into a separate file.
        arm_v7_vfp_le-objcopy --only-keep-debug "$f" "${debugfn}" || :
        arm_v7_vfp_le-strip --strip-debug "$f" || :
        arm_v7_vfp_le-objcopy --add-gnu-debuglink="${debugfn}" "$f" || :

done

mkdir -p ${RPM_BUILD_ROOT}$INSTALL_DIR/target/usr/src/debug
cat $sf | (cd $RPM_BUILD_DIR; LANG=C sort -z -u | cpio -pd0m ${RPM_BUILD_ROOT}/$INSTALL_DIR/target/usr/src/debug)
# stupid cpio creates new directories in mode 0700, fixup
find ${RPM_BUILD_ROOT}$INSTALL_DIR/target/usr/src/debug -type d -print0 | xargs -0 chmod a+rx

find ${RPM_BUILD_ROOT}$INSTALL_DIR/target/usr/lib/debug -type f | sed -n -e "s#^$RPM_BUILD_ROOT##p" > $blf
find ${RPM_BUILD_ROOT}$INSTALL_DIR/target/usr/src/debug -mindepth 1 -maxdepth 1 | sed -n -e "s#^$RPM_BUILD_ROOT##p" >> $blf


####################
#  CLEAN
#

echo "----<SELP glibc Builder>---------------------------------"
echo " Congurations! glibc building task finsihed succesfully."
echo " Installed Directory is $INSTALL_DIR ."
echo -e "\t * START TIME: $StartTime"
echo -e "\t * END   TIME: $EndTime"
echo "---------------------------------------------------------"

#exit 0
