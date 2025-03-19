set -eux

REPO_PREFIX="$1" # like "ubuntu-"

for dist in ${DIST_NAMES:-$(ls -1 Dockerfile-*|awk -F- '{print $2}')}; do
    if ! test -e Dockerfile-$dist; then
        echo -e "\e[1;31mDockerfile-$dist not exists\e[0m"
        continue
    fi

    echo -e "\e[1;33mBuilding for \e[1;34m$dist\e[1;0m"
    docker build -f Dockerfile-$dist -t unbound-bundle-deb-$dist . ||\
         echo -e "\e[1;31mFailed to build image for $dist\e[0m"
    CONTAINER=$(docker create unbound-bundle-deb-$dist)
    mkdir -p ./artifacts/$dist
    docker cp $CONTAINER:/build/artifacts ./artifacts/$dist/
    docker rm $CONTAINER
    mv artifacts/$dist/artifacts/* artifacts/$dist
    rmdir -v artifacts/$dist/artifacts
    pushd artifacts/$dist
    debsign ${DEBSIGN_KEY:-} --debs-dir . yandex-unbound_*_amd64.changes  --re-sign
    dpkg-deb --extract yandex-unbound_*_amd64.deb yandex-unbound-extract-tmp
    mkdir -p debian
    zcat yandex-unbound-extract-tmp/usr/share/doc/yandex-unbound/changelog.gz > debian/changelog
    rm -rf yandex-unbound-extract-tmp
    popd

    pushd artifacts/$dist
    read -p "`pwd`: Release $(ls -1 yandex-unbound_*_amd64.deb) to ${REPO_PREFIX}$dist? (y/N) "
    if [[ "${REPLY,,}" =~ ^y ]]; then
        debrelease --debs-dir . --to=${REPO_PREFIX}$dist
    fi
    popd
done

read -p "`pwd`: run \`rm -rf ./artifacts\`? (y/N) "
if [[ "${REPLY,,}" =~ ^y ]]; then
    rm -rf ./artifacts
fi

