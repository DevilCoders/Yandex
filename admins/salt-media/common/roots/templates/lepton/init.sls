lepton:
  pkg.installed


{% for file in ['/etc/logrotate.d/lepton.conf', '/etc/ubic/service/lepton/daemon', '/etc/ubic/service/lepton/proxy'] %}
{{ file }}:
  file.managed:
    - source: salt://templates/lepton/files{{ file }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
{% endfor %}

logs-dir:
  file.directory:
    - name: /var/log/lepton


/etc/yandex/lepton/config.yaml:
  file.managed:
    - source: salt://templates/lepton/files/etc/yandex/lepton/config.yaml
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
        port: {{ pillar.get("lepton-port", 7070) }}

local_lepton:
  monrun.present:
    - command: "/usr/bin/http_check.sh ping {{ pillar.get("lepton-port", 7070) }}"
    - execution_interval: 60
    - type: lepton
