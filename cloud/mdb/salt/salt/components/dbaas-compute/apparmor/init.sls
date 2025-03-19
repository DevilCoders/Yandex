apparmor:
    pkg.installed:
        - pkgs:
            - apparmor
            - apparmor-utils
            - osquery-vanilla: 5.0.1.3
{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'package' %}
            - apparmor-ycloud-ycommon-prof: '1.0.7'
{% else %}
apparmor-ycloud-ycommon-prof:
    pkg.purged

/etc/apparmor.d/tunables:
    file.recurse:
        - source: salt://components/apparmor-profiles/tunables
        - user: root
        - group: root
        - file_mode: '0644'
        - require:
            - pkg: apparmor
            - pkg: apparmor-ycloud-ycommon-prof

/etc/apparmor.d/abstractions:
    file.recurse:
        - source: salt://components/apparmor-profiles/abstractions
        - user: root
        - group: root
        - file_mode: '0644'
        - exclude_pat: yc-mdb-pg
        - require:
            - pkg: apparmor
            - pkg: apparmor-ycloud-ycommon-prof
{% endif %}

auditd:
    pkg.purged

osqueryd-running:
    service.running:
        - name: osqueryd
        - require:
            - pkg: apparmor

{% set osrelease = salt['grains.get']('osrelease') %}
{% if osrelease == '18.04' %}
systemd-journald-audit.socket:
    service.masked

systemd-journald.service:
    service.running:
        - watch:
             - systemd-journald-audit.socket
{% endif %}
