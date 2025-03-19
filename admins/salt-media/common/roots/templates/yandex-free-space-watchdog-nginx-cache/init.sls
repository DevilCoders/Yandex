/usr/share/free_space_watchdog/nginx_stop.sh:
  file.managed:
    - makedirs: true
    - name: /usr/share/free_space_watchdog/nginx_stop.sh
    - source: salt://{{ slspath }}/files/usr/share/free_space_watchdog/nginx_stop.sh
    - mode: 0700
    - user: root
