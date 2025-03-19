{% from slspath + '/map.jinja' import mongodb,cluster with context %}

include:
  - .monrun-cfgsrv

/etc/mongodbcfg.conf:
  file.managed:
    - follow_symlinks : False
    - source:
      - salt://files/{{cluster}}/etc/mongodbcfg.conf
      - salt://{{cluster}}/etc/mongodbcfg.conf
      - salt://templates/mongodb/files/etc/mongodbcfg-vanilla.conf
    - template: jinja
    - user: root
    - group: root
    - context:
      mongodb: {{ mongodb }}
      cfg: {{ mongodb.get("mongodbcfg", {}) }}
    - mode: 644
    - makedirs: True

{% if mongodb.stock %}
{% if grains['oscodename'] == "xenial" %}
/etc/systemd/system/mongodbcfg.service:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/systemd/system/mongodbcfg.service
    - user: root
    - group: root
    - mode: 644
    - require:
      - file: /etc/mongodbcfg.conf
cleanup_/etc/init/mongodbcfg.conf:
  file.absent:
    - name: /etc/init/mongodbcfg.conf
{% else %}
/etc/init/mongodbcfg.conf:
  file.managed:
    - source: salt://templates/mongodb/files/etc/init/mongodbcfg.conf
    - follow_symlinks: False
    - replace: True
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - require:
      - file: /etc/mongodbcfg.conf
{% endif %}
{% endif %}

{{ mongodb.mongodbcfg.dbpath }}:
  file.directory:
    - user: mongodb
    - group: mongodb
    - mode: 0755
    - makedirs: True

mongodbcfg:
  service.running:
    - require:
      - file: /etc/mongodbcfg.conf
      - file: {{ mongodb.mongodbcfg.dbpath }}
