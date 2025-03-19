/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx.conf
