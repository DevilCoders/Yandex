/etc/default/cpufrequtils:
  file.managed:
    - contents: 'GOVERNOR=performance'
    - require:
      - yc_pkg: cpufrequtils

cpufrequtils:
  yc_pkg.installed:
    - name: cpufrequtils
  service.running:
    - enable: True
    - watch:
      - file: /etc/default/cpufrequtils
      - yc_pkg: cpufrequtils

ondemand:
  service.dead:
    - enable: False
    - require:
      - yc_pkg: cpufrequtils
