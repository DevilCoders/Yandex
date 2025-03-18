#!/usr/bin/env bash
set -e -o pipefail

sort -S 2000M sample_urls.txt | uniq -c > sample_urls_sorted.txt
cat sample_urls_sorted.txt | ./url_sampler 2>hoststats.txt >sample_urls_to_download.txt
pv -l sample_urls_to_download.txt | cut -f 2 | ./kwworm read -k 50 -Q read_sample.query -f protobin > sample_docs.protobin

