#!/usr/bin/env bash
# see details here https://wiki.yandex-team.ru/pypi/#zagruzkapaketov
set -ex
cd $(dirname $0)
ya make -r --add-result=.py
python3 setup.py clean && python3 setup.py preprocess && cd distribute && python3 setup.py sdist upload -r yandex


