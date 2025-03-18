#!/bin/sh

envsubst '$$WORKERS_COUNT $$PROBABILITY_SEED' \
    < /etc/nginx/nginx.conf.template \
    > /etc/nginx/nginx.conf

envsubst '$$CRYPROX $$RATELIMIT $$ACCELREDIRECT_TARGET $$JSTRACER_URL $$EXTERNAL_RESOLVER $$PROXY_ADD_X_FORWARDED_FOR $$PROBABILITY_SEED' \
    < /etc/nginx/sites-available/cryprox.conf.template \
    > /etc/nginx/sites-available/cryprox.conf

envsubst '$$CRYPROX $$RATELIMIT $$ACCELREDIRECT_TARGET $$JSTRACER_URL $$EXTERNAL_RESOLVER $$PROXY_ADD_X_FORWARDED_FOR' \
    < /etc/nginx/sites-available/naydex.conf.template \
    > /etc/nginx/sites-available/naydex.conf

envsubst '$$CRYPROX $$PROXY_ADD_X_FORWARDED_FOR' \
    < /etc/nginx/sites-available/static-mon.conf.template \
    > /etc/nginx/sites-available/static-mon.conf

envsubst '$$CRYPROX $$PROXY_ADD_X_FORWARDED_FOR' \
    < /etc/nginx/sites-available/cookiematcher.conf.template \
    > /etc/nginx/sites-available/cookiematcher.conf
