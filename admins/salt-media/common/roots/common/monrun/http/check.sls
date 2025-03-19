{% from slspath + "/map.jinja" import http_check,py_version with context %}

/usr/local/bin/http_check.py:
  file.managed:
    - mode: 0755
{% if py_version == "3" %}
    - source: salt://{{ slspath }}/http_check.py
{% else %}
    - source: salt://{{ slspath }}/http_check.py2
{% endif %}
