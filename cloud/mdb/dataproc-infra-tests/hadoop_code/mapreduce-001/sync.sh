#!/bin/bash

shopt -s expand_aliases
alias s3="aws --endpoint-url=https://storage.cloud-preprod.yandex.net --profile=e2e-preprod s3"

s3 cp mapper.py s3://dataproc-e2e/jobs/sources/mapreduce-001/
s3 cp reducer.py s3://dataproc-e2e/jobs/sources/mapreduce-001/
