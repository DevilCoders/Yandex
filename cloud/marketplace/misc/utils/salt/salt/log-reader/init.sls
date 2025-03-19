yc-log-reader-pkg:
  pkg.installed:
    - name: yc-log-reader

/etc/yc/log-reader/config.yaml:
  file.managed:
    - user: yc-log-reader
    - group: yc-log-reader
    - mode: 0644
    - source: salt://{{ slspath }}/files/yc-log-reader.conf
    - template: jinja
    - defaults:
      tvm_id: {{ pillar['security']['log_reader_tvm']['client_id'] }}
      topic: {{ pillar['common']['log-reader']['topic'] }}
    - require:
      - pkg: yc-log-reader-pkg

/etc/yc/log-reader/secret.txt:
  file.managed:
    - source: salt://{{ slspath }}/files/secret.txt
    - template: jinja
    - user: yc-log-reader
    - mode: 600
    - defaults:
      secret: {{ pillar['security']['log_reader_tvm']['secret'] }}
    - require:
      - pkg: yc-log-reader-pkg

yc-log-reader:
  service.running:
    - enable: true
    - watch:
      - file: /etc/yc/log-reader/config.yaml
      - pkg: yc-log-reader
