#!/usr/bin/env bash

# BREAKING NEWS: you may use universal porto layer builder from quality/yaqlib
# These scripts are still working, but it's better to migrate to new layers with native python2/3 and new ubuntu 18.04 (bionic)


# This script is NOT intended for direct running, it's for sandbox task BUILD_PORTO_LAYER.
# If you want to test it and build environment - run build_layer_in_chroot.sh

# "strict mode"
# crash on error
set -e
# crash on undefined variable
set -u

inst_cmd()
{
    apt-get install -y --force-yes "$@"
}

build_and_install_xgboost()
{
	## Revome this after xgboost.DMatrix( [[10,0]], feature_names=['a', 'b']) test pass at the pypi xgboost package
	## xgboost start

	mkdir -p xgboost-build
	cd  xgboost-build
	wget https://proxy.sandbox.yandex-team.ru/220976771 -O xgboost.tgz
	tar -xvf xgboost.tgz
	rm xgboost.tgz

	cd xgboost
	./build.sh
	cd python-package
	python setup.py install
	echo "xgboost installed OK"

	rm -rf /xgboost-build
	echo "/xgboost-build removed"
	## xgboost end
}

build_and_install_jq()
{
	cd /
	mkdir -p jq-build
	cd  jq-build
	wget https://proxy.sandbox.yandex-team.ru/227827125 -O jq.tgz
	tar -xvf jq.tgz
	rm jq.tgz
	cd jq-1.5
	autoreconf -i
	./configure --disable-maintainer-mode
	make
	make install
	echo "jq installed OK"
	cd /
	rm -rf /jq-build
	echo "/jq-build removed"
	## jq end
}


echo "###########################################"
echo "ENVIRONMENT INFO"
echo "Ubuntu version:"
lsb_release -r
echo "Python version:"
python --version
echo "###########################################"

rm -rf /etc/apt/preferences.d/*
cat > /etc/apt/sources.list.d/yandex.list <<EOF
deb http://ru.archive.ubuntu.com/ubuntu/ trusty main universe multiverse
deb http://ru.archive.ubuntu.com/ubuntu/ trusty-security main universe multiverse

deb http://mirror.yandex.ru/ubuntu trusty main multiverse restricted universe
deb http://mirror.yandex.ru/ubuntu trusty-security main multiverse restricted universe
deb http://mirror.yandex.ru/ubuntu trusty-updates main multiverse restricted universe
deb http://mirror.yandex.ru/ubuntu trusty-backports main multiverse restricted universe

deb http://system.dist.yandex.ru/system configs/all/
deb http://system.dist.yandex.ru/system trusty/amd64/
deb http://system.dist.yandex.ru/system trusty/all/

deb http://yandex-trusty.dist.yandex.ru/yandex-trusty stable/all/
deb http://yandex-trusty.dist.yandex.ru/yandex-trusty stable/amd64/


deb http://common.dist.yandex.ru/common stable/all/
deb http://common.dist.yandex.ru/common stable/amd64/

deb http://dist.yandex.ru/system/ trusty/all/
EOF

apt-get update

#inst_cmd jq
inst_cmd autoconf
inst_cmd libtool

inst_cmd make # python build, tests, etc.

BASE_DEPS="gcc g++ python-dev gfortran"
inst_cmd $BASE_DEPS

inst_cmd libgomp1 # for xgboost
inst_cmd libgfortran3 # for decisiontrain

inst_cmd imagemagick
inst_cmd pigz

inst_cmd python-pip

# https://wiki.yandex-team.ru/security/ssl/sslclientfix/#ubuntu
wget https://crls.yandex.net/YandexInternalRootCA.crt -O /usr/local/share/ca-certificates/YandexInternalRootCA.crt
update-ca-certificates

USE_CUSTOM_PYTHON=true
PYTHON_DEPS="libreadline-dev libssl-dev libbz2-dev sqlite3 tk-dev libsqlite3-dev libc6-dev libgdbm-dev libncurses-dev"

if $USE_CUSTOM_PYTHON; then
    # for python build
    inst_cmd $PYTHON_DEPS

    PY_VER="2.7.14"
    PYTHON="Python-$PY_VER"
    PYTHON_TGZ_URL="https://storage.yandex-team.ru/get-devtools/937458/7e80d3f3d3312f56eac24b4b4aa1357b2545a03e/Python-2.7.14.tgz"
    wget "${PYTHON_TGZ_URL}" -O "${PYTHON}.tgz.tmp"
    mv "${PYTHON}.tgz.tmp" "${PYTHON}.tgz"
    tar -xvf "${PYTHON}.tgz"
    cd "$PYTHON"

    LOCAL_DIR=/usr/local
    # flags taken from native python
    DISTRO_FLAGS="--enable-ipv6 --enable-unicode=ucs4 --with-dbmliborder=bdb:gdbm --with-system-expat --with-computed-gotos --with-system-ffi --with-fpectl"
    ./configure --prefix=${LOCAL_DIR} ${DISTRO_FLAGS}
    make
    make install
    cd ..
    echo "Python build cleanup"
    rm -rf "$PYTHON" "${PYTHON}.tgz"

    export PATH=/usr/local/bin:$PATH

    echo "New python version:"
    if ! python --version 2>&1 | grep "$PY_VER"; then
        echo "Wrong default python version. Something went wrong"
        exit 1
    fi
    echo "export PATH=/usr/local/bin/:\$PATH" >> /root/.bashrc.local
    echo "export PYTHONPATH=/usr/local/lib/python2.7/dist-packages" >> /root/.bashrc.local
    export PYTHONPATH=/usr/local/lib/python2.7/dist-packages
    echo "Python-related works completed"
fi

pip install --upgrade pip
export PATH=/usr/local/bin:$PATH
# surprise: now pip is in /usr/local/bin
pip install --upgrade setuptools


pip install --upgrade --ignore-installed six
pip install --upgrade --ignore-installed urllib3
pip install --upgrade --ignore-installed chardet
pip install --upgrade --ignore-installed requests

cat > /tmp/requirements.txt <<EOF
tornado>=4.3,<5.0
bokeh>=0.12.15
docopt>=0.6.2
jinja2>=2.10
numpy>=1.14.2
pandas>=0.22
pytest>=3.5.0
pytest-logging>=2015.11
scipy>=1.0.1
scikit-learn>=0.19.1
ujson>=1.35
attrs>=18.2.0
dill
# for work with dates on YT
iso8601
# for postprocessing scripts

# Commenting xgboost due to MSTAND-1055 issue:
# Untill the xgboost.DMatrix( [[10,0]], feature_names=['a', 'b']) will fail we will use xgboost from trunk
#xgboost
EOF

pip install --upgrade -r /tmp/requirements.txt

cat > /tmp/requirements-yandex.txt <<EOF
# market squeeze
scarab

decisiontrain
yandex-yt==0.8.*
yandex-yt-yson-bindings
EOF

build_and_install_xgboost
build_and_install_jq

pip install -r /tmp/requirements-yandex.txt --index http://pypi.yandex-team.ru/simple/ --trusted-host pypi.yandex-team.ru

apt-get purge -y --force-yes --auto-remove $BASE_DEPS $PYTHON_DEPS
apt-get autoremove -y --force-yes
apt-get clean

rm -rf /var/lib/apt
rm -rf /var/cache

rm -rf /tmp/*

mkdir /coredumps

