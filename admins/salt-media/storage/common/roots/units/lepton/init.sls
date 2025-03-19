lepton:
  pkg.installed


{% for file in ['/etc/logrotate.d/lepton.conf', '/etc/ubic/service/lepton/daemon', '/etc/ubic/service/lepton/proxy'] %}
{{ file }}:
  file.managed:
    - source: salt://units/lepton/files{{ file }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
{% endfor %}

logs-dir:
  file.directory:
    - name: /var/log/lepton


mdslepton-yasmagent-conf:
  file.managed:
    - name: /usr/local/yasmagent/CONF/agent.mdslepton.conf
    - source: salt://units/lepton/files/usr/local/yasmagent/CONF/agent.mdslepton.conf
    - user: monitor
    - group: monitor
    - mode: 644

/etc/yandex/lepton/config.yaml:
  file.managed:
    - source: salt://units/lepton/files/etc/yandex/lepton/config.yaml
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
        port: {{ pillar.get("lepton-port", 8080) }}
