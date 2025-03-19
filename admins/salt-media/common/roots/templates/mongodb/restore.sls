{% from slspath + "/map.jinja" import mongodb with context %}

/etc/yandex/mongo-restore.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{ slspath }}/files/etc/yandex/mongo-restore.tmpl
    - template: jinja
    - context:
        config: {{ mongodb.restore.config }}

# subscribe for deploy list
yandex-media-mongo-restore:
  pkg.installed
# backward compatibility with old states
/usr/bin/mongo-restore.sh:
  file.symlink:
    - force: True
    - target: /usr/sbin/mongodb-restore.sh
old_binary_for_restore_sharded_clusters:
  file.managed:
    - names:
      - /opt/bin/mongod_3_2:
        - source: http://s3.mds.yandex.net/music/salt/mongo/mongod_3.2
        - source_hash: ba590ae17f9f50db41cd9750ad8d8061
      - /opt/bin/mongo_3_2:
        - source: http://s3.mds.yandex.net/music/salt/mongo/mongo_3.2
        - source_hash: ed8d8c823fb3411cd026b3172ac3c663
    - makedirs: True
    - mode: 755
/etc/cron.d/mongodb-restore:
  file.absent
# Backward compatibility END

/var/cache/mongo-restore:
  file.directory:
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True

{% if mongodb.restore.schedule %}
/etc/cron.d/mongo-restore:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/etc/cron.d/mongo-restore.tmpl
    - template: jinja
    - context:
        schedule: {{ mongodb.restore.schedule }}
{% endif %}
