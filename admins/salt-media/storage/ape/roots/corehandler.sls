#corehandler:
#  pkg.installed:
#    - pkgs:
#      - corehandler: 0.1.8.2
#      - coregrabber-service-ubic: 0.1.8.2
#    - skip_suggestions: True
#    - install_recommends: False
#
#kernel.core_pattern:
#  sysctl.present:
#    - name: "kernel.core_pattern"
#    - value: "|/usr/bin/corehandler %P %s %h %c %t %e"
#    - config: "/etc/sysctl.d/ZZ-corehandler.conf"
#
#kernel.core_pipe_limit:
#  sysctl.present:
#    - name: "kernel.core_pipe_limit"
#    - value: 20
#    - config: "/etc/sysctl.d/ZZ-corehandler.conf"
#
#/etc/logrotate.d/corehandler:
#  file.managed:
#    - source: salt://conf/pkg/corehandler/etc/logrotate.d/corehandler
#
# saltmaster.tst.ape.yandex.net:/srv/salt/roots/coreinfra/etc/corehandler.conf
#/etc/corehandler.conf:
#  file.managed:
#    - source: "salt://coreinfra/etc/corehandler.conf"
#
#/etc/monrun/conf.d/corehandler.conf:
#  file.managed:
#    - source: salt://conf/pkg/corehandler/etc/monrun/conf.d/corehandler.conf
#
#/usr/lib/config-monitoring-common/corehandler.py:
#  file.managed:
#    - source: salt://conf/pkg/corehandler/usr/lib/config-monitoring-common/corehandler.py
#    - mode: 755
#
#/usr/lib/yandex-graphite-checks/enabled/corehandler.py:
#  file.managed:
#    - source: salt://conf/pkg/corehandler/usr/lib/config-monitoring-common/corehandler.py
#    - mode: 755

