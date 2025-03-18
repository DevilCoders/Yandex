#!/bin/bash

####
# THIS IS TEMPORARY SOLUTION !!! WE NEED NORMAL BASE CODE FOR TESTING UTILS !!!
####

#####
# RUN EXCLUSIVELY, AS TESTS CAN MODIFY ./db
####

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

echo "TESTING UPDATE FASTCONF..."
# temporary commented, needed tag
# utils_tests/update_fastconf.sh

