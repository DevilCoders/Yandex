#!/bin/bash

shopt -s expand_aliases
alias s3="aws --endpoint-url=https://storage.cloud-preprod.yandex.net --profile=e2e-preprod s3"

s3 cp config.json s3://dataproc-e2e/jobs/sources/data/

s3 ls s3://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
if [[ $? -ne 0 ]]; then
  echo "Going to download cities500.zip from geonames server, repack it into bzip2 format and upload to s3"
  wget -q -O /tmp/cities500.zip http://download.geonames.org/export/dump/cities500.zip
  unzip /tmp/cities500.zip -d /tmp/
  bzip2 -z /tmp/cities500.txt
  s3 cp /tmp/cities500.txt.bz2 s3://dataproc-e2e/jobs/sources/data/
  rm /tmp/cities500.txt.bz2
fi

s3 ls s3://dataproc-e2e/jobs/sources/data/hive-cities/cities500.txt
if [[ $? -ne 0 ]]; then
  echo "Going to download cities500.zip from geonames server, unpack it and upload to s3"
  wget -q -O /tmp/cities500.zip http://download.geonames.org/export/dump/cities500.zip
  unzip /tmp/cities500.zip -d /tmp/
  s3 cp /tmp/cities500.txt s3://dataproc-e2e/jobs/sources/data/hive-cities/
  rm /tmp/cities500.txt
fi

s3 ls s3://dataproc-e2e/jobs/sources/data/country-codes.csv.zip
if [[ $? -ne 0 ]]; then
  wget -q -O /tmp/country-codes.csv https://datahub.io/core/country-codes/r/country-codes.csv
  zip -j /tmp/country-codes.csv.zip /tmp/country-codes.csv
  s3 cp /tmp/country-codes.csv.zip s3://dataproc-e2e/jobs/sources/data/
  rm /tmp/country-codes.csv.zip
  rm /tmp/country-codes.csv
fi
