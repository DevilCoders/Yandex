#!/bin/bash

#See https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/Snippetnyjjinstrumenty#perevodychereztanker
function download {
    kset=$1
    case $2 in
    tur)
        lang=tr ;;
    eng)
        lang=en;;
    kaz)
        lang=kk;;
    rus)
        lang=ru;;
    ukr)
        lang=uk;;
    blr)
        lang=be;;
    ind)
        lang=id;;
esac
    curl "http://tanker-api.tools.yandex.net:3000/keysets/po/?project-id=snippets&keyset-id=$kset&language=$lang"
}


for x in blr eng kaz rus tur ukr ind; do
    download robotstxt $x  > texts/robots_txt_$x.po
done;

for x in blr eng kaz rus tur ukr ind; do
    download schemaorg $x > texts/schema_org_$x.po
done;

for x in blr eng kaz rus tur ukr ind; do
    msgcat texts/*_$x.po | msgfmt -o $x.mo -
done;
