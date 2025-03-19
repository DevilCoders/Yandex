#!/bin/bash


cd /home/www/kinopoisk.ru/
echo "Compiling templates. It can take up to few minutes."
iter=0
line=" "
echo > /tmp/shab_generator.log
for file in `find . -type f -name "*.html" -print | sed -e s/\\\.\\\///`;
do
    if [[ $iter -eq 20 ]];
    then
        php bo/cron/cron_compile_shab.php $line 2>&1 >> /tmp/shab_generator.log;
        iter=0
        line=" "
    fi
    line="$line $file"
    let iter=iter+1
done

cd /home/www/kinopoisk.ru/ && /usr/bin/php bo/cron/cron_top_banner.php >> /tmp/top_banners.log

exit 0

