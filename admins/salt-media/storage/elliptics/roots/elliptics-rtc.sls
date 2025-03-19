# MDS-12068: Deprecated. Present as example.
include:
  - templates.unistat-lua

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://elliptics-rtc/etc/monrun/conf.d/
    - file_mode: '0644'

/etc/elliptics/elliptics.conf:
  file.managed:
    - source: salt://elliptics-rtc/etc/elliptics/elliptics.conf
    - makedirs: True

/usr/local/bin:
  file.recurse:
    - source: salt://elliptics-rtc/usr/local/bin/
    - file_mode: '0755'

/etc/yandex/statbox-push-client:
  file.recurse:
    - source: salt://elliptics-rtc/etc/yandex/statbox-push-client/
    - file_mode: '0644'

/usr/local/bin/qloud-uptime.py:
  file.managed:
    - source: salt://common-files/usr/local/bin/qloud-uptime.py
    - file_mode: '0755'

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://elliptics-rtc/etc/cocaine/cocaine.conf
    - makedirs: True

/etc/elliptics/elliptics-rtc.conf:
  file.managed:
    - source: salt://elliptics-rtc/etc/elliptics/elliptics-rtc.conf
    - makedirs: True

/etc/elliptics/spacemimic.conf:
  file.managed:
    - source: salt://elliptics-rtc/etc/elliptics/spacemimic.conf
    - makedirs: True

/etc/cocaine/mecoll.yaml:
  file.managed:
    - source: salt://elliptics-rtc/etc/cocaine/mecoll.yaml
    - makedirs: True

/usr/local/sbin/runit-flaps.sh:
  file.managed:
    - source: salt://elliptics-rtc/usr/local/sbin/runit-flap.sh
    - mode: 755


/etc/runit/1:
  file.managed:
    - source: salt://elliptics-rtc/etc/runit/1
    - mode: 755
    - makedirs: True

/etc/runit/2:
  file.managed:
    - source: salt://elliptics-rtc/etc/runit/2
    - mode: 755
    - makedirs: True

/etc/runit/3:
  file.managed:
    - source: salt://elliptics-rtc/etc/runit/3
    - mode: 755
    - makedirs: True

/etc/service:
  file.directory

{% set svservices = ['cocaine-runtime','elliptics','elliptics-stat','spacemimic','rtc-helper','juggler','mastermind-minion','mecoll','statbox-push-client','loggiver','mr_proper','nginx','mds-ops-agent'] %}
{% for s in svservices %}

/etc/sv/{{ s }}/run:
  file.managed:
    - source: salt://elliptics-rtc/etc/sv/{{ s }}/run
    - mode: 755
    - makedirs: True

/etc/sv/{{ s }}/log/run:
  file.managed:
    - source: salt://elliptics-rtc/etc/sv/{{ s }}/log/run
    - mode: 755
    - makedirs: True

/etc/service/{{ s }}:
   file.symlink:
     - target: /etc/sv/{{ s }}

{% endfor %}

/etc/sv/spacemimic/finish:
  file.managed:
    - source: salt://elliptics-rtc/etc/sv/spacemimic/finish
    - mode: 755
    - makedirs: True

/var/log/runit/cocaine-runtime/config:
  file.managed:
    - source: salt://elliptics-rtc/var/log/runit/cocaine-runtime/config
    - mode: 755
    - makedirs: True

create finish files for all services in runit:
  cmd.script:
    - source: salt://elliptics-rtc/scripts/finish.sh

/etc/build.ok:
  file.managed:
    - source: salt://elliptics-rtc/etc/build.ok
    - mode: 666
    - makedirs: True

