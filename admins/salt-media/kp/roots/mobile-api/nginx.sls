{% set yaenv = grains['yandex-environment'] %}
{% if yaenv in ['development'] %}
{% set host_fqdn = grains['fqdn'] %}
{% set dev_user = salt['pillar.get']('dev_users_to_host:'+host_fqdn) %}
{% set servername = dev_user+'.'+pillar.domains.kp_ext %}
{% else %}
{% set servername = pillar.domains.kp_ext %}
{% endif %}

include:
  - templates.nginx

sites-enabled/php-fpm.conf:
  yafile.managed:
    - name: /etc/nginx/sites-enabled/10-php-fpm-ext.conf
    - source: salt://backend/files/etc/nginx/sites-enabled/10-php-fpm.conf
    - template: jinja
    - context:
      servername: {{ servername }}
      rootdir: /home/www/ext.kinopoisk.ru/ext
    - watch_in:
      - service: nginx

