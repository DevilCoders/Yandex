{%- from slspath + "/map.jinja" import mm_vars with context -%}
{% set cluster = pillar.get('cluster') %}
{%- set env =  grains['yandex-environment'] %}

NSCFG_CONFIG:
  file.append:
    - name: /etc/profile.d/nscfg.sh
    - text: export NSCFG_CONFIG=/etc/nscfg/nscfg-cli.conf

/etc/elliptics/nscfg-server.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/nscfg-server.conf
    - template: jinja
    - context:
      vars: {{ mm_vars }}

/etc/ubic/service/nscfg-server:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/nscfg-server
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/init.d/nscfg-server:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/init.d/nscfg-server
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/nscfg-server:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/nscfg-server
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/nscfg-nginx:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/nscfg-nginx
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/var/log/nscfg-server:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/etc/elliptics/nscfg-server-ng.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/nscfg-server-ng.conf
    - template: jinja

/etc/ubic/service/nscfg-server-ng:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/nscfg-server-ng
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/init.d/nscfg-server-ng:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/init.d/nscfg-server-ng
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/nscfg-server-ng:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/nscfg-server-ng
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/var/log/nscfg-server-ng:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

{%- if env == 'testing' %}
/etc/elliptics/nscfg-server-ng-test.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/nscfg-server-ng-test.conf
    - template: jinja

/etc/ubic/service/nscfg-server-ng-test:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ubic/service/nscfg-server-ng-test
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/init.d/nscfg-server-ng-test:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/init.d/nscfg-server-ng-test
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/nscfg-server-ng-test:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/nscfg-server-ng-test
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/var/log/nscfg-server-ng-test:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/var/cache/nscfg-server-ng-test:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{%- endif %}

nscfg-server:
  monrun.present:
    - command: "/usr/bin/http_check.sh ping 9532"
    - execution_interval: 60
    - execution_timeout: 10
    - type: mastermind
    - makedirs: True

/etc/nginx/sites-available/10-nscfg.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-available/10-nscfg.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: true
    - template: jinja
    - watch_in:
      - service: nginx_nscfg

/etc/nginx/sites-enabled/10-nscfg.conf:
  file.symlink:
    - target: /etc/nginx/sites-available/10-nscfg.conf
    - makedirs: true
    - require:
      - file: /etc/nginx/sites-available/10-nscfg.conf

/etc/nscfg:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/etc/nscfg/nscfg-cli.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/nscfg-cli.conf
    - template: jinja
    - context:
      vars: {{ mm_vars }}

/var/cache/nscfg-server-ng:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

nscfg-err-log:
  monrun.present:
    - command: "monrun-resizer-and-reporter.sh -l /var/log/nscfg-server/server.log,/var/log/nscfg-server/access.log,/var/log/nscfg-server-ng/server.log,/var/log/nscfg-server-ng/access.log"
    - execution_interval: 600
    - execution_timeout: 30


nginx_nscfg:
  service:
    - name: nginx
    - running
    - reload: True
