/etc/monitoring/la_per_core.conf:
  file.managed:
    - name: /etc/monitoring/la_per_core.conf
    - contents: |
        c1=3; c5=2.5; c15=2
        w1=2.5; w5=2; w15=1.5
    - user: root
    - group: root
    - mode: 644
