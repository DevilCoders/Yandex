yc-iam-takeout-agent:
  yc_pkg.installed:
    - pkgs:
      - yc-iam-takeout-agent
  service.running:
    - enable: True
    - name: yc-iam-takeout-agent
    - watch:
      - yc_pkg: yc-iam-takeout-agent
      - file: /etc/yc/iam-takeout-agent/config.yaml
      - file: /etc/yc/iam-takeout-agent/server.pem

/etc/yc/iam-takeout-agent/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - user: root
    - group: root
    - require:
      - yc_pkg: yc-iam-takeout-agent

/etc/yc/iam-takeout-agent/server.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub-certificate.pem
    - makedirs: True
    - replace: False
    - user: yc-iam-takeout-agent
    - group: yc-iam-takeout-agent
    - mode: 0400
