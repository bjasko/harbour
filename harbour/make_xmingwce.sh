#!/bin/sh
[ "$BASH" ] || exec bash `which $0` ${1+"$@"}
#
# $Id$
#
# This script simplifies cross-compiling Harbour for Windows-CE from Unix systems.
#
# Copyright 2007 by Przemyslaw Czerpak (druzus/at/priv.onet.pl)
#

cleanup()
{
    rm -fR "${HB_BIN_COMPILE}"
}

UNAME=`uname`
UNAMEL=`echo "$UNAME"|tr A-Z a-z`
UNAMEU=`echo "$UNAME"|tr a-z A-Z`

export HB_ARCHITECTURE=w32
export HB_COMPILER=mingwce

if [ "$OSTYPE" = "msdosdjgpp" ]; then
    HB_HOST_ARCH="dos"
    HB_HOST_CC="djgpp"
else
    HB_HOST_ARCH="${UNAMEL}"
    HB_HOST_CC="gcc"
    case "$HB_HOST_ARCH" in
        *windows*|*mingw32*|msys*)  HB_HOST_ARCH="w32"; HB_HOST_CC="mingw" ;;
        *dos)    HB_HOST_ARCH="dos" ;;
        *bsd)    HB_HOST_ARCH="bsd" ;;
        *darwin) HB_HOST_ARCH="darwin" ;;
        *SunOS)  HB_HOST_ARCH="sunos" ;;
    esac
fi

CC_PRG_USR="-D__PLATFORM__WINCE"
if [ "$HB_HOST_ARCH" != "w32" ]; then
    CC_PRG_USR="$CC_PRG_USR -D__PLATFORM__WINDOWS -undef:__PLATFORM__UNIX -undef:__PLATFORM__$UNAMEU"
fi

[ -z "$HB_INSTALL_PREFIX" ] && \
export HB_INSTALL_PREFIX="/usr/local/arm-wince-mingwce-harbour"
export CC_C_USR=""
export C_USR="$CC_C_USR $C_USR"
export CC_PRG_USR
export PRG_USR="$CC_PRG_USR $PRG_USR"

# default cegcc instalation path
[ -z "$CCPATH" ] && CCPATH="/opt/mingw32ce/bin"
export PATH="$CCPATH:$PATH"

# cegcc executables prefix - this
# has changed in cegcc/gcc4.3.0
if [ -z "$CCPREFIX" ]; then
    if [ -x "${CCPATH}/arm-wince-mingw32ce-gcc" ]; then
        export CCPREFIX="arm-wince-mingw32ce-"
    else
        if [ -x "${CCPATH}/arm-mingw32ce-gcc" ]; then
            export CCPREFIX="arm-mingw32ce-"
        else
            echo "cegcc compiler executable not found. Ensure you have cegcc package installed in"
            echo "/opt/mingw32ce dir, or (alternatively) set environment variable CCPATH to a cegcc"
            echo "installation directory"
            exit 1
        fi
    fi
fi

export CCPATH="${CCPATH}:"
export HB_TOOLS_PREF="hbce"
export HB_XBUILD="wce"
[ "${HB_HOST_BUILD}" = "all" ] || export HB_HOST_BUILD="lib"
export HB_GT_LIB="gtwvt"

export HB_BIN_COMPILE="/tmp/hb-${CCPREFIX}-$$"
rm -fR "${HB_BIN_COMPILE}"
trap cleanup EXIT >/dev/null 2>&1
mkdir ${HB_BIN_COMPILE}

DIR=`cd $(dirname $0);pwd`
if [ -z "${HB_COMP_PATH}" ]; then
    if which harbour > /dev/null 2>&1; then
        HB_COMP_PATH=`which harbour 2> /dev/null`
    else
        HB_COMP_PATH="$DIR/source/main/$HB_HOST_ARCH/$HB_HOST_CC/harbour"
    fi
fi

if [ -x "${HB_COMP_PATH}" ]; then
    ln -s "${HB_COMP_PATH}" ${HB_BIN_COMPILE}/harbour.exe
else
    echo "You must have a working 'harbour' executable for your platform on your PATH."
    exit 1
fi

if [ -z "${HB_PPGEN_PATH}" ]; then
    if which hbpp &> /dev/null; then
        HB_PPGEN_PATH=`which hbpp 2> /dev/null`
    elif [ -x "${DIR}/source/pp/${HB_HOST_ARCH}/${HB_HOST_CC}/hbpp" ]; then
        HB_PPGEN_PATH="${DIR}/source/pp/${HB_HOST_ARCH}/${HB_HOST_CC}/hbpp"
    else
        DIR=`dirname ${HB_COMP_PATH}`
        if [ -x "${DIR}/hbpp" ]; then
            HB_PPGEN_PATH="${DIR}/hbpp"
        else
            HB_PPGEN_PATH="$DIR/source/pp/$HB_HOST_ARCH/$HB_HOST_CC/hbpp"
        fi
    fi
fi
if [ -d "${HB_PPGEN_PATH}" ]; then
   if [ -x "${HB_PPGEN_PATH}/hbpp" ]; then
      HB_PPGEN_PATH="${HB_PPGEN_PATH}/hbpp"
   fi
fi
if [ -x "${HB_PPGEN_PATH}" ] && [ -f "${HB_PPGEN_PATH}" ]; then
    ln -s ${HB_PPGEN_PATH} ${HB_BIN_COMPILE}/hbpp.exe
    HB_PPGEN_PATH="${HB_BIN_COMPILE}"
else
    echo "You must have a working 'hbpp' executable for your platform on your PATH."
    exit 1
fi
export HB_PPGEN_PATH

case "$1" in
    tgz|gnu)
        ext=$1
        shift
        . `dirname $0`/make_${ext}.sh "$@"
        ;;
    *)
        . `dirname $0`/make_gnu.sh "$@"
        ;;
esac

stat="$?"
cleanup
exit "${stat}"
