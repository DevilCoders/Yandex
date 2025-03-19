#! /bin/bash

set -e

virtualenv3 venv
. venv/bin/activate
pip3 install pylint
deactivate
virtualenv3 --relocatable venv
