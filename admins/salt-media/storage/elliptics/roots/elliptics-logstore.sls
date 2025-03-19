{% if grains['conductor']['group'] != 'elliptics-logstore-new' %}
/etc/monitoring/unispace.conf:
  file.managed:
   - source: salt://files/elliptics-stat/unispace.conf
   - user: root
   - group: root
   - mode: 644

/etc/monrun/conf.d/push_server_mds.conf:
  file.managed:
   - source: salt://files/elliptics-stat/monrun_push_client_mds.conf
   - user: root
   - group: root
   - mode: 644
{% else %}
/etc/logstore-rotator.conf:
  file.managed:
   - source: salt://files/elliptics-stat/etc/logstore-rotator.conf
   - user: root
   - group: root
   - mode: 644

/etc/logrotate.d/logstore-rotator:
  file.managed:
   - source: salt://files/elliptics-stat/etc/logrotate.d/logstore-rotator
   - user: root
   - group: root
   - mode: 644

/etc/cron.d/logstore-rotator:
  file.managed:
   - source: salt://files/elliptics-stat/etc/cron.d/logstore-rotator
   - user: root
   - group: root
   - mode: 644

/usr/sbin/logstore-rotator.py:
  file.managed:
   - source: salt://files/elliptics-stat/usr/sbin/logstore-rotator.py
   - user: root
   - group: root
   - mode: 755
{% endif %}

include:
  - units.iface-ip-conf

xfs_frag:
  monrun.present:
    - command: "/usr/sbin/xfs_frag_monitoring.sh -w 30 -c 50 2>/dev/null"
    - execution_interval: 300
    - execution_timeout: 200

  file.managed:
    - name: /usr/sbin/xfs_frag_monitoring.sh
    - source: salt://files/elliptics-stat/usr/sbin/xfs_frag_monitoring.sh
    - user: root
    - group: root
    - mode: 755
