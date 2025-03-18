#!/bin/bash

set -e -o pipefail
set -x

ya make -r && strip ./uc && tar -zchO ./uc | ya upload -d "uc for linux" -T "SCRIPT_BUILD_RESULT" -A "platform=Linux-3.10.69-25-x86_64-with-Ubuntu-10.04-lucid" -A "tool_name=uc" --do-not-remove --stdin-tag linux-uc.tgz
