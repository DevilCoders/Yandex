/etc/apparmor.d/local/usr.sbin.unbound:
  file.managed:
    - source: salt://files/bionic-fixes/etc/apparmor.d/local/usr.sbin.unbound
    - user: root
    - group: root
    - mode: 644
