#!/bin/sh

set -e

function sync_image() {
    short="$(echo "$1" | cut -d' ' -f1)"
    long="$(echo "$1" | cut -d' ' -f2)"
    bucket=$2
    latest="$(/usr/bin/s3cmd ls s3://${bucket}/${long}/ | grep '.gz' | sort -k2M -k1 -k3 | tail -n 1 | awk '{print $1, $3, $4};')"
    if [ -z "$latest" ]
    then
        echo "$long was not found in bucket $bucket. Skipping."
        return
    fi
    latest_date="$(echo "${latest}" | awk '{print $1};')"
    latest_size="$(echo "${latest}" | awk '{print $2};')"
    latest_path="$(echo "${latest}" | awk '{print $3};')"
    target_path="/data/images/${short}-template-${latest_date}.tar.gz"
    skip_download=0

    if [ -f "${target_path}" ]
    then
        target_size="$(stat -c %s "${target_path}")"
        if [ "${target_size}" = "${latest_size}" ]
        then
            echo "Download will be skipped"
            skip_download=1
        fi
    fi

    if [ "${skip_download}" = 0 ]
    then
        s3cmd get --force "${latest_path}" "${target_path}"
    fi

    cur_link="$(ls -la /data/images/${short}-template.tar.gz | awk '{print $NF};')"

    if [ "${cur_link}" != "${target_path}" ]
    then
        echo "Updating link"
        ln -sf "${target_path}" "/data/images/${short}-template.tar.gz"
        rm -f "${cur_link}"
    fi
}

echo "Starting $(date)"

{% for img in salt['pillar.get']('data:images') %}
sync_image "{{ img.short_name }}-bionic {{ img.name }}-bionic" "{{ salt['pillar.get']('data:dom0porto:image_bucket', 'dbaas-images') }}"
{% endfor %}

# legacy trusty images
for i in "pg postgresql" "ch clickhouse" "mongodb mongodb" "zk zookeeper" "redis redis" "mysql mysql"
do
  sync_image "${i}" "dbaas-images"
done
