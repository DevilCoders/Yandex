tflow:
  yc_pkg.installed:
    - pkgs:
      - tflow

precheck_tflow_service_configuration:
  file.managed:
    - name: /etc/tflow/tflow.service.do_not_delete
    - source: salt://{{ slspath }}/files/lib/systemd/system/tflow.service.j2
    - template: jinja
    - makedirs: True
    - required:
      - yc_pkg: tflow

# 'configure_tflow_service' should be allowed to run only after tflow service is stoped
# bc only with old unite file it can remove loopback anouncement in case if loopback
# will be changed
# 'configure_tflow_service' will be runned in dry-test. Service will be stopped
# if test-run will return changed
stop_tflow_before_updating_service_file:
  service.dead:
    - name: tflow
    - onchanges:
      - file: precheck_tflow_service_configuration

configure_tflow_service:
  file.managed:
    - name: /lib/systemd/system/tflow.service
    - source: /etc/tflow/tflow.service.do_not_delete
    - makedirs: True
    - listen: stop_tflow_before_updating_service_file
    - required:
      - yc_pkg: tflow

service.systemctl_reload:
  module.wait:
    - watch:
      - file: configure_tflow_service

restart_tflow_service:
  service.running:
    - name: tflow
    - enable: True
    - watch:
      - yc_pkg: tflow
      - file: configure_tflow_service

remove_daily_logrotate:
  file.absent:
    - name: /etc/cron.daily/logrotate

run-logrotate-every-minute:
  cron.present:
    - name: pgrep logrotate >> /dev/null 2>&1 || /usr/sbin/logrotate /etc/logrotate.conf >> /dev/null 2>&1
    - user: root
