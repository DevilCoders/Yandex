{% set cgroup = grains['conductor']['group'] %}

#ELLIPTICS:STORAGE

{% if 'elliptics-test-storage' in cgroup or 'ape-test-cloud' in cgroup or 'elliptics-storage' in cgroup or 'ape-cloud' in cgroup %}

{% if 'test' in cgroup %}

/etc/yandex-hbf-agent/targets.list:
    file.append:
        - makedirs: True
        - text:
          - "pid:_SRW_APPS_TESTING_NETS_"
          - "pid:_APEQLTESTINGNETS_"
          - "pid:_BROWSER_API_TEST_NETS_"

{% else %}

/etc/yandex-hbf-agent/targets.list:
    file.append:
        - makedirs: True
        - text:
          - "pid:_SRW_APPS_PRODUCTION_NETS_"
          - "pid:_APEQLPRODTINGNETS_"
          - "pid:_BROWSER_API_PROD_NETS_"

{% endif %}

/etc/monrun/conf.d/oops-agent-check.conf:
    file.managed:
        - source: salt://units/cocaine/files/etc/monrun/conf.d/oops-agent-check.conf
        - user: root
        - group: root
        - mode: 644

/usr/bin/oops-agent-check.sh:
    file.managed:
        - source: salt://units/cocaine/files/usr/bin/oops-agent-check.sh
        - user: root
        - group: root
        - mode: 755

isolate-cleanup:
  cron.present:
    - name: /usr/bin/find /place/cocaine-isolate-daemon/ -maxdepth 1 -type f -name "sha256:*" -atime +365 -delete && /usr/bin/timeout -s KILL 3600 /usr/sbin/portoctl layer -F 60 2>/dev/null
    - user: root
    - identifier: isolate-cleanup
    - minute: '39'
    - hour: '5'
    - dayweek: '2'

{% endif %}

/etc/yandex-hbf-agent/rules.d/30-dom0-2-mtn.v4:
    file.managed:
        - source: salt://units/cocaine/files/etc/yandex-hbf-agent/rules.d/30-dom0-2-mtn.any
        - user: root
        - group: root

/etc/yandex-hbf-agent/rules.d/30-dom0-2-mtn.v6:
    file.managed:
        - source: salt://units/cocaine/files/etc/yandex-hbf-agent/rules.d/30-dom0-2-mtn.any
        - user: root
        - group: root

/etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf:
    file.managed:
        - source: salt://units/cocaine/files/etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf
        - user: root
        - group: root
        - makedirs: True
        - template: jinja

/usr/local/bin/srw_instance_getter.sh:
    file.managed:
        - source: salt://units/cocaine/files/usr/local/bin/srw_instance_getter.sh
        - user: root
        - group: root
        - mode: 755

# ORCA CONF
/etc/cocaine/orca.yaml:
    file.managed:
        - source: salt://units/cocaine/files/etc/cocaine/orca.yaml
        - template: jinja
        - user: root
        - group: root
        - mode: 644

{% if cgroup == 'ape-pipeline' %}

/etc/monrun/conf.d/check-ip-broker.conf:
    file.managed:
        - source: salt://units/cocaine/files/etc/monrun/conf.d/check-ip-broker.conf
        - user: root
        - group: root
        - mode: 644

/usr/bin/check-ip-broker.py:
    file.managed:
        - source: salt://units/cocaine/files/usr/bin/check-ip-broker.py
        - user: root
        - group: root
        - mode: 755

{% endif %}

