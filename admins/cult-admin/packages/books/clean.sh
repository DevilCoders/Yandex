#!/bin/sh

debclean
rm -rf -- ./*.deb ./*.build ./*.changes ./*.upload ./*.dsc ./*.tar.gz
