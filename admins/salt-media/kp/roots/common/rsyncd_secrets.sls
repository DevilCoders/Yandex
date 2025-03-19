rsyncd_secrets:
  file.managed:
    - name: /etc/rsyncd.secrets
    - source: salt://common/secure/files/rsyncd.secrets
    - mode: 600
    - template: jinja
