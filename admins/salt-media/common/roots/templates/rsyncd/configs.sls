{% from slspath + "/map.jinja" import rsyncd with context %}

include:
  - .services

{% for share_name, share_opts in rsyncd.shares.items() %}
{% for opt in share_opts %}
{% for key, value in opt.items() %}
{% if key == 'path' %}
{{ value }}-share:
  file.directory:
    - name: {{ value }}
    - makedirs: True
{% endif %}
{% endfor %}
{% endfor %}
{% endfor %}

/etc/rsyncd.conf:
  file.managed:
    - source: {{ rsyncd.config }}
    - template: jinja
    - context:
      user: {{ rsyncd.user }}
      group: {{ rsyncd.group }}
      log: {{ rsyncd.log }}
      loglevel: {{ rsyncd.loglevel }}
      shares: {{ rsyncd.shares }}
    - makedirs: True
    - user: {{ rsyncd.user }}
    - group: {{ rsyncd.group }}
    - mode: 644
    - watch_in:
      - service: {{ rsyncd.service }}

/etc/default/rsync:
  file.managed:
    - source: {{ rsyncd.default_config }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - watch_in:
      - service: {{ rsyncd.service }}

{{ rsyncd.log }}:
  file.managed:
    - makedirs: True
    - user: {{ rsyncd.user }}
    - group: {{ rsyncd.group }}
    - mode: 644
    - watch_in:
      - service: {{ rsyncd.service }}

{{ rsyncd.logrotate_name }}:
  file.managed:
    - source: {{ rsyncd.logrotate }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
      log: {{ rsyncd.log }}
