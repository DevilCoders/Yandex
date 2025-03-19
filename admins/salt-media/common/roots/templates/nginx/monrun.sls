{% from slspath + "/map.jinja" import nginx with context %}


{% if nginx.get("monrun", {}).get("errorlog", {}).get("enabled") %}
yandex-media-nginx-errorlog-check:
  pkg.installed

/etc/monitoring/ngx-error-log-check.conf:
  file.managed:
    - user: root
    - group: root
    - makedirs: True
    - contents: |
        {{nginx.monrun.errorlog.config|d("", true)|indent(8)}}
{% endif %}
