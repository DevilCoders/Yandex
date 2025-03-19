{% set unit = "nocauth" %}
include:
  - templates.certificates
  - units.nginx_conf

/etc/nginx/sites-enabled/nocauth-idm:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/nocauth-idm
    - template: jinja

/etc/nginx/sites-enabled/nocauth-ssh:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/nocauth-ssh
    - template: jinja

/etc/nginx/sites-enabled/nocauth-tacacs:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/nocauth-tacacs
    - template: jinja

/etc/yandex/unified_agent/conf.d/nocauth-metrics.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/conf.d/nocauth-metrics.yml
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - mode: 644
    - dir_mode: 750
    - makedirs: True

/etc/nocauth/:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/nocauth/
    - template: jinja
