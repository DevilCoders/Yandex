{% set component = "console" %}
{% set path = slspath if slspath.endswith('/bacula') else slspath.split('/')[0] %}
{% include path + "/common.sls" with context %}
