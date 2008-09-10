#!/bin/sh

#
# $Id$
#

# ---------------------------------------------------------------
# Partly based on harbour/make_gcc.sh by :
# Copyright 2007 Przemyslaw Czerpak (druzus/at/priv.onet.pl),
# See doc/license.txt for licensing terms.
# ---------------------------------------------------------------

if [ -z "$HB_ARCHITECTURE" ]; then
   if [ "$OSTYPE" = "msdosdjgpp" ]; then
      hb_arch="dos"
   else
      hb_arch=`uname -s | tr -d "[-]" | tr '[A-Z]' '[a-z]' 2>/dev/null`
      case "$hb_arch" in
         *windows*|*mingw32*|msys*)   hb_arch="w32" ;;
         *cygwin*)                    hb_arch="cyg" ;;
         *dos)                        hb_arch="dos" ;;
         *bsd)                        hb_arch="bsd" ;;
         *darwin)                     hb_arch="darwin" ;;
      esac
   fi
   export HB_ARCHITECTURE="$hb_arch" _HB_ARCH_SAVED=1
fi

#**************************************************************

if [ -z "$HB_CC_NAME" ]; then
   case "$HB_ARCHITECTURE" in
      w32) HB_CC_NAME="mingw" ;;
      dos) HB_CC_NAME="djgpp" ;;
      *)   HB_CC_NAME="gcc" ;;
   esac
   export HB_CC_NAME _HB_CC_NAME_SAVED=1
fi

#**************************************************************

_HB_DIRS="hbbmcdx hbbtree hbclipsm hbct hbgt hbmisc hbmsql hbmzip hbnf hbtip hbsqlit3 hbtpathy hbvpdf hbziparc xhb"

case "$HB_ARCHITECTURE" in
   w32|cyg|os2)
        _HB_DIRS_ADD="gtwvg hbole hbodbc hbw32 hbwhat rddado"
        ;;
   *)
        _HB_DIRS_ADD=;;
esac

if [ "${HB_INC_ALLEGRO}"   != "" ]; then _HB_DIRS="${_HB_DIRS} gtalleg"; fi;
if [ "${HB_INC_APOLLO}"    != "" ]; then _HB_DIRS="${_HB_DIRS} hbapollo"; fi;
if [ "${HB_INC_CURL}"      != "" ]; then _HB_DIRS="${_HB_DIRS} hbcurl  "; fi;
if [ "${HB_INC_FIREBIRD}"  != "" ]; then _HB_DIRS="${_HB_DIRS} hbfbird "; fi;
if [ "${HB_INC_FREEIMAGE}" != "" ]; then _HB_DIRS="${_HB_DIRS} hbfimage"; fi;
if [ "${HB_INC_GD}"        != "" ]; then _HB_DIRS="${_HB_DIRS} hbgd    "; fi;
if [ "${HB_INC_LIBHARU}"   != "" ]; then _HB_DIRS="${_HB_DIRS} hbhpdf  "; fi;
if [ "${HB_INC_MYSQL}"     != "" ]; then _HB_DIRS="${_HB_DIRS} hbmysql "; fi;
if [ "${HB_INC_PGSQL}"     != "" ]; then _HB_DIRS="${_HB_DIRS} hbpgsql "; fi;
if [ "${HB_INC_ADS}"       != "" ]; then _HB_DIRS="${_HB_DIRS} rddads  "; fi;

_HB_DIRS="${_HB_DIRS} ${_HB_DIRS_ADD}"

#**************************************************************

for n in ${_HB_DIRS}; do
  if [ -d $n ]; then
    echo Entering $n ... \( $1 $2 $3 $4 $5\)
    cd $n
    [ -f ./make_gcc.sh ] && ${SHELL} -c "./make_gcc.sh $1 $2 $3 $4 $5"
    cd ..
  fi
done

#**************************************************************

if [ -n "$_HB_CC_NAME_SAVED" ]; then unset HB_CC_NAME _HB_CC_NAME_SAVED;   fi
if [ -n "$_HB_ARCH_SAVED" ];    then unset HB_ARCHITECTURE _HB_ARCH_SAVED; fi
