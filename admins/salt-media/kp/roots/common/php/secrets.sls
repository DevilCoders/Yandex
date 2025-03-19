{% set yaenv = grains['yandex-environment'] -%}
{% set data = salt.pillar.get('php:secrets')|load_yaml %}
{% set debug_map = {'prestable': 1, 'production': 0, 'qa': 1, 'stress': 0, 'testing': 1, 'development': 1} %} 
{% set host_fqdn = grains['fqdn'] %}
{% set dev_user = salt['pillar.get']('dev_users_to_host:'+host_fqdn) %}
{% set tvmtoken = salt['cmd.shell']('cat /var/lib/tvmtool/local.auth') %}
{% set tvm_port = salt['pillar.get']('php:tvm_port') %}
{% set js_api_key = salt['pillar.get']('php:js_api_key') %}
{% set yp_token = salt['pillar.get']('php:yp_token') %}
{% set redis_cache_hosts = salt['pillar.get']('php:redis_cache_hosts') %}

#syntax check script
/usr/local/sbin/php-check-secrets-conf.sh:
  file.managed:
    - source: salt://{{slspath}}/files/php-check-secrets-conf.sh
    - mode: 0700
    - user: root
    - group: root

# CADMIN-6318
{% for version in ['5.6','7.3'] %}
/etc/php/{{ version }}/fpm/secrets.conf:
  file.managed:
    - source: salt://{{slspath}}/files/secrets.conf
    - mode: 0640
    - user: www-data
    - group: www-data
    - makedirs: true
    - template: jinja
    - context:
      data: {{data|json}}
      yaenv: {{yaenv}}
      debug: {{debug_map[yaenv]}}
      dev_user: {{dev_user}}
      tvmtoken: {{tvmtoken}}
      tvm_port: {{tvm_port}}
      js_api_key: {{js_api_key}}
      yp_token: {{yp_token}}
      redis_cache_hosts: {{redis_cache_hosts}}
    - check_cmd: /usr/local/sbin/php-check-secrets-conf.sh

# CADMIN-6963
/etc/php/{{ version }}/fpm/secrets-env.sh:
  file.managed:
    - source: salt://{{slspath}}/files/secrets-env.sh
    - mode: 0750
    - user: www-data
    - group: www-data
    - makedirs: true
    - template: jinja
    - context:
      data: {{data|json}}
      yaenv: {{yaenv}}
      debug: {{debug_map[yaenv]}}
      dev_user: {{dev_user}}
      tvmtoken: {{tvmtoken}}
      tvm_port: {{tvm_port}}
      js_api_key: {{js_api_key}}
      yp_token: {{yp_token}}
      redis_cache_hosts: {{redis_cache_hosts}}
{% endfor %}

/home/www/kinopoisk.ru/config/.env.salt:
  file.managed:
    - source: salt://{{slspath}}/files/env.conf
    - mode: 0640
    - user: www-data
    - group: www-data
    - makedirs: true
    - template: jinja
    - context:
      data: {{data|json}}
      yaenv: {{yaenv}}
      debug: {{debug_map[yaenv]}}
      dev_user: {{dev_user}}
      tvmtoken: {{tvmtoken}}
      tvm_port: {{tvm_port}}
      js_api_key: {{js_api_key}}
      yp_token: {{yp_token}}
      redis_cache_hosts: {{redis_cache_hosts}}

/home/www/kinopoisk.ru/config/.env:
  file.symlink:
    - target: /home/www/kinopoisk.ru/config/.env.salt
    - force: true
    - makedirs: true
