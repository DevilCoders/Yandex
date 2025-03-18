#!/usr/bin/env sh

./pure_compiler -d $1 | ./dump2input.sh $1_hdf.txt > $1.txt
sh relemmatize_pure_.sh $1.txt $1_.txt $2
