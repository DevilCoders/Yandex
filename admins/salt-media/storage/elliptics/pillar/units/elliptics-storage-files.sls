{%- set env =  grains['yandex-environment'] %}

elliptics-storage-files-755:
  - /usr/local/bin/g3Xflash
  - /usr/bin/spacemimic2tskv.py

elliptics-storage-files-644:
  - /etc/monitoring/watchdog.conf
  - /etc/monitoring/la.conf
  - /etc/monitoring/hw_errs.conf
  - /etc/ipmi_memory/ipmi_memory.conf

elliptics-storage-conf-files:
  - /etc/elliptics/cocaine-srw.conf
  - /etc/elliptics/elliptics.conf
  - /etc/elliptics/masters.id
  - /etc/monitoring/unispace.conf
  - /etc/elliptics/elliptics-node.template
  - /etc/elliptics/handystats.json
  - /etc/rsyncd.conf
  - /etc/logrotate.d/syslog-ng

elliptics-storage-exec-files:
  - /usr/bin/eblob_check.py
  - /etc/cron.daily/config-mds-storage
  - /usr/bin/reuse_ssd.sh

elliptics-storage-secrets-files:
  - /etc/rsyncd.secrets

elliptics-storage-conf-dirs:
  - /etc/elliptics/
  - /etc/elliptics/ssl
  - /etc/monitoring/
  - /etc/default/
