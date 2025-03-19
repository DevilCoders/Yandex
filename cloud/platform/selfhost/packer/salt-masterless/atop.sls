atop:
  pkg.installed:
    - pkgs:
      - atop
    - allow_fallback_to_global_pinning: True
  file.managed:
    - name: /etc/default/atop
    - source: salt://{{ slspath }}/atop/atop
    - require:
      - pkg: atop
  service.running:
    - enable: True
    - require:
      - pkg: atop
      - file: atop
    - watch:
      - file: atop
daily_atop_rotation:
  file.managed:
    - name: /etc/logrotate.d/atop
    - source: salt://{{ slspath }}/atop/daily_rotation.cfg
    - require:
      - pkg: atop
