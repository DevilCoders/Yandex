nginx_service:
  service.running:
    - name: nginx
    - enable: True
    - reload: True
