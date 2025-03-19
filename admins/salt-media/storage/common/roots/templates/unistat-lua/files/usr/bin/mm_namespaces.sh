#!/bin/bash

cache_file_tmp="{{ salt['pillar.get']('libmastermind_cache_path', '/var/cache/mastermind/mediastorage-proxy.cache') }}"

nss_file='/var/tmp/mm.namespaces'
if [ ! -e $nss_file ]
then
    touch $nss_file
    chmod +r $nss_file
fi

cache_file=$(ls -t $cache_file_tmp* |grep -v tmp| head -n1)

if echo $cache_file | grep -Pq ".zst$"
then
    zst_cache_file='/var/tmp/zst_cache_file'
    zstdcat $cache_file | pv -q --rate-limit 4M > $zst_cache_file
    cache_file=$zst_cache_file
fi

tmp_file=$(mktemp)
/usr/bin/mmc-list-namespaces --file $cache_file > $tmp_file

# MDS-17457
echo "avatars-iot" >> $tmp_file

if ! grep -q 'disk' ${tmp_file}
then
    echo "Failed to get namespaces"
fi
count_in_tmp_file=$(cat $tmp_file | wc -l)
count_in_ns_file=$(cat $nss_file | wc -l)

if [ $count_in_tmp_file -gt 0 ]
then
    mv $tmp_file $nss_file
else
    rm $tmp_file
fi

chmod +r $nss_file
