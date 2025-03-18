#! /bin/sh

echo "yandex.ru, www.yandex.ru = {"
yr +META PRINTSERVERLIST | awk '{print "    "$0".yandex.ru"}' | sort | grep -v pgfront
echo "}"

for i in 1 2 3
do
    echo
    echo "www$i.yandex.ru = {"
    yr +META$i PRINTSERVERLIST | awk '{print "    "$0".yandex.ru"}' | sort | grep -v pgfront
    echo "}"
done
