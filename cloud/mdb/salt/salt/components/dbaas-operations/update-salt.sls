{% set salt_version = salt['pillar.get']('data:salt_version', '3000.9+ds-1+yandex0') %}

remove-master-keys:
    file.absent:
        - names:
            - /etc/salt/pki/minion/master_sign.pub
            - /etc/salt/pki/minion/minion_master.pub

restart-mdb-ping-salt-master:
    cmd.run:
        - name: 'systemctl restart mdb-ping-salt-master'
        - require:
            - remove-master-keys

salt-pkgs:
    pkg.installed:
        - pkgs:
            - salt-common: {{ salt_version }}
            - salt-minion: {{ salt_version }}
        - require:
            - restart-mdb-ping-salt-master

{% if salt['pillar.get']('data:monrun2') %}
/etc/monrun/conf.d/salt_minion_status.conf:
    file.managed:
        - mode: '0644'
        - template: jinja
        - source: salt://components/monrun2/common/conf.d/salt_minion_status.conf
{% endif %}
