{% set unit = 'nginx' %}

#### remove

{{ unit }}-monrun-files:
  - /etc/monrun/conf.d/{{ unit }}.conf

#{{ unit }}-files:
#  - /etc/nginx/ssl/tls/avatars.mds.yandex.net.ticket
