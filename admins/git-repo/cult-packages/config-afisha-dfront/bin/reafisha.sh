#!/bin/bash

/usr/lib/yandex/afisha/scripts/frontgen.sh

# creating correct config file for django:
if [[ $(cat /etc/yandex/environment.type) == "production" ]]; then
    rm /etc/yandex/yandex-afisha/local_settings.py;
    ln -s /etc/yandex/yandex-afisha/production-lyra.py /etc/yandex/yandex-afisha/local_settings.py;
fi

/etc/init.d/syslog-ng restart
/etc/init.d/django restart all
/etc/init.d/gunicorn restart all
/etc/init.d/nginx restart

