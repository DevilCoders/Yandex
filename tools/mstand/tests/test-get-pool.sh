#!/usr/bin/env bash

set -e

# this test should be executed within a few seconds.
# if not => something went wrong

python ./adminka-get-pool.py --date-from=20151201 --date-to=20151201 --queue-id=1
python ./adminka-get-pool.py 24763
