/etc/chrony/chrony.conf:
  file.managed:
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - contents: |
        # man 5 chrony.conf for details
        pool ntp1.yandex.net iburst
        pool ntp2.yandex.net iburst
        pool ntp3.yandex.net iburst
        pool ntp4.yandex.net iburst
        keyfile /etc/chrony/chrony.keys
        driftfile /var/lib/chrony/chrony.drift
        logdir /var/log/chrony
        maxupdateskew 100.0
        rtcsync
        makestep 1 3

chrony:
  pkg.installed:
    - require:
      - file: /etc/chrony/chrony.conf
