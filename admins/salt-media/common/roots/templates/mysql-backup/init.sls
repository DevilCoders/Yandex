/etc/mysql-backup.conf:
  file.managed:
    - contents: |
        host = mysql-backup.{{ grains['conductor']['project'] }}.yandex.net

backup_packages:
  pkg.installed:
    - pkgs:
    {% for package in salt['conductor.package']('mysql-backup') %}
      - {{ package }}
    {% endfor %}
