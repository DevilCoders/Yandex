#!/bin/bash

set -xe

MAP="
    bionic:mdb-bionic
"

FILTER="$1"

for ROW in $MAP
do
    DIST="$(echo "${ROW}" | cut -d: -f1)"
    REPO="$(echo "${ROW}" | cut -d: -f2)"
    if [[ "$FILTER" != "" ]] && [[ "$FILTER" != "$DIST" ]]
    then
        continue
    fi
    rm -rf "${DIST}"
    mkdir "${DIST}"
    DIST="${DIST}" ARCH=amd64 flock "/tmp/pbuilder-amd64-${DIST}.lock" \
        pdebuild_go \
        --use-pdebuild-internal \
        --buildresult "${DIST}" \
        --auto-debsign \
        --debsign-k robot-pgaas-ci || exit $?
    cat > dupload.conf <<EOL
package config;
\$default_host = "$REPO";
\$cfg{'$REPO'} = {
    fqdn => "$REPO.dupload.dist.yandex.ru",
    method => "scpb",
    incoming => "/repo/$REPO/mini-dinstall/incoming/",
    dinstall_runs => 0,
};
EOL
    USER=robot-pgaas-ci debrelease --debs-dir "./${DIST}" --nomail --configfile ./dupload.conf || exit "$?"

    # We need a pause between upload and dmove
    sleep 60

    cd "./${DIST}"
    for i in *.deb
    do
        pkg_name=$(echo "$i" | rev | cut -d_ -f3- | rev)
        pkg_version=$(echo "$i" | rev | cut -d_ -f2 | rev)
        ssh robot-pgaas-ci@duploader.yandex.ru sudo dmove "$REPO" stable "$pkg_name" "$pkg_version" unstable || exit "$?"
    done
    cd -
done
