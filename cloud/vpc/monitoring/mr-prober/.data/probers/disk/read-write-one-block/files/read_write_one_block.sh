#!/bin/bash

FILENAME=/mnt/test/read_write_one_block.data

TIMESTAMP=$(date +%s)
if [ $((TIMESTAMP % 2)) -eq 0 ] && [ -f $FILENAME ]
then
  # In case of even timestamp, read block
  dd bs=4096 count=1 if=$FILENAME of=/dev/null iflag=direct,dsync,sync || exit 1
else
  # In case of odd timestamp, write block
  dd bs=4096 count=1 if=/dev/urandom of=$FILENAME oflag=direct,dsync,sync || exit 1
fi
