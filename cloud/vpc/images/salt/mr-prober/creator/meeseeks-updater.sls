meeseeks-updater:
  file.managed:
    - names:
      - /etc/systemd/system/meeseeks-updater.service:
        - source: salt://{{ slspath }}/files/meeseeks-updater.service
        - template: jinja
        - context:
            version: "{{ pillar["mr_prober"]["versions"]["creator"] }}"
      - /etc/systemd/system/meeseeks-updater.timer:
        - source: salt://{{ slspath }}/files/meeseeks-updater.timer
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: meeseeks-updater
  service.enabled:
    - name: meeseeks-updater.timer
    - watch:
      - module: meeseeks-updater
