include:
  - units.netconfig_slb

solomon-proxy:
  pkg.installed:
    - version: 1.8.8920965
    - allow_updates: True
  service.running:
    - enable: True

influx-proxy:
  pkg.installed:
    - version: 1.8.8920965
    - allow_updates: True
  service.running:
    - enable: True

telegraf:
  service.running:
    - reload: True
    - require:
      - pkg: telegraf
  pkg:
    - installed
    - pkgs:
      - telegraf-noc-conf
      - telegraf

/etc/telegraf/telegraf.d/solomon-proxy.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/telegraf/telegraf.d/solomon-proxy.conf
    - template: jinja
    - user: telegraf
    - group: telegraf
    - mode: 660
    - makedirs: True

ngrep:
  pkg.installed: [ ]

grad:
  user.present:
    - name: grad
    - system: True

/etc/solomon_proxy/credentials.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/solomon_proxy/credentials.yaml
    - template: jinja
    - user: grad
    - group: grad
    - mode: 600
    - makedirs: True

nginx:
  pkg.installed: [ ]
  service.running:
    - enable: True

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/nginx.conf
    - template: jinja
    - mode: 660
    - makedirs: True

/etc/nginx/sites-enabled/grad.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/grad.conf
    - template: jinja
    - mode: 660
    - makedirs: True

/etc/nginx/ssl/grad.yandex.net.key:
  file.managed:
    - contents: {{ pillar['ssl_key'] | json }}
    - template: jinja
    - mode: 400
    - makedirs: True

/etc/nginx/ssl/grad.yandex.net.crt:
  file.managed:
    - contents: {{ pillar['ssl_cert'] | json }}
    - template: jinja
    - mode: 400
    - makedirs: True
