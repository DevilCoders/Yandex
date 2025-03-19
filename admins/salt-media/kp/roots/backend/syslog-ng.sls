{% set yaenv = grains['yandex-environment'] %}
{% if yaenv in ["development", "testing", "prestable", "production"] -%}

/etc/syslog-ng/conf.d/99-php.conf:
  file.absent
/etc/syslog-ng/conf.d/php.conf:
  file.absent

{% for config in [
  'yy-kinopoisk-php-perfile.conf',
  'zz-php-common.conf',
  'kinopoisk-health.conf',
  'kinopoisk-metrics.conf',
  'kinopoisk-awaps.conf',
  'kinopoisk-awaps-gyt.conf',
  'kinopoisk-bunker-gyt.conf',
  'kinopoisk-vote-gyt.conf',
  'kinopoisk-search.conf',
  'mail_short.conf',
  'kinopoisk-parser.conf'
] %}

/etc/syslog-ng/conf.d/{{ config }}:
  file.managed:
    - template: jinja
    - source: salt://common/files/syslog-ng/{{ config }}
    - watch_in:
      - service: syslog-ng
    - context:
      yaenv: {{ yaenv }}

{% endfor %}

# CADMIN-6873
/etc/syslog-ng/syslog-ng.tpl:
  file.managed:
    - source: salt://common/files/syslog-ng/syslog-ng.tpl
    - watch_in:
      - service: syslog-ng

syslog-ng:
  service.running:
    - enable: true
    - reload: true

{% endif -%}
