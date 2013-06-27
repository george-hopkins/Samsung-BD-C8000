#!/bin/sh
# upper/lowercase problem patch, SP Team 2009-04-23

RENAMED_FILE_LIST[0]="ipt_connmark_.h"
RENAMED_FILE_LIST[1]="ipt_dscp_.h"
RENAMED_FILE_LIST[2]="ipt_ecn_.h"
RENAMED_FILE_LIST[3]="ipt_mark_.h"
RENAMED_FILE_LIST[4]="ipt_tcpmss_.h"
RENAMED_FILE_LIST[5]="ipt_tos_.h"
RENAMED_FILE_LIST[6]="ipt_ttl_.h"
RENAMED_FILE_LIST[7]="ip6t_hl_.h"
RENAMED_FILE_LIST[8]="ip6t_mark_.h"
RENAMED_FILE_LIST[9]="xt_connmark_.h"
RENAMED_FILE_LIST[10]="xt_dscp_.h"
RENAMED_FILE_LIST[11]="xt_mark_.h"
RENAMED_FILE_LIST[12]="xt_tcpmss_.h"
#RENAMED_FILE_LIST[13]="ipt_ecn_.c"
#RENAMED_FILE_LIST[14]="ipt_tos_.c"
#RENAMED_FILE_LIST[15]="ipt_ttl_.c"
#RENAMED_FILE_LIST[16]="ip6t_hl_.c"
#RENAMED_FILE_LIST[17]="xt_connmark_.c"
#RENAMED_FILE_LIST[18]="xt_dscp_.c"
#RENAMED_FILE_LIST[19]="xt_mark_.c"
#RENAMED_FILE_LIST[20]="xt_tcpmss_.c"

declare -a ORG_FILE_LIST

INDEX=0;
FILE_LIST_COUNT=${#RENAMED_FILE_LIST[*]}

TOP_DIR=`pwd`

## create symbolic link files
while [ "$INDEX" -lt "$FILE_LIST_COUNT" ] 
do
   RENAMED_FILE_PATH=`find . -name ${RENAMED_FILE_LIST[$INDEX]}`
   echo "$RENAMED_FILE_PATH"
   if [ -z "$RENAMED_FILE_PATH" ]; then echo "Renamed file, ${RENAMED_FILE_LIST[$INDEX]} not exist...!"; exit 1 ; fi

   RENAMED_FILE_DIR=${RENAMED_FILE_PATH%${RENAMED_FILE_LIST[$INDEX]}}
   SYM_FILE_NAME=${RENAMED_FILE_LIST[$INDEX]%_.*}      # ***_.h => ***
   PREFIX_TEMP=${RENAMED_FILE_LIST[$INDEX]#*.}         # ***_.h => h

   cd $RENAMED_FILE_DIR

       case "$PREFIX_TEMP" in
       "h") 
           ORG_FILE_LIST[$INDEX]="$SYM_FILE_NAME.h"
           if [ ! -f "$SYM_FILE_NAME.h" ]; then ln -s ${RENAMED_FILE_LIST[$INDEX]} $SYM_FILE_NAME.h; fi
           ;;
       "c")
           ORG_FILE_LIST[$INDEX]="$SYM_FILE_NAME.c"
           if [ ! -f "$SYM_FILE_NAME.c" ]; then ln -s ${RENAMED_FILE_LIST[$INDEX]} $SYM_FILE_NAME.c; fi
           ;;
       *)
           echo "Unknown files, error...!"; exit 1;
           ;;
       esac
   
   cd $TOP_DIR
    let "INDEX = $INDEX+ 1"
done

## remove symbolic files
if [ "$1" == "distclean" ] || [ "$1" == "mrproper" ] 
then

INDEX=0
while [ "$INDEX" -lt "$FILE_LIST_COUNT" ] 
do
   REMOVE_FILE=`find . -name ${ORG_FILE_LIST[$INDEX]}`
   #echo "$REMOVE_FILE"
   rm -f $REMOVE_FILE
   let "INDEX = $INDEX+ 1"
done

fi 

