{% set yaenv = grains['yandex-environment'] -%}

{% set data = salt.pillar.get('php:touch-api-secrets')|load_yaml %}
{% set tvm_port = salt.pillar.get('php:tvm_port') %}
{% set js_api_key = salt.pillar.get('php:js_api_key') %}
{% set redis_cache_hosts = salt.pillar.get('php:redis_cache_hosts') %}

{% set debug_map = {'prestable': 0, 'production': 0, 'stress': 0, 'testing': 1, 'development': 1} %} 

{% set env_prefix_map = {'prestable': '.',
                         'production': '.',
                         'stress': '.load.',
                         'testing': '.tst.',
                         'development': '.dev.'}
%}

{% set host_fqdn = grains['fqdn'] %}

touch-php-secrets:
  file.managed:
    - name: /etc/yandex/secrets/touch-api-php.secrets
    - source: salt://{{slspath}}/files/etc/yandex/secrets/php.secrets
    - mode: 0640
    - user: www-data
    - group: www-data
    - makedirs: true
    - template: jinja
    - context:
      data: {{data|json}}
      yaenv: {{yaenv}}
      debug: {{debug_map[yaenv]}}
      prefix: {{env_prefix_map[yaenv]}}
      tvm_port: {{tvm_port}}
      js_api_key: {{js_api_key}}
      redis_cache_hosts: {{redis_cache_hosts}}


/home/www/touch-api.kinopoisk.ru/config/.env:
  file.symlink:
    - target: /etc/yandex/secrets/touch-api-php.secrets
    - force: true
    - makedirs: true
