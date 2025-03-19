#!/bin/bash
export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
tmp_secrets_conf=$1
cd /etc/php/5.6/fpm
cp php-fpm.conf php-fpm.conf.tmp
sed -i 's/secrets.conf/secrets.conf.tmp/g' php-fpm.conf.tmp
cp $tmp_secrets_conf secrets.conf.tmp
php-fpm5.6 --test -y ./php-fpm.conf.tmp &>/dev/null
res=$?
rm -f secrets.conf.tmp php-fpm.conf.tmp
exit $res
