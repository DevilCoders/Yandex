#!/bin/bash
dir=$(pwd)

echo $dir

rm $dir/*.deb
rm $dir/*.build
rm $dir/*.changes
rm $dir/*.dsc
rm $dir/*.tar.gz
rm $dir/*.upload
rm $dir/*.diff.gz

echo seems cleared
