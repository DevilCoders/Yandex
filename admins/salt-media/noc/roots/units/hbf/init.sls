{% set unit = "hbf" %}
{% if "-test-" in grains["fqdn"] %}
{% set dc = grains["fqdn"].split("-")[0] %}
{% else %}
{% set dc = grains["conductor"]["group"] | regex_search('nocdev-hbf-(.*)') | first %}
{% endif %}

include:
  - units.nginx_conf
  - units.juggler-checks.common
  - units.juggler-checks.hbf
  - templates.certificates
  - .logstat

hbf-user:
  user.present:
    - name: hbf
    - home: /home/hbf
hbf:
  group.present:
    - addusers:
      - hbf

/etc/cron.d/update_geo.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/update_geo.conf

/etc/hbf/.env:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/hbf/.env
    - template: jinja
    - makedirs: True
    - mode: 600
    - user: hbf
    - dir_mode: 755

/etc/hbf/config.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/hbf/config.ini
    - template: jinja
    - context:
        dc: {{dc}}
    - makedirs: True
    - user: hbf
    - dir_mode: 755

/var/cache/hbf:
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - opts: defaults,size=500M
    - mount: True
    - mkmnt: True

/etc/logrotate.d/hbf.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/hbf.conf

{%- if grains['yandex-environment'] != 'testing' and 'validator' not in grains['conductor']['group'] %}
yandex-push-client:
  pkg.installed:
    - pkgs:
      - yandex-push-client
  service.running:
    - name: statbox-push-client
    - enable: True
  file.directory:
    - name: /var/lib/nginx-access
    - mode: 755
    - makedirs: True
    - user: statbox
/etc/default/push-client:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/default/push-client
/etc/yandex/statbox-push-client/nginx-access.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/statbox-push-client/nginx-access.yaml
/etc/logrotate.d/push-client:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/push-client
/etc/yandex/statbox-push-client/.tvm_secret:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/statbox-push-client/.tvm_secret
    - template: jinja
{%- endif %}


/etc/nginx/conf.d/connlimit.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/conf.d/connlimit.conf
    - template: jinja
/etc/nginx/conf.d/hbf-tskv.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/conf.d/hbf-tskv.conf

/etc/systemd/system/hbf-check.service:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/systemd/system/hbf-check.service
    - template: jinja
    - context:
        dc: {{dc}}
  service.running:
    - name: hbf-check
    - enable: True

/usr/sbin/build_geoconf.py:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/sbin/build_geoconf.py
    - mode: 755
/usr/sbin/hbf-check.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/sbin/hbf-check.sh
    - mode: 755

m4:
  pkg.installed:
    - pkgs:
      - m4

/etc/nginx/sites-enabled/10-main.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/10-main.conf
    - template: jinja
/etc/nginx/sites-enabled/20-hbf.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/20-hbf.conf
    - template: jinja
    - context:
        dc: {{dc}}
{%- if grains['yandex-environment'] == 'testing' %}
/etc/nginx/sites-enabled/30-hbf-test.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/30-hbf-test.conf
{%- endif %}
{%- if 'validator' in grains['conductor']['group'] %}
/etc/nginx/sites-enabled/30-hbf-validator.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/30-hbf-validator.conf
    - template: jinja
{%- endif %}
