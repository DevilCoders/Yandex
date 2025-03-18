#!/bin/sh

python ./abt-fetch-single.py --metric sspu.web unknown_metric --metric-type base --observation-id 23149 --date-from 20151205 --date-to 20151207 "$@"
