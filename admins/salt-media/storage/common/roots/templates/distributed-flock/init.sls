{% set unit = 'distributed-flock' %}

/etc/distributed-flock.json:
  file.managed:
    - source: salt://{{ slspath }}/files/distributed-flock.json
    - template: jinja
