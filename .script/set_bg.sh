#!/bin/bash

if [ -z $BGSCRIPT ]; then
  echo "Please export a BGSCRIPT environmental variable to the directory of this script"
  exit -1
fi

source $BGSCRIPT/defaults 

get_backgrounds()
{
  if [ "$ONLINE" = true ]; then
    BACKGROUNDS=($(cat $URLS))
  else
    BACKGROUNDS=($(ls -I $TMP_IMG $RDIR))
  fi
  MAX=${#BACKGROUNDS[@]}
}

check_online()
{
  [ -f "$URLS" ]
  ONLINE=$?

  ping -c 1 -q www.google.com
  

  if [ "$ONLINE" = 0 ] && [ $? = 0 ] && [ "$INTERNET" = true ]; then
    ONLINE=true
  else
    ONLINE=false
  fi
}

next_img()
{
  if [ "$ONLINE" = true ]; then
    on_next_img
  else
    off_next_img
  fi
}

off_next_img()
{
  if [ "$SHUFFLE" = true ]; then
    index=$RANDOM
    let "index%=MAX"
    IMAGE=${BACKGROUNDS[index]}
  else
    IMAGE=${BACKGROUNDS[index]}
    ((index++))
    if [ $index -ge $MAX ]; then
      index=0
    fi
  fi
}

on_next_img()
{
  if [ "$SHUFFLE" = true ]; then
    index=$RANDOM
    let "index%=MAX"
    IMAGE=${BACKGROUNDS[index]}
  else
    IMAGE=${BACKGROUNDS[index]}
    ((index++))
    if [ $index -ge $MAX ]; then
      index=0
    fi
  fi
  wget -O $RDIR/$TMP_IMG $IMAGE
  echo $IMAGE > $BGSCRIPT/$CURRENT
  IMAGE=$TMP_IMG
}

set_img()
{
  feh --$FILL_MODE $RDIR/$IMAGE
}

set_img
while [ "$CYCLE" = true ]; do
  duration=0
  check_online
  get_backgrounds
  next_img
  set_img
  while [ $duration -le $INTERVAL ]; do
    duration=$(($duration+$DELTA))
    source $RC
    sleep $DELTA
  done
done
