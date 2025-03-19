{% from slspath + '/map.jinja' import mongodb,cluster with context %}

include:
  - .monrun-mongos
  - .wd-mongos

/etc/mongos.conf:
  file.managed:
    - follow_symlinks : False
    - source:
      - salt://files/{{cluster}}/etc/mongos.conf
      - salt://{{cluster}}/etc/mongos.conf
      - salt://files/etc/mongos.conf
{% if mongodb.mongos["cfg-dbs"] %}
      - salt://templates/mongodb/files/etc/mongos-vanilla.conf
{%- endif %}
    - template: jinja
    - user: root
    - group: root
    - context:
      configdbs: {{ mongodb.mongos["cfg-dbs"]|join(",") }}
      key: {{"--keyFile={0}".format(mongodb.keyFile) if mongodb.get("keyFile") else ""}}
      mongodb: {{ mongodb }}
    - mode: 644
    - makedirs: True

{% if mongodb.stock %}
{% if grains['oscodename'] == "xenial" %}
/etc/systemd/system/mongos.service:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/systemd/system/mongos.service
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /etc/mongos.conf
cleanup_/etc/init/mongos.conf:
  file.absent:
    - name: /etc/init/mongos.conf
{% else %}
/etc/init/mongos.conf:
  file.managed:
    - source: salt://templates/mongodb/files/etc/init/mongos.conf
    - follow_symlinks: False
    - replace: True
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - require:
      - file: /etc/mongos.conf
{% endif %}
{% endif %}


mongos:
  service.running:
    - enable: true
    - require:
      - file: /etc/mongos.conf
