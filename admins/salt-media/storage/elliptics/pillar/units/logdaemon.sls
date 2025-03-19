{%- set env =  grains['yandex-environment'] %}

logdaemon-exec-files:
  - /usr/local/bin/logdaemon.py
  - /etc/init.d/logdaemon

logdaemon-initd-links:
  - /etc/rc0.d/K21logdaemon
  - /etc/rc1.d/K21logdaemon
  - /etc/rc2.d/S21logdaemon
  - /etc/rc3.d/S21logdaemon
  - /etc/rc4.d/S21logdaemon
  - /etc/rc5.d/S21logdaemon
  - /etc/rc6.d/K21logdaemon

logdaemon-conf-files:
  - /etc/logrotate.d/logdaemon
  - /etc/monrun/conf.d/daemon_check_logdaemon.conf
  - /etc/logdaemon/config.yaml

logdaemon-dirs:
  - /var/log/logdaemon
