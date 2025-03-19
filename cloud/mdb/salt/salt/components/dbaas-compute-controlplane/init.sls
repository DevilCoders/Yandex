/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-controlplane-config
        - mode: '0644'
        - require_in:
            - pkg: controlplane-osquery-pkgs

old-controlplane-osquery-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-controlplane-config
        - require_in:
            - file: /etc/osquery.tag

controlplane-osquery-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - osquery-vanilla: 5.0.1.3
        - require:
            - pkg: old-controlplane-osquery-pkgs
        - prereq_in:
            - cmd: repositories-ready

auditd:
    pkg.purged

osqueryd-running:
    service.running:
        - name: osqueryd
        - require:
            - pkg: controlplane-osquery-pkgs

{% set osrelease = salt['grains.get']('osrelease') %}
{% if osrelease == '18.04' %}
systemd-journald-audit.socket:
    service.masked

systemd-journald.service:
    service.running:
        - watch:
             - systemd-journald-audit.socket
{% endif %}

include:
    - components.dbaas-controlplane
    - components.linux-kernel
{% if salt['pillar.get']('data:control_plane_interfaces', 'True') %}
    - .network
{% endif %}

/etc/cron.d/cloud-21192-band-aid:
    file.absent

/etc/logrotate.d/cloud-21192-band-aid:
    file.absent

cloud-21192-band-aid-logs-clean:
    file.tidied:
        - name: /var/log
        - matches:
            - '^cloud-21192.*'
        - require:
            - file: /etc/cron.d/cloud-21192-band-aid
