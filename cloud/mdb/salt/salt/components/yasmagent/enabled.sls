yasmagent-enable:
    service.enabled:
        - name: yasmagent

yasmagent:
    service.running:
        - require:
            - service: yasmagent-enable
        - watch:
            - file: yasmagent-init
    pkg.installed:
        - name: yandex-yasmagent
        - version: '2.436-20200617'

kill-orphan-yasmagent-tails:
    cmd.wait:
        - name: pkill -f ^tail || true
        - watch:
            - service: yasmagent

/etc/default/yasmagent:
    file.absent:
        - require:
            - file: yasmagent-watchdog

/etc/cron.d/yandex-yasmagent:
    file.absent:
        - require:
            - file: yasmagent-watchdog

yasmagent-watchdog:
    file.absent:
        - name: /usr/local/yasmagent/watchdog.sh
        - require:
            - pkg: yasmagent

yasmagent-init:
    file.absent:
        - name: /etc/init.d/yasmagent
        - require:
            - file: yasmagent-watchdog

/lib/systemd/system/yasmagent.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/yasmagent.service
        - template: jinja
        - require:
            - file: yasmagent-init
        - require_in:
            - service: yasmagent-enable
        - watch_in:
            - service: yasmagent
        - onchanges_in:
            - module: systemd-reload

{% set runlist = salt['pillar.get']('data:runlist', []) %}
{% if 'components.dom0porto' not in runlist and 'components.dbaas-internal-api' not in runlist %}
yasmagent-kill-if-many-script:
    file.managed:
        - name: /usr/local/yandex/yasmagent_kill_if_many.sh
        - source: salt://{{ slspath }}/conf/yasmagent_kill_if_many.sh
        - makedirs: True
        - mode: 755

yasmagent-custom-watch:
    file.managed:
        - name: /etc/cron.d/yasmagent-kill-superfluous
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             * * * * * root /usr/local/yandex/yasmagent_kill_if_many.sh >>/var/log/yasmagent/kill_if_many.log 2>&1 || /bin/true
        - mode: 644
{% endif %}
