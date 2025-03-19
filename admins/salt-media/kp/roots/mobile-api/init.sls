{% set yaenv = grains['yandex-environment'] %}
include:
  - templates.packages
  - templates.memcached
{% if yaenv in ['production', 'prestable', 'testing'] %}
  - templates.loggiver
  - templates.push-client
{% endif %}
  - common.haproxy
  - backend.syslog-ng
  - .nginx
  - common.nginx_configs
  - .monrun
  - .pingunoque
  - .secrets
  - common.yandex-graphite-checks.php-auth-expired
  - .packages
