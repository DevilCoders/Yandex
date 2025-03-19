/etc/monrun/salt_logrotate/logrotate.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_logrotate/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

{% if grains["osmajorrelease"] < 20 %}
logrotate:
  pkg.installed:
    - version: '>=3.14.0-14-gfcb65b0-yandex'
{% endif %}
