#!/bin/sh

set -e

# FIXME: Here is a temporary solution to make mstand distro.

#list_and_sync()
#{
#	rsync "$1"
#	rsync --progress -avz "$1" "$2"
#}

# This part is should be done via Sandbox task REMOTE_COPY_RESOURCE, but it has 'ya package' incompatibility (--tar options issue)

project_dir="`cd $(dirname $0)/.. && pwd`"
. "${project_dir}/../../quality/yaqlib/yaqutils/build_package_common_functions.sh"

# TODO: use https://yt.yandex-team.ru/hahn/#page=navigation&path=//statbox/statbox-dict-last/blockstat.dict
# or https://sandbox.yandex-team.ru/resources?page=1&pageCapacity=20&type=YA_BLOCKSTAT_DICT&attrs=%7B%7D&recipients=
echo "blockstat.dict ..."
# Proper source of blockstat.dict:
# https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/userdata/requestaggregatelib/help/#tblockstatinfo
#list_and_sync "veles02::Berkanavt/statdata/blockstat.dict" "./"
yt --proxy hahn read-file //statbox/statbox-dict-last/blockstat.dict > ${project_dir}/blockstat.dict

echo "browser.xml ..."
yt --proxy hahn read-file //statbox/statbox-dict-last/browser.xml > ${project_dir}/browser.xml

echo "geodata4.bin ..."
yt --proxy hahn read-file //statbox/statbox-dict-last/geodata4.bin > ${project_dir}/geodata4.bin

echo "mousetrack_decoder.py ..."
if is_svn_repo; then
    svn export --force "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/logs/mousetrack_lib/python/mousetrack_decoder.py" ${project_dir}/
else
    cp ${project_dir}/../../quality/logs/mousetrack_lib/python/mousetrack_decoder.py ${project_dir}
fi

echo "Geobasev6.bin ..."
yt --proxy hahn read-file //statbox/statbox-dict-last/Geobasev6.bin > ${project_dir}/Geobasev6.bin

echo "libra.so ..."
yt --proxy hahn read-file //statbox/resources/libra3.so > ${project_dir}/libra.so
strip libra.so

echo "bindings.so ..."
yt --proxy hahn read-file //home/mstand/resources/bindings.so > ${project_dir}/bindings.so

mkdir -p py3.8
echo "py3.8/bindings.so ..."
yt --proxy hahn read-file //home/mstand/resources/py3.8/bindings.so > py3.8/bindings.so
