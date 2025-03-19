{% set is_prod = grains["yandex-environment"] in ["production"] %}

include:
  - templates.push-client
  - templates.certificates
  - templates.zk_client
  - templates.loggiver
  - templates.yandex-free-space-watchdog-nginx-cache
  - templates.nginx
  - templates.packages
  - common.yandex-graphite-checks.php-auth-expired
  - common.php.secrets
  - common.haproxy
  - backend.php
  - tvmtool
  - .php_repo
  - backend.syslog-ng
  - .monrun
  - .custom_files
  - .flyway
  - .nginx
  - .scripts
{% if is_prod %}
  - common.rsyncd_secrets
  - templates.rsyncd
  - .rsync_static
{% endif %}
  - .lsyncd
  - .pingunoque
  - .s3_log_dir
  - .logrotate
  - .images_to_s3_hourly
  - .php73
  - .perm
  - .hosts
  - .coredump_monitor
