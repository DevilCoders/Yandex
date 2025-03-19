redis:
  pkg.installed:
    - name: redis-server
  service.running:
    - enable: True
    - reload: True