{% set yaenv = grains['yandex-environment'] %}
include:
  - templates.certificates
  - templates.packages
  - templates.loggiver
  - templates.nginx
  - templates.memcached
#  - templates.tls_tickets
{% if yaenv not in [ 'stress' ] %}
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
