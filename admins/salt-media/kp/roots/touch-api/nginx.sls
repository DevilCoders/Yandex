{% set yaenv = grains['yandex-environment'] %}
{% if yaenv in ['development'] %}
{% set host_fqdn = grains['fqdn'] %}
{% set dev_user = salt['pillar.get']('dev_users_to_host:'+host_fqdn) %}
{% set touch_api_servername = dev_user+'.'+pillar.domains.kp_touch_api %}
{% else %}
{% set touch_api_servername = pillar.domains.kp_touch_api %}
{% endif %}

nginx-site-touch-api-conf:
  file.managed:
    - name: /etc/nginx/sites-enabled/10-touch-api.kinopoisk.ru.conf
    - source: salt://touch-api/files/etc/nginx/sites-enabled/touch-api.kinopoisk.ru.conf
    - template: jinja
    - context:
      servername: {{ touch_api_servername }}
    - watch_in:
    - service: nginx
