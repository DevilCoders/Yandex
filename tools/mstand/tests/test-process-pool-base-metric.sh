#!/bin/sh

input_file="pool66.json"
wget -N -q "https://nirvana.yandex-team.ru/api/storage/11f2121d-dd25-4668-975d-a0a66075117a/data" --output-document "$input_file"
output_file="result-of-$input_file"
python ./abt-fetch.py --metric sspu.web --metric-type base --input-file "$input_file" --output-file "$output_file"
