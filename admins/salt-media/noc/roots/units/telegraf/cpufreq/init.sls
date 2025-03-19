/usr/sbin/cpufreq.py:
  file.managed:
    - source: salt://{{ slspath }}/files/cpufreq.py
    - user: telegraf
    - group: root
    - mode: 755

/etc/telegraf/telegraf.d/cpufreq.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/cpufreq.conf
    - user: telegraf
    - group: root
    - mode: 755
