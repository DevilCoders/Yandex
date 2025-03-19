#!/bin/bash
## This script is for cult-admin packages
## Takes template from ../packages/general/template/ and gens new package into ../packages/<PRJ>/<PKG_NAME>
# Getting script params
SCRIPT_NAME=`basename $0`
PRJ=$1
PKG_NAME=$2

# Constants
PRJ_PATH="../packages/$PRJ"
PKG_PATH="$PRJ_PATH/$PKG_NAME"
TMPL_PATH="../packages/general/template/*"

MAINTAINER_FNAME=$DEBFULLNAME
MAINTAINER_LOGIN=$DEBEMAIL

DATE=`date -R`

# Check if all ok
if [[ -z "$PRJ" ]] || [[ -z "$PKG_NAME" ]]; then
	echo "Usage: $SCRIPT_NAME project package_name"
	exit 0
fi

if [[ -z "$MAINTAINER_FNAME" ]] || [[ -z "$MAINTAINER_LOGIN" ]]; then
	echo "Empty DEBFULLNAME or DEBEMAIL variables. Update them, please!"
	exit 0
fi

if [[ ! -d "$PRJ_PATH" ]]; then
	echo "Project $PRJ not found!"
	exit 0
elif [[ -d "$PKG_PATH" ]]; then
	echo "$PKG_PATH already exists!"
	exit 0
fi

## If all ok, lets start
# Copy template into new package dir
mkdir $PKG_PATH
cp -Rp $TMPL_PATH $PKG_PATH && echo "Success copied template into $PKG_PATH"

# Rename variables in template
sed -i "s/<_PKGNAME_>/$PKG_NAME/g" $PKG_PATH/Makefile
sed -i "s/<_PKGNAME_>/$PKG_NAME/g" $PKG_PATH/debian/README.Debian
sed -i "s/<_PKGNAME_>/$PKG_NAME/g" $PKG_PATH/debian/rules
sed -i "s/<_PKGNAME_>/$PKG_NAME/g" $PKG_PATH/debian/postinst
sed -i "s/<_PKGNAME_>/$PKG_NAME/g" $PKG_PATH/debian/changelog
sed -i "s/<_PKGNAME_>/$PKG_NAME/g" $PKG_PATH/debian/control

sed -i "s/<_MAINTAINER_FNAME_>/$MAINTAINER_FNAME/g" $PKG_PATH/debian/changelog
sed -i "s/<_MAINTAINER_FNAME_>/$MAINTAINER_FNAME/g" $PKG_PATH/debian/control
sed -i "s/<_MAINTAINER_FNAME_>/$MAINTAINER_FNAME/g" $PKG_PATH/debian/README.Debian

sed -i "s/<_YA_LOGIN_>/$MAINTAINER_LOGIN/g" $PKG_PATH/debian/changelog
sed -i "s/<_YA_LOGIN_>/$MAINTAINER_LOGIN/g" $PKG_PATH/debian/control
sed -i "s/<_YA_LOGIN_>/$MAINTAINER_LOGIN/g" $PKG_PATH/debian/README.Debian

sed -i "s/<_PROJECT_>/$PRJ/g" $PKG_PATH/debian/README.Debian

sed -i "s/<_DATE_>/$DATE/g" $PKG_PATH/debian/README.Debian
sed -i "s/<_DATE_>/$DATE/g" $PKG_PATH/debian/changelog

# Work finished
echo "$PKG_NAME created at $PKG_PATH"
echo "Do not forget to update description/depends/conflicts in $PKG_PATH/debian/control!"
echo "Have a good day, bye bye"
