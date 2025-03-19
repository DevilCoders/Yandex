{{ sls }}_packages:
  yc_pkg.installed:
    - pkgs:
      - juggler-client
    - require:
      - file: /etc/juggler/client.conf

/etc/juggler/client.conf:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://common/mon/client.conf

juggler-client:
  service.running:
    - enable: True
    - watch:
      - file: /etc/juggler/client.conf
    - require:
      - yc_pkg: {{ sls }}_packages

