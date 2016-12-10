#!/bin/bash

RDIR="$HOME/pictures/backgrounder/img"
URLS=""
SHUFFLE=true
CYCLE=true
INTERVAL=90
RC=$HOME/.backgroundrc
FILL_MODE=bg-fill

source $RC

TMP_IMG="tmp.img"
IMAGE=$TMP_IMG
index=0

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
  

  if [ "$ONLINE" = 0 ] && [ $? = 0 ]; then
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
  IMAGE=$TMP_IMG
}

set_img()
{
  feh --$FILL_MODE $RDIR/$IMAGE
}

set_img
check_online
get_backgrounds
next_img
set_img
while [ "$CYCLE" = true ]; do
  set_img
  check_online
  get_backgrounds
  next_img
  source $RC
  sleep $INTERVAL
done
