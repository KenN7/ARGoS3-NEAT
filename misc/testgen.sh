#!/bin/bash

FILE=$1
EXP=$2
NAME=$(basename $FILE)
echo $NAME
scp khasselmann@majorana.ulb.ac.be:"$FILE" /tmp/
/home/ken/depots/neat-argos3/bin/NEAT-launch -c $EXP -s 1234 -g /tmp/$NAME

rm /tmp/$NAME
