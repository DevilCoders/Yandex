patch-skm-restart-policy:
  file.line:
    - name: /lib/systemd/system/skm.service
    - mode: ensure
    - after: "\\[Service\\]"
    - content: "Restart=on-failure"
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: patch-skm-restart-policy
