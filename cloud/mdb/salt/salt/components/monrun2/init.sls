{% if not salt['pillar.get']('data:monrun:disable', False) %}
yamail-monitor:
    pkg.purged:
        - require_in:
            - pkg: juggler-pgks

config-juggler-client-mail:
    pkg.purged:
        - require_in:
            - pkg: juggler-pgks

snaked:
    pkg.purged:
       - require_in:
            - pkg: juggler-pgks

yamail-monrun:
    pkg.purged:
       - require_in:
            - pkg: juggler-pgks

yamail-monrun-common:
    pkg.purged:
       - require_in:
            - pkg: juggler-pgks

juggler-client-purge:
    pkg.purged:
       - version: '1.4.53'
       - require_in:
            - pkg: juggler-pgks

monrun-purge:
    pkg.purged:
       - version: '1.2.52-1+sleep.1'
       - require_in:
            - pkg: juggler-pgks

config-juggler-search:
    pkg.purged

juggler-pgks:
    pkg.installed:
        - pkgs:
            - python-requests
            - python3-openssl
            - monrun: '1.3.2'
            - juggler-client: '2.1.05311124'
            - yandex-timetail
            - python3-dnspython
            - bc
            - yandex-lib-autodetect-environment
        - require:
            - pkgrepo: mdb-bionic-stable-all
            - pkgrepo: mdb-bionic-stable-arch
        - require_in:
            - cmd: monrun-jobs-update

yamail-monrun-errata:
    pkg.installed:
        - version: 38-ae478e8
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: monrun-jobs-update

yamail-monrun-restart-required:
    pkg.installed:
        - version: '5-d922d04'
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: monrun-jobs-update

yandex-media-common-grub-check:
    pkg.purged:
        - watch_in:
            - cmd: monrun-jobs-update

yandex-media-common-iface-ip-check:
    pkg.purged:
        - watch_in:
            - cmd: monrun-jobs-update

/etc/monrun/conf.d/iface_ip.conf:
    file.absent:
        - watch_in:
            - cmd: monrun-jobs-update

juggler-client:
    service:
        - running
        - require:
            - pkg: juggler-pgks
        - require_in:
            cmd: monrun-jobs-update

juggler-client-restart:
    cmd.wait:
        - name: "service juggler-client restart"
        - require:
            - pkg: juggler-pgks
            - service: juggler-client

include:
    - .common
    - .update-jobs

/var/log/juggler-client:
    file.directory:
        - user: monitor
        - group: monitor
        - recurse:
            - user
            - group
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: juggler-client-restart

/etc/cron.d/monrun-cache-chown:
    file.managed:
        - source: salt://{{ slspath }}/config/monrun-cache-chown.cron

/etc/logrotate.d/monrun:
    file.managed:
        - source: salt://{{ slspath }}/config/logrotate.conf
        - mode: 644
        - makedirs: True

/etc/logrotate.d/apparmor-check:
    file.managed:
        - source: salt://{{ slspath }}/config/apparmor-check-logrotate.conf
        - mode: 644
        - makedirs: True

/var/log/monrun:
    file.directory:
        - user: monitor
        - group: monitor
        - recurse:
            - user
            - group
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: juggler-client-restart

/etc/monrun/general.conf:
    file.managed:
        - source: salt://{{ slspath }}/config/general.conf
        - user: monitor
        - group: monitor
        - mode: 644
        - require:
            - pkg: juggler-pgks
        - watch_in:
            - cmd: juggler-client-restart

/etc/juggler/etc:
    file.directory:
        - user: root
        - group: root
        - mode: 755
        - require:
            - pkg: juggler-pgks
            - pkg: config-juggler-search

/etc/juggler/etc/client.conf:
    file.managed:
        - source: salt://{{ slspath }}/config/client.conf
        - template: jinja
        - user: root
        - group: root
        - mode: 644
        - require:
            - pkg: juggler-pgks
            - file: /etc/juggler/etc
        - watch_in:
            - cmd: juggler-client-restart

/etc/cron.yandex/wd-juggler-client.sh:
    file.managed:
        - source: salt://{{ slspath }}/config/wd-juggler-client.sh
        - mode: '0755'
        - require:
            - pkg: juggler-pgks

/etc/cron.d/juggler-client:
    file.managed:
        - source: salt://{{ slspath }}/config/wd-juggler-client.cron
        - require:
            - pkg: juggler-pgks
            - file: /etc/cron.yandex/wd-juggler-client.sh

/etc/default/juggler-client:
    file.managed:
        - source: salt://{{ slspath }}/config/juggler-client
        - user: root
        - group: root
        - mode: 644
        - require:
            - pkg: juggler-pgks
            - pkg: config-juggler-search
        - watch_in:
            - cmd: juggler-client-restart
{% else %}
monrun-pkgs-purged:
    pkg.purged:
        - pkgs:
            - monrun
            - juggler-client

{%     for entity in ['/var/log/juggler-client', '/etc/cron.d/monrun-cache-chown', '/etc/logrotate.d/monrun',
                      '/var/log/monrun', '/var/log/monrun.log', '/etc/monrun', '/etc/juggler',
                      '/etc/cron.yandex/wd-juggler-client.sh', '/etc/cron.d/juggler-client',
                      '/etc/default/juggler-client', '/etc/logrotate.d/apparmor-check']: %}
{{ entity }}:
    file.absent:
        - require:
            - pkg: monrun-pkgs-purged
{%     endfor %}
{% endif %}
