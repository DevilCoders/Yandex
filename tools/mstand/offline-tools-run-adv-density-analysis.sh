#!/bin/sh

set -e

output="results-`date +%s`.wiki"

./offline-fetch-serps.py -i pool-offlinesup-467.json --serp-set-filter true
./offline-parse-serps.py
./offline-calc-metric.py -b adv-monitoring-metric-batch.json -o pool-with-results.json --no-use-cache
./offline-tools-adv-density-analysis.py -i pool-with-results.json > $output
echo "Results stored in $output"

