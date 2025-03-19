{% set unit = "solomon" %}

/etc/systemd/system/solomon.service:
  file.managed:
    - source: salt://{{ slspath }}/files/solomon.service

/etc/solomon/solomon.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/solomon.conf
    - template: jinja
    - makedirs: True

/etc/solomon/conf.d/:
  file.recurse:
    - source: salt://{{ slspath }}/files/conf.d/
    - makedirs: True

/etc/solomon/conf.d:
  file.directory:
    - makedirs: True

solomon:
  pkg.installed:
    - pkgs:
      - yandex-solomon-agent-bin
  service.running:
    - enable: True
