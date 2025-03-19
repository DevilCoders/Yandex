coreface:
  pkg.installed:
    - pkgs:
      - coreface: 0.0.4
      - coreface-service-runit: 0.0.4
    - skip_suggestions: True
    - install_recommends: False

/etc/coreface.conf:
  file.managed:
    - source: salt://coreinfra/etc/coreface.conf

/etc/monrun/conf.d/mds_ns_check.conf:
  file.managed:
    - source: salt://conf/pkg/coreface/etc/monrun/conf.d/mds_ns_check.conf

/usr/lib/config-monitoring-common/mds_ns_check.py:
  file.managed:
    - source: salt://conf/pkg/coreface/usr/lib/config-monitoring-common/mds_ns_check.py
    - mode: 755

/usr/lib/yandex-graphite-checks/enabled/mds_ns_check.sh:
  file.managed:
    - source: salt://conf/pkg/coreface/usr/lib/config-monitoring-common/mds_ns_check.sh
    - mode: 755


