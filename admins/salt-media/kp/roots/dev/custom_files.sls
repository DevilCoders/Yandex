{% set yaenv = grains['yandex-environment'] %}
{% if yaenv in ['development'] %}
{% set host_fqdn = grains['fqdn'] %}
{% set dev_user = salt['pillar.get']('dev_users_to_host:'+host_fqdn) %}
{% set ext_servername = dev_user+'.'+pillar.domains.kp_ext %}
{% set touch_api_servername = dev_user+'.'+pillar.domains.kp_touch_api %}
{% else %}
{% set ext_servername = pillar.domains.kp_ext %}
{% set touch_api_servername = pillar.domains.kp_touch_api %}
{% endif %}

# nginx for ext
sites-enabled/php-fpm-ext.conf:
  yafile.managed:
    - name: /etc/nginx/sites-enabled/10-php-fpm-ext.conf
    - source: salt://backend/files/etc/nginx/sites-enabled/10-php-fpm.conf
    - template: jinja
    - context:
      servername: {{ ext_servername }}
      rootdir: /home/www/ext.kinopoisk.ru/ext
    - watch_in:
      - service: nginx


# nginx touch-api for dev
nginx_touch-api_conf:
  file.managed:
    - name: /etc/nginx/sites-enabled/10-touch.kinopoisk.ru-api.conf
    - source: salt://touch-api/files/etc/nginx/sites-enabled/touch-api.kinopoisk.ru.conf
    - template: jinja
    - context:
      servername: {{ touch_api_servername }}

kpmail_conf:
  file.managed:
    - name: /etc/kpmail.conf
    - source: salt://{{slspath}}/files/etc/kpmail.conf
