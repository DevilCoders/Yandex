{% from slspath + "/map.jinja" import http_check with context %}

/etc/monitoring/http_check.yaml:
  file.managed:
    - makedirs: True
      contents: |
        {{ http_check|yaml(False)|indent(8) }}

