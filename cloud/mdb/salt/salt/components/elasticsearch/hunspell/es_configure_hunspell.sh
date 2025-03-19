#!/bin/bash

# This script creates symlinks for .dic and .aff files from ${src_dir} to ${dest_dir}
# https://www.elastic.co/guide/en/elasticsearch/reference/current/analysis-hunspell-tokenfilter.html

src_dir="/usr/share/hunspell"
dest_dir="/etc/elasticsearch/hunspell"

rm -rf ${dest_dir}

for file in "${src_dir}"/*.dic; do
    country_code=$(basename -s ".dic" "${file}")
    mkdir -p "${dest_dir}/${country_code}"
    for ext in "dic" "aff"; do
        ln -s "${src_dir}/${country_code}.${ext}" "${dest_dir}/${country_code}/${country_code}.${ext}"
    done
done
