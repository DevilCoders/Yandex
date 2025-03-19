uwsgi:
  yafile.managed:
    - name: /etc/nginx/sites-available/uwsgi.conf
    - source: salt://{{ slspath }}/uwsgi.conf
    - makedirs: True
    - mode: 0644
  file.symlink:
    - name: /etc/nginx/sites-enabled/uwsgi.conf
    - target: /etc/nginx/sites-available/uwsgi.conf
    - makedirs: True
    - force: True
    - require:
      - yafile: /etc/nginx/sites-available/uwsgi.conf

/var/tmp/nginx/client-temp:
  file.directory:
    - user: www-data
    - group: root
    - dir_mode: 755
    - makedirs: True
    - order: 0
