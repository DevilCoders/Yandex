telegraf:
  user.present:
    - home: /etc/telegraf
    - createhome: True
    - shell: /sbin/nologin
    - system: True

/etc/telegraf/telegraf.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/telegraf.conf
    - user: telegraf
    - group: telegraf
    - mode: 0640
    - template: jinja
    - require:
      - user: telegraf

/var/log/telegraf:
  file.directory:
    - user: telegraf
    - group: telegraf
    - mode: 0755
    - require:
      - user: telegraf

mdb-telegraf:
  pkg.installed:
    - version: '2.139.255560889'
    - prereq_in:
      - cmd: repositories-ready

/lib/systemd/system/telegraf.service:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/telegraf.service
    - template: jinja
    - require_in:
      - service: telegraf
    - require:
      - pkg: mdb-telegraf
      - file: /etc/telegraf/telegraf.conf
      - file: /var/log/telegraf
    - onchanges_in:
      - module: systemd-reload

telegraf.service:
  service.running:
    - name: telegraf
    - enable: True
    - watch:
      - file: /etc/telegraf/telegraf.conf
      - pkg: mdb-telegraf
{% if salt['pillar.get']('data:cluster_private_key') %}
      - file: cluster-key

cluster-key:
  file.managed:
    - name: /etc/telegraf/cluster_key.pem
    - mode: '0640'
    - user: telegraf
    - group: telegraf
    - template: jinja
    - contents_pillar: data:cluster_private_key
{% endif %}

