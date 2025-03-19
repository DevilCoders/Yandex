# loads AWS keys from yav.
# requires ya tool installed and YAV_TOKEN environment variable
#
# source from shell where terragrunt is run:
#   $ export YAV_TOKEN=...
#   $ . credentials.sh

SECRET_VERSION=ver-01eeg2ztgzc2r5d91t6cm5z9jb

read AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY < <(
    ya vault get version -j $SECRET_VERSION |
        python3 -c '
import json, sys
v = json.load(sys.stdin)["value"]

key, secret = next(iter(v.items()))
print(key, secret)')

export AWS_ACCESS_KEY_ID AWS_SECRET_ACCESS_KEY
