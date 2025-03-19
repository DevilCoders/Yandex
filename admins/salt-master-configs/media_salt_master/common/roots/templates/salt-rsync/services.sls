{% from slspath + "/map.jinja" import salt_rsync with context %}

rsync_server:
  pkg.installed:
    - name: {{ salt_rsync.package }}
  service.running:
    - name: {{ salt_rsync.service }}
    - enable: True
