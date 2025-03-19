#!/bin/bash

set -ev

python main.py || exit 1
python3 main.py || exit 1
