#!/usr/bin/env bash

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
	/usr/local/bin/python setup.py install
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
deb http://ru.archive.ubuntu.com/ubuntu/ precise main universe multiverse
deb http://ru.archive.ubuntu.com/ubuntu/ precise-security main universe multiverse

deb http://mirror.yandex.ru/ubuntu precise main multiverse restricted universe
deb http://mirror.yandex.ru/ubuntu precise-security main multiverse restricted universe
deb http://mirror.yandex.ru/ubuntu precise-updates main multiverse restricted universe
deb http://mirror.yandex.ru/ubuntu precise-backports main multiverse restricted universe

deb http://system.dist.yandex.ru/system configs/all/
deb http://system.dist.yandex.ru/system precise/amd64/
deb http://system.dist.yandex.ru/system precise/all/

deb http://yandex-precise.dist.yandex.ru/yandex-precise stable/all/
deb http://yandex-precise.dist.yandex.ru/yandex-precise stable/amd64/

deb http://common.dist.yandex.ru/common stable/all/
deb http://common.dist.yandex.ru/common stable/amd64/

deb http://dist.yandex.ru/system/ precise/all/
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

inst_cmd pigz

inst_cmd python-pip

pip install --upgrade pip
export PATH=/usr/local/bin:$PATH
# surprise: now pip is in /usr/local/bin
pip install --upgrade setuptools

cat > /tmp/requirements.txt <<EOF
bokeh>=0.12
coloredlogs
docopt>=0.6.2
jinja2>=2.8
numpy>=1.10.4
pandas>=0.18
pytest>=2.8.5
pytest-logging>=2015.11
requests>=2.9.1
scipy>=0.17
scikit-learn>=0.17.1
ghalton
# for postprocessing scripts

# Commenting xgboost due to MSTAND-1055 issue:
# Untill the xgboost.DMatrix( [[10,0]], feature_names=['a', 'b']) will fail we will use xgboost from trunk
#xgboost
EOF

pip install --upgrade -r /tmp/requirements.txt

##pip install decisiontrain --index http://pypi.yandex-team.ru/simple/ --trusted-host pypi.yandex-team.ru

##build_and_install_xgboost

##build_and_install_jq

inst_cmd r-base

#apt-get purge -y --force-yes --auto-remove $BASE_DEPS $PYTHON_DEPS
#apt-get autoremove -y --force-yes
#apt-get clean

#rm -rf /var/lib/apt
#rm -rf /var/cache

#rm -rf /tmp/*

mkdir /coredumps

mkdir -p /tmp

