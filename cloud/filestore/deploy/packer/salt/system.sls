Template for cloud-init to form /etc/hosts:
  file.managed:
    - makedirs: True
    - name: /etc/cloud/templates/hosts.debian.tmpl
    - source: salt://system/hosts.debian.tmpl
