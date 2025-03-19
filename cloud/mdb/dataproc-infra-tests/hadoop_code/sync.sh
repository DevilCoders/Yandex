#!/bin/bash

DIR=$(pwd)

cd "$DIR/data" && ./sync.sh
cd "$DIR/hive-001" && ./sync.sh
cd "$DIR/java" && ./sync.sh
cd "$DIR/mapreduce-001" && ./sync.sh
cd "$DIR/pyspark-001" && ./sync.sh
cd "$DIR" || exit
