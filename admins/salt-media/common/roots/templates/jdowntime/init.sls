# from secure project repo
/etc/yandex/jdowntime.conf:
  file.managed:
    - makedirs: True
    {% if salt.pillar.get("jdowntime:token") %}
    - source: salt://{{ slspath }}/config.tpl
    - template: jinja
    - context:
      juggler_oauth_token: {{ salt.pillar.get("jdowntime:token") }}
    {% else %}
    - source: salt://jdowntime/config
    {% endif %}


jdowntime_pkg:
  pkg.installed:
    - pkgs:
      - yandex-media-jdowntime
