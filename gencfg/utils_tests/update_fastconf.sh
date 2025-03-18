#!/bin/bash

source `dirname "${BASH_SOURCE[0]}"`/scripts/run.sh

echo "Get locations..."
run optimizers/dynamic/main.py -a get_locations -m ALL_DYNAMIC >&2
echo "Show usage..."
run optimizers/dynamic/main.py -a show -m ALL_DYNAMIC --location SAS >&2
echo "Add group..."
run optimizers/dynamic/main.py -a add -g SAS_UTILTEST_UPDATE_FASTCONF -o mishex -m ALL_DYNAMIC --location SAS --itype none --min_power 1 --verbose >&2
echo "Remove group..."
run utils/update_igroups.py -a remove -g SAS_UTILTEST_UPDATE_FASTCONF >&2
echo "Done"

