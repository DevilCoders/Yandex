{% from slspath + "/map.jinja" import http_check,interval,timeout with context %}

{% for proto in http_check %}
{{ proto }}:
  monrun.present:
    - command: /usr/local/bin/http_check.py {{ proto }}
    - execution_interval: {{ interval }}
    - execution_timeout: {{ timeout|d(interval//2) }}
{% endfor %}
