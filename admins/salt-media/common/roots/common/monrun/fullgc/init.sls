{% set fullgc = pillar.get("fullgc", {}) %}

{% if "watched_dir" in fullgc %}

zk-flock-for-fullgc:
  pkg.installed:
    - pkgs:
      - zk-flock
      - jq
  file.managed:
    - name: /etc/distributed-flock-realtime-gc-jmapper.json
    - source:
      - salt://common/zk-flock/distributed-flock.json-{{grains["fqdn"]}}
      - salt://common/zk-flock/distributed-flock.json-{{grains.get("conductor:group")}}
      - salt://common/zk-flock/distributed-flock.json-{{grains.get("yandex-environment")}}
      - salt://common/zk-flock/distributed-flock.json
      - salt://common/monrun/fullgc/zk-flock-media-common.json-{{grains["fqdn"]}}
      - salt://common/monrun/fullgc/zk-flock-media-common.json-{{grains.get("conductor:group")}}
      - salt://common/monrun/fullgc/zk-flock-media-common.json-{{grains.get("yandex-environment")}}
      - salt://common/monrun/fullgc/zk-flock-media-common.json

fullgc:
  yafile.managed:
    - name: /usr/sbin/realtime-gc-monrun.sh
    - source: salt://common/monrun/fullgc/realtime-gc-monrun.sh
    - mode: 0755
  file.managed:
    - name: /etc/monitoring/realtime-gc-monrun.conf
    - source: salt://common/monrun/fullgc/realtime-gc-monrun.conf
    - template: jinja
    - context:
      fullgc: {{ fullgc }}
  monrun.present:
    - command: /usr/sbin/realtime-gc-monrun.sh {{ fullgc.watched_dir }}
    - execution_interval: 15
    - execution_timeout: 10
  pkg.installed:
    - name: inotify-tools

{% if "rsync_secrets" in fullgc %}
/etc/monitoring/realtime-gc-monrun.secrets:
  yafile.managed:
    - name: /etc/monitoring/realtime-gc-monrun.secrets
    - mode: 0400
    - source: salt://{{ fullgc.rsync_secrets }}
{% endif %}

{% else %}

fullgc.log pillar NOT DEFINED:
  cmd.run:
    - name: echo "No posible suitable defaults :-(";exit 1

{% endif %}
