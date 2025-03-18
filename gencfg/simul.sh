#!/bin/bash
# pushd ~/branches/132
# PYTHONPATH=/skynet ./venv/venv/bin/python ./ypdump.py > d.json
# popd
rm -rf sandbox/* reports
PATH=$PATH:~/arcadia/yp/scripts/scheduler_simulator/environment ~/arcadia/yp/scripts/scheduler_simulator/run_experiment/yp-scheduler-simulator-run-experiment --sync --play-count 1 --process-limit 1 --yp-history ./dump.json --sandbox sandbox --report reports
