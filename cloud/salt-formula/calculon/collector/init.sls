include:
  - calculon

{{ sls }}_settings:
  file.recurse:
    - name: /usr/share/calculon/lib/python2.7/site-packages/collector/
    - source: salt://calculon/etc/
    - template: jinja

{{ sls }}_lib:
  file.recurse:
   - name: /usr/share/calculon/lib/python2.7/site-packages/collector/
   - source: salt://calculon/lib/

{{ sls }}_code:
  file.recurse:
    - name: /usr/share/calculon/lib/python2.7/site-packages/collector/
    - source: salt://{{ slspath }}/code/
    - require:
      - sls: calculon

{{ sls }}_uwsgi:
  file.managed:
    - name: /etc/uwsgi/apps-enabled/calculon-collector.ini
    - source: salt://{{ slspath }}/etc/uwsgi

{{ sls }}_srv_stop:
  cmd.run:
    - name: '/etc/init.d/uwsgi stop calculon-collector'
    - watch:
      - file: {{ sls }}_code
      - file: {{ sls }}_uwsgi

{{ sls }}_srv_start:
  cmd.run:
    - name: '/etc/init.d/uwsgi start calculon-collector'
    - watch:
      - cmd: {{ sls }}_srv_stop

{{ sls }}_nginx:
  file.managed:
    - name: /usr/share/gr_cfg/42calculon-collector.conf
    - source: salt://{{ slspath }}/etc/nginx
    - mode: 0644
  service.running:
    - name: nginx
    - reload: True
    - watch: 
      - file: {{ sls }}-nginx

{{ sls }}-log:
  file.touch:
    - name: /var/log/calculon/collector.log
    - makedirs: True
