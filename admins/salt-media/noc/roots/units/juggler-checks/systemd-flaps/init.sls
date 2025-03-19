/usr/local/share/salt/juggler_scripts//systemd-flaps.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/systemd-flaps.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_systemd-flaps/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
    - template: jinja
