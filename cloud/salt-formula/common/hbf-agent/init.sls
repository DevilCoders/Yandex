hbf-agent-pkg:
  yc_pkg.installed:
    - pkgs:
      - hbf-agent
  service.running:
    - name: hbf-agent
    - enable: true
    - require:
      - yc_pkg: hbf-agent-pkg
    - watch:
      - file: /usr/lib/hbf-agent/collect_ips.d/00-system
      - yc_pkg: hbf-agent-pkg

/usr/lib/hbf-agent/collect_ips.d/00-system:
  file.managed:
    - template: jinja
    - makedirs: True
    - mode: 0755
    - source: salt://{{ slspath }}/00-system
    - require:
      - yc_pkg: hbf-agent-pkg
