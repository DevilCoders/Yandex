{% from slspath + "/map.jinja" import rsyncd with context %}
rsyncd:
  monrun.present:
    - command: /usr/bin/jrsync.sh
{% for mopt in rsyncd.get('mopts', '') %}
    - {{ mopt }}: {{ rsyncd['mopts'][mopt] }} 
{% endfor %}
