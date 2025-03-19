/usr/lib/yandex-graphite-checks/auth_expired.sh:
  file.absent
/usr/lib/yandex-graphite-checks/enabled/auth_expired.sh:
  file.absent
/usr/lib/yandex-graphite-checks/enabled/php_auth_expired.sh:
  file.managed:
    - source: salt://common/files/usr/lib/yandex-graphite-checks/enabled/php_auth_expired.sh
    - mode: 755
