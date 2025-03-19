#!/bin/bash

#BUG. move to package
mkdir -p /home/www/kinopoisk.ru/bo2/upload
mkdir -p /home/www/kinopoisk.ru/api/yandex

#called from conductor service kinopoisk_symlink
chown -R www-data:www-data /home/www/kinopoisk.ru/js
chown -R www-data:www-data /home/www/kinopoisk.ru/bo
chown -R www-data:www-data /home/www/kinopoisk.ru/bo2


