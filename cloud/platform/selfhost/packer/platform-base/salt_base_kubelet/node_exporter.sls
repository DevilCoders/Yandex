node_exporter_user:
  user.present:
    - name: node_exporter
    - fullname: Node Exporter
    - shell: /sbin/nologin
    - home: /home/node_exporter
    - system: True

extract_node-exporter:
  archive.extracted:
    - name: /tmp/node_exporter
    - source: https://s3.mds.yandex.net/mcdev/node_exporter-0.18.1.linux-amd64.tar
    - skip_verify: True

/usr/sbin/node_exporter:
  file.managed:
    - source: /tmp/node_exporter/node_exporter-0.18.1.linux-amd64/node_exporter
    - user: root
    - group: root
    - mode: 755
    - attrs: i

/etc/systemd/system/node_exporter.service:
  file.managed:
    - source: salt://services/node_exporter.service
    - user: root
    - group: root
    - mode: 644
    - attrs: i

/var/lib/node_exporter/textfile_collector:
  file.directory:
    - makedirs: True
    - user: root
    - group: root
    - mode: 755

/etc/default/node_exporter:
  file.managed:
    - source: salt://configs/node_exporter/node_exporter.default
    - user: root
    - group: root
    - mode: 644
    - attrs: i

service_node_exporter:
  service.running:
    - name: node_exporter
    - enable: True
