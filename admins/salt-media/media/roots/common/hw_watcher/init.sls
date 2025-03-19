/etc/hw_watcher/conf.d/token.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - contents: |
        [bot]
        oauth_token={{ salt['pillar.get']('hw_watcher:token') }}
        initiator={{ salt['pillar.get']('hw_watcher:user') }}

{% if salt['pillar.get']('hw_watcher:monitor_logs', False) %}
/usr/bin/monrun-hw-watcher-bot-auth.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - source: salt://{{slspath}}/monrun-hw-watcher-bot-auth.sh
    - makedirs: True
/etc/monrun/conf.d/hw-watcher-bot-auth.conf:
  file.managed:
    - contents: |
        [hw-watcher-bot-auth]
        command = /usr/bin/monrun-hw-watcher-bot-auth.sh
        execution_interval = 90
        execution_timeout = 80
{% endif%}
