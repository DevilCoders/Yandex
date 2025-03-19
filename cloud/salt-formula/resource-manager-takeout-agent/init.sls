yc-rm-takeout-agent:
  yc_pkg.installed:
    - pkgs:
      - yc-rm-takeout-agent
  service.running:
    - enable: True
    - name: yc-rm-takeout-agent
    - watch:
      - yc_pkg: yc-rm-takeout-agent
      - file: /etc/yc/rm-takeout-agent/config.yaml
      - file: /etc/yc/rm-takeout-agent/server.pem

/etc/yc/rm-takeout-agent/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-rm-takeout-agent

/etc/yc/rm-takeout-agent/server.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub-certificate.pem
    - makedirs: True
    - replace: False
    - user: yc-rm-takeout-agent
    - group: yc-rm-takeout-agent
    - mode: 0400
