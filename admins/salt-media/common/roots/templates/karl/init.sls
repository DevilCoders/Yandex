include:
  - templates.karl-tls
  - .ubic

/etc/karl/karl.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/karl/karl.yaml
    - user: karl
    - group: karl
    - mode: 644
    - template: jinja
    - makedirs: true

/etc/logrotate.d/karl:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/karl
    - mode: 644
    - makedirs: true

/etc/karl/allCAs.pem:
  file.managed:
    - contents_pillar: tls_karl:karl_sslrootcert
    - mode: 644
    - user: karl
    - group: karl
    - makedirs: true

/etc/sudoers.d/karl-status:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/sudoers.d/karl-status
    - makedirs: True
    - mode: 440
    - user: root
    - group: root

karl-status:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/bin/karl-status.sh
    - name: /usr/bin/karl-status.sh
    - mode: 755
    - makedirs: true

  monrun.present:
    - command: "/usr/bin/karl-status.sh"
    - execution_interval: 60
    - execution_timeout: 20
    - type: karl
    - makedirs: True
