nginx:
  pkg:
    - installed
  file.managed:
    - name: /etc/nginx/sites-enabled/default
    - source: salt://{{ slspath }}/files/nginx.conf
  service.running:
    - watch:
      - pkg: nginx
      - file: nginx
