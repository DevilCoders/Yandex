#!/usr/bin/env bash
# see details here https://wiki.yandex-team.ru/pypi/#zagruzkapaketov
set -ex
python setup.py clean && python setup.py preprocess && cd distribute && python setup.py sdist upload -r yandex


