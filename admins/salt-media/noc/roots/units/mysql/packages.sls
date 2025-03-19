telegraf-package:
  pkg.installed:
    - name: telegraf

mysql-server:
  pkg.installed:
    - skip_suggestions: True

percona-xtrabackup:
  pkg.installed

mdb-mysync:
  pkg.installed:
    - skip_suggestions: True
    {% if grains['yandex-environment'] == 'production' %}
    - version: 1.8600031
    {% else %}
    - version: 1.9317370
    {% endif %}
    - cache_valid_time: 300
    - allow_updates: True

mycli:
  pkg.installed
