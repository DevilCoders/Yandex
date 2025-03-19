#!/usr/bin/env bash

if [ "$(pgrep -f [y]asmagent/run.py | wc -l)" != "1" ]; then
        pkill -f [y]asmagent/run.py
fi
