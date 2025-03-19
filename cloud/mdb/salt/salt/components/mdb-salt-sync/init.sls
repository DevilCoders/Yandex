{% set personal = salt['pillar.get']('data:mdb_salt_sync:personal', False) %}
{% set source_of_truth = salt['pillar.get']('data:mdb_salt_sync:source_of_truth', False) %}

{% if source_of_truth or personal %}

mdb-salt-sync-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-salt-sync: '1.9268650'
        - prereq_in:
            - cmd: repositories-ready

/opt/yandex/mdb-salt-sync/repos.sls:
    file.managed:
        - source: salt://{{ slspath + '/conf/repos.sls' }}
        - mode: 644
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/opt/yandex/mdb-salt-sync/mdb-salt-sync-envs-run.sh:
    file.absent

/opt/yandex/mdb-salt-sync/fix-permissions.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/fix-permissions.sh' }}
{% if personal %}
        - mode: 755
{% else %}
        - mode: 744
{% endif %}
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/opt/yandex/mdb-salt-sync/sync-envs.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/sync-envs.sh' }}
{% if personal %}
        - mode: 755
{% else %}
        - mode: 744
{% endif %}
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/opt/yandex/mdb-salt-sync/sync-dev-and-private.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/sync-dev-and-private.sh' }}
{% if personal %}
        - mode: 755
{% else %}
        - mode: 744
{% endif %}
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/opt/yandex/mdb-salt-sync/bootstrap-srv.sh:
    file.managed:
        - source: salt://{{ slspath + '/conf/bootstrap-srv.sh' }}
{% if personal %}
        - mode: 755
{% else %}
        - mode: 744
{% endif %}
        - user: root
        - group: root
        - require:
            - pkg: mdb-salt-sync-pkgs

/home/robot-pgaas-deploy/.ssh/id_ecdsa:
    file.managed:
        - source: salt://{{ slspath }}/conf/id_ecdsa_sync
        - template: jinja
        - mode: 600
        - user: robot-pgaas-deploy
        - group: dpt_virtual_robots_1561
        - require:
            - pkg: common-packages

include:
{% if source_of_truth %}
    - .source_of_truth
{% endif %}
{% if personal %}
    - .personal
{% endif %}

{% endif %}
