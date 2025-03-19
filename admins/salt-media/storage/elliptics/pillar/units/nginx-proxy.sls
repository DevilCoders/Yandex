{%- set env =  grains['yandex-environment'] %}

nginx-proxy-files:
  - /etc/logrotate.d/mediastorage-proxy-conf
  - /etc/logrotate.d/collect_bucket_list

nginx-proxy-monrun-files:
  - /etc/monrun/conf.d/nginx.conf

nginx-proxy-dirs:
  - /etc/free_space_watchdog
  - /etc/nginx/Template
  - /etc/nginx/common
  - /etc/nginx/conf.d
  - /etc/nginx/include
  - /etc/nginx/sites-enabled
  - /etc/nginx/ssl
  - /etc/syslog-ng/conf-available
  - /var/cache/nginx/cache
  - /var/lib/yandex/fotki
  - /var/www/
  - /etc/nginx/tvm

nginx-proxy-helper-dirs:
  - /var/cache/yasm

nginx-proxy-exec-files:
  - /usr/local/bin/collect_bucket_list.py

nginx-proxy-config-files:
  {% if env != 'testing' %}
  - /etc/nginx/include/namespace-yt.conf
  {% endif %}
  - /etc/cron.d/collect_bucket_list
  - /etc/default/nginx
  - /etc/free_space_watchdog/free_space_watchdog.conf
  - /etc/logrotate.d/yandex-conf-comm-nginx
  - /etc/nginx/common/nocache_bypass.conf
  - /etc/nginx/conf.d/01-accesslog.conf
  - /etc/nginx/conf.d/01-mds-int-access-log-tskv.conf
  - /etc/nginx/conf.d/02-lua-init.conf
  - /etc/nginx/conf.d/map.conf
  - /etc/nginx/conf.d/mds_params.conf
  - /etc/nginx/fastcgi_params_ell
  - /etc/nginx/include/cache/range.conf
  - /etc/nginx/include/namespace-ext.conf
  - /etc/nginx/include/namespace-int.conf
  - /etc/nginx/include/proxy_options.conf
  - /etc/nginx/include/proxy_parameters.conf
  - /etc/nginx/include/upstream.conf
  - /etc/nginx/include/mds_yarl.lua
  - /etc/nginx/nginx.conf
  - /etc/nginx/ssl/certuml4.pem
  - /etc/nginx/ssl/https.conf
  - /usr/share/free_space_watchdog/nginx_stop.sh

nginx-proxy-template-files:
  - /etc/nginx/Template/template_proxycached.conf

syslog-ng-files:
  salt://units/nginx-proxy/files:
    - /etc/syslog-ng/conf-available/99-nginx.conf
    - /etc/syslog-ng/conf-available/40-int-nginx.conf
    - /etc/syslog-ng/conf-available/50-s3-nginx.conf
    - /etc/syslog-ng/conf-available/51-s3-private-nginx.conf

nginx-proxy-available-files:
  - /etc/nginx/sites-available/00-common.conf
  - /etc/nginx/sites-available/10-int-mediastorage.conf
  - /etc/nginx/sites-available/10-ext-mediastorage.conf
  - /etc/nginx/sites-available/10-storage.conf
  - /etc/nginx/sites-available/10-mulcagate.conf
  {% if env != 'testing' %}
  - /etc/nginx/sites-available/30-yt-mediastorage.conf
  {% endif %}
