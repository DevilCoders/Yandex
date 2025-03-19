#!/bin/sh

export GNUPGHOME="$HOME/.gnupg"
# get deb/udeb/ddeb file names from changes files
DEBS=$(cat "$@" | grep -E '^ \S+ \S+ \S+deb$'| grep -oE '\S+deb$' | sort -u )
echo $DEBS
# get first changes file name from input
FIRST_FILE=$(echo $@ | cut -d ' ' -f 1)
# get path where changes and deb files are
DEB_PATH=$(dirname $FIRST_FILE)
# sign debs
for deb in $DEBS
do
    debsigs -v --sign=origin -k 15CEBB54479ECBB315B390E7D6E78992614FA282 "$DEB_PATH/$deb"
done

# update checksums in changes files
for a_changes in $@
do
    changestool $a_changes updatechecksums
done
# sign changes files
exec debsign -k15CEBB54479ECBB315B390E7D6E78992614FA282 "$@"
