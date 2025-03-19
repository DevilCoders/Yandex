# Disable almost all of GCE daemons.
/lib/systemd/system/google-accounts-daemon.service:
  file.managed:
    - source: salt://services/google-accounts-daemon.service

google_clock_skew_daemon_service:
  service.disabled:
    - name: google-clock-skew-daemon.service

google_instance_setup_service:
  service.disabled:
    - name: google-instance-setup.service

google_network_daemon_service:
  service.disabled:
    - name: google-network-daemon.service
