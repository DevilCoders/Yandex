#!/bin/bash

set -x

docker system prune --force --all --filter until=8h

# drop ansible-juggler garbage
find /home/robot-pgaas-ci/ -maxdepth 1 -type f -name '*.retry' -delete

# drop old workspace files
find /home/robot-pgaas-ci/workspace/ -maxdepth 1 -mmin +240 -print0 | xargs -r -0 -n1 rm -rf
