{% set yaenv = grains['yandex-environment'] -%}



nginx:
  service.running:
    - enable: True
    - reload: True
    - require:
      - cmd: allCAs
      - cmd: combinedcrl

cond.d/realip.conf:
  yafile.managed:
    - name: /etc/nginx/conf.d/realip.conf
    - source: salt://{{ slspath  }}/files/etc/nginx/conf.d/realip.conf
    - watch_in:
      - service: nginx

{% for configfile in [
    '20-kp1-api-internal.conf',
    '20-kp1-media-api.conf',
    '20-music-api.conf',
    '20-awaps.conf',
    '20-laas.conf',
    '02-default-go-404.conf',
    '05-php-fpm.conf',
    '10-bo.kinopoisk.ru.conf',
    '10-www.kinopoisk.ru.conf',
    '15-bo.kinopoisk.ru-for-idm.conf'
    ] %}
/etc/nginx/sites-enabled/{{ configfile }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/{{ configfile }}
    - template: jinja
    - watch_in:
      - service: nginx
{% endfor %}

{% for config in [
    'auth-app-fallback',
    'auth-app-location',
    'auth-app',
    'proxy-headers'
    ] %}
/etc/nginx/include/{{ config }}.conf:
  yafile.managed:
    - makedirs: true
    - source: salt://{{ slspath }}/files/etc/nginx/include/{{ config }}.conf
    - template: jinja
    - watch_in:
      - service: nginx
{% endfor %}

combinedcrl:
  cmd.run:
    - name: curl -s https://crls.yandex.net/combinedcrl -o /etc/nginx/ssl/combinedcrl
    - unless: "[[ -f /etc/nginx/ssl/combinedcrl ]]"

/etc/cron.daily/update_combinedcrl.sh:
  file.managed:
    - contents: |
        #!/bin/bash
        set -e  # fail on error
        CRL_TEMP_FILE=`mktemp`
        CRL_WORK_FILE=/etc/nginx/ssl/combinedcrl
        # get Certificate Revocation List
        curl -so $CRL_TEMP_FILE https://crls.yandex.net/combinedcrl
        # check that file loadable by nginx
        openssl crl -noout -in $CRL_TEMP_FILE
        # if file diff then update working copy
        if ! diff $CRL_TEMP_FILE $CRL_WORK_FILE &>/dev/null ;then
          sudo mv $CRL_TEMP_FILE $CRL_WORK_FILE
        fi

allCAs:
  cmd.run:
    - name: curl -s https://crls.yandex.net/allCAs.pem -o /etc/nginx/ssl/allCAs.pem
    - unless: "[[ -f /etc/nginx/ssl/allCAs.pem ]]"

nginx_errors:
  file.recurse:
    - name: /usr/share/kinopoisk-source-stub/stub/errors
    - source: salt://{{ slspath }}/files/usr/share/kinopoisk-source-stub/stub/errors

