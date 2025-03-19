{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
/usr/local/yandex/gp_restore_from_backup.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/gp_restore_from_backup.py
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 755
        - require:
            - file: /usr/local/yandex
{% endif %}

{% if salt.pillar.get('gpdb_master') %}
{%   if salt.pillar.get('restore-from:cid') %}

/tmp/recovery-state:
    file.directory:
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 0750
        - makedirs: True

greenplum-restore:
    cmd.run:
        - name: >
            python /usr/local/yandex/gp_restore_from_backup.py
            --recovery-state=/tmp/recovery-state
            --restore-config=/etc/wal-g/wal-g-restore-config.json
            --gp-bin={{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin
            --gp-master-data={{ gpdbvars.masterdir }}
            --gp-segment-prefix={{ gpdbvars.segprefix }}
            --gp-version={{ gpdbvars.gpmajver }}
            --old-gpadmin-password={{ salt['pillar.get']('data:restore-from-pillar-data:greenplum:users:gpadmin:password', '') | yaml_encode }}
            --new-gpadmin-password={{ salt['pillar.get']('data:greenplum:users:gpadmin:password', '') | yaml_encode }}
            --new-monitor-password={{ salt['pillar.get']('data:greenplum:users:monitor:password', '') | yaml_encode }}
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
        - require:
            - file: /usr/local/yandex/gp_restore_from_backup.py
            - file: /tmp/recovery-state
            - file: {{ gpdbvars.gplog }}
            - cmd: backup-fetched
            - sls: components.greenplum.service
        - require_in:
            - cmd: restart-greenplum-cluster

{%   else %}

Init-GreenplumDB-{{ gpdbvars.gpmajver }}:
  cmd.run:
    - name: rm -rf {{ gpdbvars.gplog }}/gpinit*.log; {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/gpinitsystem -a
            {% if salt.pillar.get('data:greenplum:config:segment_mirroring_enable', True) %}--mirror-mode={{ gpdbvars.mirror_type }}{% endif %}
            -I gpconfigs/configuration_file
            -h gpconfigs/segments
            -l {{ gpdbvars.gplog }}
            -p gpconfigs/gpopts.conf
            --ignore-warnings
            {% if gpdbvars.gpadmin_pwd != '' %}--su_password={{ gpdbvars.gpadmin_pwd }}{% endif %}

    - cwd: /home/{{ gpdbvars.gpadmin }}
    - runas: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - shell: /bin/bash
    - env:
      - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
      - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
      - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
      - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
      - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
    - creates: {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1
    - require:
       - sls: components.greenplum.install_greenplum
       - sls: components.greenplum.configure_greenplum
       - sls: components.greenplum.service
       - cmd: gp-cluster-up
    - require_in:
       - cmd: restart-greenplum-cluster
{%   endif %}

gp-cluster-up:
    cmd.run:
        - name: /usr/local/yandex/gp_wait_cluster.py -w 1 -r 600 -s
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - require:
            - file: /usr/local/yandex/gp_wait_cluster.py
{%   if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
            - sls: components.greenplum.firewall
{%   endif %}

restart-greenplum-cluster:
  cmd.run:
    - name: echo -e "\nexternal_pid_file='/var/run/greenplum/gp.pid'" >> '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'/postgresql.conf; {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/gpstop -a -M fast
    - cwd: /home/{{ gpdbvars.gpadmin }}
    - runas: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - shell: /bin/bash
    - env:
      - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
      - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
      - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
      - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
      - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
    - onlyif:
      - {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/gpstate -q

greenplum:
  service.running:
    - enable: true
    - require:
      - cmd: restart-greenplum-cluster

{% else %}
not_init_master:
  test.nop
{% endif %}
