{% set yaenv = grains['yandex-environment'] %}
  include:
  - templates.packages
{% if yaenv in ['production', 'prestable', 'testing'] %}
  - templates.loggiver
  - templates.push-client
{% endif %}
# TODO: resove this case, make testing as prod ? NO. tinyproxy going to be deprecated and should not be on nonproduction envs. THIS! SHIT! GONNA! DIE!
  - .tinyproxy
  - .monrun
  - .nginx
  - common.nginx_configs
  - .syslog-ng
  - .logrotate
  - .custom_files
  - .pingunoque
  - common.haproxy
  - common.php.secrets
  - common.yandex-graphite-checks.php-auth-expired
  - .s3_log_dir
