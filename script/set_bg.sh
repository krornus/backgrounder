#!/bin/bash

index=0
if [ -z $BGSCRIPT ]; then
  echo "Please export a BGSCRIPT environmental variable to the directory of this script"
  exit -1
fi

source $BGSCRIPT/defaults 
MODIFIED=$(stat -c %Y $RC)

get_backgrounds()
{
  if [ "$ONLINE" = true ]; then
    BACKGROUNDS=($(cat $URLS))
  else
    BACKGROUNDS=($(ls $RDIR/$SAVED))
  fi
  MAX=${#BACKGROUNDS[@]}
}

check_online()
{
  [ -f "$URLS" ]
  ONLINE=$?

  ping -c 1 -q www.google.com

  if [ $? = 0 ] && [ "$ONLINE" = 0 ] && [ "$INTERNET" = true ]; then
    ONLINE=true
  else
    ONLINE=false
  fi
  echo $ONLINE
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
  IMAGE=$RDIR/$SAVED/$IMAGE
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
  wget -O $RDIR/$IMGLIST/$TMP_IMG $IMAGE
  echo $IMAGE > $RDIR/$IMGLIST/$CURRENT
  IMAGE=$RDIR/$IMGLIST/$TMP_IMG
}

set_img()
{
  feh --$FILL_MODE $IMAGE
}

check_online
get_backgrounds
next_img
while [ "$CYCLE" = true ]; do
  duration=0
  next_img
  set_img
  get_backgrounds
  check_online
  while [ $duration -le $INTERVAL ]; do
    duration=$(($duration+$DELTA))
    if [ $MODIFIED != $(stat -c %Y $RC) ]; then
      duration=$INTERVAL
      MODIFIED=$(stat -c %Y $RC)
      source $RC
    fi
    sleep $DELTA
  done
  source $RC
done
