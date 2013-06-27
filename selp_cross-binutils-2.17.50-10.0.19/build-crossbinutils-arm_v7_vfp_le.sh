#!/bin/bash
#              SELP cross-binutils Auto-Build Script
# ==============================================
#                         (2009.04.13)
# This script was originally generated from mvl-edition-rpmbuild
# and edited using exit 1 , exit 0 intercept command at %build line of *.spec file.
#
# @Requirements:
#  . Cross Compiler    : SELP Cross Toolchain
#  . Host PC's Utility : gmake 3.80 , sed 4.1.5 , autoconf 2.59 , gawk 3.1.5 ,
#                        perl 5.8.8 , bash 3.1.7 , libtool 1.5.8
#  . Estimated Time    : 4min 30secs
#  . Dependant Utility : makeinfo 4.8
#
# @Caution:
#  1. Confirm  permission 777  of /tmp directory in your linux pc.
#
# @History:
# 2008.01.02 Lim,GeunSik Created initial binutils build script code for binutils 2.17.50
# 2008.01.02 Lim,GeunSik Resolved path finding of binutils binaries.
# 2008.01.02 Lim,GeunSik Remove "objdir" directory for successfull compiliation again at next time when build binutils source.
# 2008.01.03 Lim,GeunSik Appended Exception Handling to monitor abnormal result of command.
# 2008.01.03 Lim,GeunSik Changed variable from $INSTALL_DIR to ${INSTALL_DIR} at "make bfdincludedir=${INSTALL_DIR}/include" line.
# 2008.01.03 Lim,GeunSik You have to use fedora core 6 for compatibility of makinfo ver 4.8. But If you are using another distribution,
# Appended "PATH=/opt/mvl.5.0.pro.arm.v5t.le/common/bin/:******" setting to use makeinfo 4.8(GNU texinfo translation).
# 2008.01.05 Lim,GeunSik Changed toolchain naming from arm_v6_vfp_le-*** to selp-arm_v6_vfp_le-*** for consistency.
# 2009.04.13 Lee,Hakbong migration to Moblinux.5.0.24 arm_v7_vfp_le, add -fshort-wchar option
#

StartTime="`date '+%Y-%m-%d(%H:%M)'`"


if [ -z "$1" ]
then
  echo ""
  echo "----arm_v7_vfp_le-------------------------------- "
  echo "cross-binutils AutoBuilder 2.0 (SELP) 2009.11.17 "
  echo "------------------------------------------------- "
  echo "Usage : `basename $0` absolute-dir-to-install(ex:/home/yourid/cross.binutils.1010)"
  echo ""
  echo ""
  exit 1
fi

INSTALL_DIR=$1
export INSTALL_DIR

RPM_BUILD_DIR=`pwd`
RPM_OPT_FLAGS="-O2"
RPM_ARCH="arm_v7_vfp_le"
RPM_OS="linux"
export RPM_BUILD_DIR RPM_OPT_FLAGS RPM_ARCH RPM_OS

RPM_PACKAGE_NAME="cross-binutils"
RPM_PACKAGE_VERSION="2.17.50"
RPM_PACKAGE_RELEASE="10.0.19.custom"
export RPM_PACKAGE_NAME RPM_PACKAGE_VERSION RPM_PACKAGE_RELEASE
  
set -x
umask 022
cd $RPM_BUILD_DIR
LANG=C
export LANG

# Caution: You have to append "/opt/mvl.5.0.pro.arm.v5t.le/common/bin/" directory  like below PATH 
# for translating textinfo(bfd/info) successfully.
# Appended "PATH=/opt/mvl.5.0.pro.arm.v5t.le/common/bin/:******" setting to use makeinfo.
#PATH=/opt/mvl.5.0.pro.arm.v5t.le/common/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X/bin:/usr/X11R6/bin:/usr/ucb
#export PATH


######
# Configure

pushd ../../arm_v7_vfp_le/
CROSS_COMPILE_DIR=`pwd`
## MV package dependent for montavista testsuite shell script ##
##MV_CROSS_COMPILE_DIR=/opt/mvl.5024.mobil.arm.v7.vfp.le.omap3530/mobilinux/devkit/arm/v7_vfp_le
##CROSS_COMPILE_DIR=$MV_CROSS_COMPILE_DIR

export CROSS_COMPILE_DIR
popd

if [[ -d $INSTALL_DIR/$RPM_ARCH ]]; then  rm -rf $INSTALL_DIR/$RPM_ARCH;fi
mkdir -p $INSTALL_DIR/$RPM_ARCH

if [[ -d ./objdir ]]; then  rm -rf ./objdir;fi
mkdir -p objdir
cd objdir

## add -fshort-wchar gcc option to prevent "Conflicting wchar_t type static link error"
export CFLAGS="-fshort-wchar -g -O2"

CFLAGS="${CFLAGS:--O2}" ; export CFLAGS ; 
CXXFLAGS="${CXXFLAGS:--O2}" ; export CXXFLAGS ; 
FFLAGS="${FFLAGS:--O2}" ; export FFLAGS ; 
[ -f oonfigure.in ] && libtoolize --copy --force ; 
../configure --host=i686-pc-linux-gnu \
	--build=i686-pc-linux-gnu \
	--target=armv7fl-montavista-linux-gnueabi \
        --prefix=$CROSS_COMPILE_DIR \
        --exec-prefix=$CROSS_COMPILE_DIR \
        --bindir=$CROSS_COMPILE_DIR/bin \
        --sbindir=$CROSS_COMPILE_DIR/sbin \
        --sysconfdir=$CROSS_COMPILE_DIR/etc \
        --datadir=$CROSS_COMPILE_DIR/share \
        --includedir=$CROSS_COMPILE_DIR/include \
        --libdir=$CROSS_COMPILE_DIR/lib \
        --libexecdir=$CROSS_COMPILE_DIR/libexec \
        --localstatedir=$CROSS_COMPILE_DIR/var \
        --sharedstatedir=$CROSS_COMPILE_DIR/share \
        --mandir=$CROSS_COMPILE_DIR/man \
        --infodir=$CROSS_COMPILE_DIR/info --program-prefix=arm_v7_vfp_le- \
	--with-sysroot=$CROSS_COMPILE_DIR/target \
	--enable-install-libbfd \
	--disable-werror \
	2>&1
if [ $? != 0 ]; then echo "SELP: cross-binutils Error Occurred while configure task "; exit 1 ;fi


# Gprof isn't built or installed by default.  Give it a push.
make -j 1 all 2>&1
if [ $? != 0 ]; then echo "SELP: cross-binutils Error Occurred while make all task "; exit 1 ;fi

make -j 1 all-gprof 2>&1
if [ $? != 0 ]; then echo "SELP: cross-binutils Error Occurred while make all-gprof task "; exit 1 ;fi

#
# INSTALL
#
  
set -x
umask 022
cd $RPM_BUILD_DIR

rm -rf ${INSTALL_DIR}
cp COPYING LICENSE
cd objdir

   make \
        prefix=${INSTALL_DIR} \
        exec_prefix=${INSTALL_DIR} \
        bindir=${INSTALL_DIR}/bin \
        sbindir=${INSTALL_DIR}/sbin \
        sysconfdir=${INSTALL_DIR}/etc \
        datadir=${INSTALL_DIR}/share \
        includedir=${INSTALL_DIR}/include \
        libdir=${INSTALL_DIR}/lib \
        libexecdir=${INSTALL_DIR}/libexec \
        localstatedir=${INSTALL_DIR}/var \
        sharedstatedir=${INSTALL_DIR}/share \
        mandir=${INSTALL_DIR}/man \
        infodir=${INSTALL_DIR}/info \
   install \
	MAKE='make bfdincludedir=${INSTALL_DIR}/include \
		bfdlibdir=${INSTALL_DIR}/lib' \
	install-gprof \
	install-info 2>&1
if [ $? != 0 ]; then echo "SELP: cross-binutils Error Occurred while make install task "; exit 1 ;fi

# ppc speficif script to embed spu code in ppc ELF objects 
# we dont support SPU so do not need it.

# GCC's collect2 looks for these.  It will find the non-'g' versions
# but adding these prevents a search in $PATH.
ln -s strip \
	${INSTALL_DIR}/armv7fl-montavista-linux-gnueabi/bin/gstrip
ln -s nm \
	${INSTALL_DIR}/armv7fl-montavista-linux-gnueabi/bin/gnm

# Replace some hardlinks with symlinks.  The symlinks are friendlier to
# relocation.
for prog in ar as ld nm objdump ranlib strip; do
  rm -f ${INSTALL_DIR}/armv7fl-montavista-linux-gnueabi/bin/${prog}
  ln -s ../../bin/arm_v7_vfp_le-${prog} ${INSTALL_DIR}/armv7fl-montavista-linux-gnueabi/bin/${prog}
done

install  ../include/dis-asm.h ${INSTALL_DIR}/include

# Remove unnecessary info pages
rm ${INSTALL_DIR}/info/dir
rm ${INSTALL_DIR}/info/configure.*
rm ${INSTALL_DIR}/info/standards.*

rm ${INSTALL_DIR}/man/man1/arm_v7_vfp_le-dlltool.1
rm ${INSTALL_DIR}/man/man1/arm_v7_vfp_le-windres.1

# install the testsuite drivers
# include the dejaGNU script file generation piece.
# this file generates the GDejaGNU drivers for GCC cross testing.

local_target_alias=`echo arm_v7_vfp_le- | sed 's/-//'`
# this variable is a magic to extract devkit/$arch/$subarch as it
# appears in the install path of the architecture.
## MV package dependent for montavista testsuite shell script ##
local_target_path_suffix=`echo $CROSS_COMPILE_DIR | awk 'BEGIN { FS = "/" };{ print $(NF-2) "/" $(NF-1) "/" $NF }'`

# Generate remote-site.exp
cat >> site.exp << EOF
#!/usr/bin/env expect
set myboard armv7fl-montavista-linux-gnueabi
set target_list [list armv7fl-montavista-linux-gnueabi]
set target_alias    ${local_target_alias}
set CC             \$target_alias-gcc
set CXX            \$target_alias-g++
set GCC_UNDER_TEST \$CC
set GXX_UNDER_TEST \$CXX
set cxxfilt        \$target_alias-c++filt
set LD             \$target_alias-ld
set AR             \$target_alias-ar
set AS             \$target_alias-as
set NM             \$target_alias-nm
set OBJCOPY        \$target_alias-objcopy
set OBJDUMP        \$target_alias-objdump
set READELF        \$target_alias-readelf
set STRIP          \$target_alias-strip
set HOSTCC	   gcc
set HOSTCFLAGS     ""
set libiconv       ""
set builddir       [pwd]
EOF

#Generate testboard.exp
cat >> testboard.exp << EOF
#!/usr/bin/env expect
load_generic_config            "unix"
set_board_info rsh_prog        /usr/bin/ssh
set_board_info rcp_prog        /usr/bin/scp
set_board_info hostname        \$TARGETIP
set_board_info username        \$REMOTEUSER
set_board_info shell_prompt    "arm_v7_vfp_le-target:> "
EOF

#Generate mv-test-cross-binutils.sh
cat >> mv-test-cross-binutils.sh << EOF
#!/usr/bin/env bash
umask 0
pushd \`mvl-whereami\`/../$local_target_path_suffix/testsuite/binutils
usage()
{
    echo "

Description:
    Script to run cross binutils dejaGNU tests.

Usage:

     mv-test-cross-binutils.sh <target IP>  <target user name>

Examples:

     > mv-test-cross-binuitls.sh remote.example.com tester
       It will run the binutils tests on remote.example.com 
       with user 'tester'.
"
}

case \$1 in
    --help | -h)
        usage
        exit 0
        ;;
    *)
        ;;
esac


if [ \$# -ne 2 ]; then
    usage
    exit 0
fi

mkdir -p log
mkdir -p tmp
export DEJAGNU=\$PWD/site.exp
pushd \$PWD/testsuite
runtest TARGETIP=\$1 REMOTEUSER=\$2 --tool gas --all \\
--outdir \$PWD/../log --srcdir \$PWD/gas \\
--target_board=testboard --target armv7fl-montavista-linux-gnueabi \\
-v
runtest TARGETIP=\$1 REMOTEUSER=\$2 --tool binutils --all \\
--outdir \$PWD/../log --srcdir \$PWD/binutils \\
--target_board=testboard --target armv7fl-montavista-linux-gnueabi \\
-v
runtest TARGETIP=\$1 REMOTEUSER=\$2 --tool ld --all \\
--outdir \$PWD/../log --srcdir \$PWD/ld \\
--target_board=testboard --target armv7fl-montavista-linux-gnueabi \\
-v
popd
echo "The test log files are in log."
popd
exit 0
EOF

install -d ${INSTALL_DIR}/testsuite/binutils/testsuite
install -d ${INSTALL_DIR}/testsuite/binutils/testsuite/binutils
install -d ${INSTALL_DIR}/testsuite/binutils/testsuite/gas
install -d ${INSTALL_DIR}/testsuite/binutils/testsuite/ld
install -d ${INSTALL_DIR}/testsuite/binutils/boards
cp -pr ../binutils/testsuite/* ${INSTALL_DIR}/testsuite/binutils/testsuite/binutils/
cp -pr ../gas/testsuite/* ${INSTALL_DIR}/testsuite/binutils/testsuite/gas/
cp -pr ../ld/testsuite/* ${INSTALL_DIR}/testsuite/binutils/testsuite/ld/
cp -pr ../ld/configure.tgt ${INSTALL_DIR}/testsuite/binutils/testsuite
cp -pr ../ld/configure.host ${INSTALL_DIR}/testsuite/binutils/testsuite

install -m 755 mv-test-cross-binutils.sh ${INSTALL_DIR}/testsuite/binutils
install -m 755 site.exp ${INSTALL_DIR}/testsuite/binutils
install -m 755 testboard.exp ${INSTALL_DIR}/testsuite/binutils/boards
#make a link of mv-test-cross-binutils.sh to <cross>-testbinutils
pushd ${INSTALL_DIR}/bin
ln -s ../testsuite/binutils/mv-test-cross-binutils.sh arm_v7_vfp_le-testbinutils
popd
#
# CLEAN
#

# /opt/mvl.5.0.pro.arm.v5t.le/common/lib/rpm/find-debuginfo.sh /home/invain/montavista/rpm/BUILD/selp_cross-binutils-2.17.50-5.0.19 $CROSS_COMPILE_DIR/target selp-arm_v6_vfp_le-

EndTime="`date '+%Y-%m-%d(%H:%M)'`"

echo -en "\a"; sleep 1
echo -en "\a"; sleep 1
echo -en "\a"; sleep 1
echo -en "\a"; sleep 1
echo -en "\a"; sleep 1


echo " -------------------------------------------------------------" 
echo " SELP: binutils (selp-arm) is compiled successfully.~~~ "
echo " -Installed Directory : ${INSTALL_DIR} "
echo " -Caution: If 'objdir' directory is existed in this directory, "
echo "           remove for successfull compiling at next time."
echo -e "\t * START TIME: $StartTime"
echo -e "\t * END   TIME: $EndTime"
echo " -------------------------------------------------------------" 


exit 0
