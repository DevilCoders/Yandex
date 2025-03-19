{% from slspath + '/map.jinja' import mongodb,cluster with context -%}

{%- if mongodb.enable_daytime %}
/etc/xinetd.d/daytime-media:
  file.managed:
    - source: salt://templates/mongodb/files/etc/xinetd.d/daytime-media
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - watch_in:
      - service: xinetd

xinetd:
  pkg:
    - installed
  service:
    - running

daytime:
  monrun.present:
    - type: common
    - execution_timeout: '30'
    - execution_interval: 60
    - command: nc localhost daytime &>/dev/null&&echo '0;OK;daytime service running'||echo '2;FAIL;daytime service not available'
{%- endif %}
