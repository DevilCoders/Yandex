/etc/juggler/client.conf:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://juggler/files/client.conf

{{ sls }}_packages:
  pkg.installed:
    - pkgs:
      - monrun
      - juggler-client
    - require:
      - file: /etc/juggler/client.conf

juggler-client:
  service.running:
    - enable: True
    - watch:
      - file: /etc/juggler/client.conf
    - require:
      - pkg: {{ sls }}_packages
