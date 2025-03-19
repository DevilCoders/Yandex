/etc/mongo-backup.conf:
  file.managed:
    - source: salt://templates/mongo-backup/mongo-backup.conf

/etc/distributed-flock-backup.json:
  file.managed:
    - source: salt://templates/mongo-backup/distributed-flock-backup.json

backup_packages:
  pkg.installed:
    - pkgs:
    {% for package in salt.conductor.package('corba-mongo-backup') or ['corba-mongo-backup'] %}
      - {{ package }}
    {% endfor %}
