{{ sls }}-cfg:
  file.managed:
    - name: /home/robot-zoidberg/.conductor_client.ini
    - source: salt://{{ slspath }}/etc/conductor.ini
    - mode: 600

{{ sls }}-code:
  file.managed:
    - name: /usr/local/sbin/env_switcher
    - source: salt://{{ slspath }}/code
    - mode: 755
