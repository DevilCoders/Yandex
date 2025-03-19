{% if "mongodb" in pillar %}

{% set mongodb = salt.pillar.get("mongodb") %}

/etc/mongo-backup.conf:
  yafile.managed:
    - source: salt://templates/mongodb-backup/files/config.tmpl
    - follow_symlinks : False
    - mode: 755
    - user: root
    - group: root
    - makedirs: true
    - template: jinja

/etc/distributed-flock-mongo-backup.json:
  file.managed:
    - follow_symlinks : False
    - source:
{% if mongodb.backup.zk["hosts"] %}
      - salt://templates/mongodb-backup/files/zk.tmpl
{%- endif %}
    - template: jinja
    - user: root
    - group: root
    - context:
      hosts: {{ mongodb.backup.zk["hosts"]|join(",") }}
    - mode: 644
    - makedirs: True
    - template: jinja

{% else %}

mongodb pillar NOT DEFINED:
  cmd.run:
    - name: echo "may be memory leaks on salt master!!!";exit 1

{% endif %}
