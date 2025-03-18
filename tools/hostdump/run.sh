#!/bin/bash

set -xue -o pipefail -o posix

host_name=$1
archive_name=$2
kiwi_dump_name=$archive_name
dump_dir_name=$archive_name"_dir"
dump_dir_path=$(pwd)"/"$dump_dir_name
archive_file_name=$archive_name".tar.gz"


wget https://proxy.sandbox.yandex-team.ru/63902916 -O kwworm
chmod +x kwworm

mkdir -p $dump_dir_name
./kwworm iterate -k 50 -f protobin -q 'return $URL, $HTTPResponse;' $host_name > $kiwi_dump_name
cat $kiwi_dump_name | ./host_dump_formatter --dir $dump_dir_path
tar -czvf $archive_file_name $dump_dir_name
