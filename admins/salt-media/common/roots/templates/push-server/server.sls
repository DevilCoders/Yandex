{% from "templates/push-server/map.jinja" import push_server with context %}

include:
  - templates.push-server
  - templates.push-server.monrun

push_server_daemon:
  service:
    - name: {{ push_server.service }}
    - enable: True
    - running

push_server_conf:
  file:
    - managed
    - name: {{ push_server.filename }}
    - source:
      - salt://files/{{ grains['conductor']['group'] }}{{ push_server.filename }}
      - {{ push_server.filesource }}
    - template: jinja
    - context:
      logfile: {{ push_server.logger.file }}
      loglevel: {{ push_server.logger.level }}
      netport: {{ push_server.network.port }}
      nethost: {{ push_server.network.host }}
      rootpath: {{ push_server.storage.path }}
      rotate: {{ push_server.storage.rotate }}
      filescount: {{ push_server.storage.filescount }}
      compress: {{ push_server.storage.compress }}
    - user: root
    - watch_in:
      - service: push_server_daemon

push_server_default:
  file:
    - managed
    - name: {{ push_server.defaultfilename }}
    - source:
      - salt://files/{{ grains['conductor']['group'] }}{{ push_server.defaultfilename }}
      - {{ push_server.defaultfilesource }}
    - template: jinja
    - context:
      user: {{ push_server.user }}
      group: {{ push_server.group }}
    - user: root
    - watch_in:
      - service: push_server_daemon

push_server_logrotate:
  file:
    - managed
    - name: /etc/logrotate.d/push-server
    - source:
      - salt://files/{{ grains['conductor']['group'] }}/etc/logrotate.d/push-server
      - {{ push_server.logrotatesource }}
    - user: root

{{ push_server.storage.path }}:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
