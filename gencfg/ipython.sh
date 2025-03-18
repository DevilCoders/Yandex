#!/usr/bin/env sh

# more recent ipythons don't work with skynet python
./venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ pip==20.1.1 --upgrade
./venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ ipython==5.7

# run it as follows:
PYTHONPATH=/skynet ./venv/venv/bin/ipython
