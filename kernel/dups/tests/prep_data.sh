#!/bin/sh -e

for r in $@; do
    cat $r.req | ./fetch_test_data.py -u xmlsearch.hamster.yandex.ru -m wspm-dev2:13338 -rf -e "&rd=0&rdba=0" --xml > $r.xml
    echo `date`: "Fetched $r.req"
done

for r in $@; do
    ./fetch_test_data.py -F IsFake,AuraDocLogAuthor -p $r.xml -j > $r.json
    echo `date`: "Parsed $r.xml"
done
