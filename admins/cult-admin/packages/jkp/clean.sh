#!/bin/sh

(
	cd $(dirname $0)

	debclean
	rm -rf -- ./*.deb ./*.build ./*.changes ./*.upload ./*.dsc ./*.tar.gz
)
