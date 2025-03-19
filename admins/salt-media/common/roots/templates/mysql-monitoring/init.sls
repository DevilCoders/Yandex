/usr/sbin/mysql_slave_iptables.sh:
  file.managed:
    - source: salt://{{ slspath }}/monrun/mysql_slave_iptables.sh
    - mode: 0766

/etc/monrun/conf.d/mysql_slave_iptables.conf:
  file.managed:
    - source: salt://{{ slspath }}/monrun/mysql_slave_iptables.conf
    - mode: 0644

/usr/sbin/mysql-lock-check:
  file.managed:
    - source: salt://{{ slspath }}/monrun/mysql-lock-check
    - mode: 0766

/etc/monrun/conf.d/mysql-lock-check.conf:
  file.managed:
    - source: salt://{{ slspath }}/monrun/mysql-lock-check.conf
    - mode: 0644
