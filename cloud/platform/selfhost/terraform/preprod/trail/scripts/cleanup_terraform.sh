#!/bin/sh

for i in trail-preparer trail-exporter trail-control-plane trail-tool
do
  cd $i
  rm -rf .terraform
  cd ..
done
