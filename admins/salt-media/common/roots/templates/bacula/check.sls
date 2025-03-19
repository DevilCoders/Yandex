{% set path = slspath if slspath.endswith('/bacula') else slspath.split('/')[0] %}
{% set env = grains.get('yandex-environment', 'FAKE').replace('production', 'stable') %}

/usr/local/bin/bacula-check.sh:
  file.managed:
    - mode: 0755
    - source:
      - salt://{{ path }}/bacula-check.sh?saltenv={{ env }}
      - salt://{{ path }}/bacula-check.sh

bacula:
  monrun.present:
    - command: /usr/local/bin/bacula-check.sh
